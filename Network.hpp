#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include <mutex>

#ifdef __WIIU__
#include <nsysnet/socket.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
static int socketlasterr() {
    return errno;
}
#endif

class Network {
public:
    void ConnectDS();
    void ListenUDP();
    void RecieveUDP();
    void SendRemotePlay(uint32_t mode, uint32_t quality, uint32_t qos);
    void SendHeartbeat();

    int GetTopJPEGID() {
        const std::lock_guard<std::recursive_mutex> lock(jpegs_top_mtx);
        int largest_id = -1;
        for (uint i = 0; i < jpegs_done_top.size(); i++) {
            if (jpegs_done_top[i]) {
                if (jpeg_map_top[i] > largest_id) {
                    largest_id = jpeg_map_top[i];
                }
            }
        }
        return largest_id;
    }
    std::vector<uint8_t> GetTopJPEG() {
        const std::lock_guard<std::recursive_mutex> lock(jpegs_top_mtx);
        int id = GetTopJPEGID();
        for (uint i = 0; i < jpeg_map_top.size(); i++) {
            if (jpeg_map_top[i] == id) {
                return jpegs_top[i];
            }
        }
        return {};
    };
    int GetBtmJPEGID() {
        const std::lock_guard<std::recursive_mutex> lock(jpegs_btm_mtx);
        int largest_id = -1;
        for (uint i = 0; i < jpegs_done_btm.size(); i++) {
            if (jpegs_done_btm[i]) {
                if (jpeg_map_btm[i] > largest_id) {
                    largest_id = jpeg_map_btm[i];
                }
            }
        }
        return largest_id;
    }
    std::vector<uint8_t> GetBtmJPEG() {
        const std::lock_guard<std::recursive_mutex> lock(jpegs_btm_mtx);
        int id = GetBtmJPEGID();
        for (uint i = 0; i < jpeg_map_btm.size(); i++) {
            if (jpeg_map_btm[i] == id) {
                return jpegs_btm[i];
            }
        }
        return {};
    };

    bool quit = false;

    ~Network();
    Network();
private:
    int ds_sock = -1;
    int udp_sock = -1;

    std::array<int, 3> jpeg_map_top = { 0, 1, 2 };
    std::array<bool, 3> jpegs_done_top = { false, false, false };
    std::array<std::vector<uint8_t>, 3> jpegs_top;
    std::recursive_mutex jpegs_top_mtx;

    std::array<int, 3> jpeg_map_btm = { 0, 1, 2 };
    std::array<bool, 3> jpegs_done_btm = { false, false, false };
    std::array<std::vector<uint8_t>, 3> jpegs_btm;
    std::recursive_mutex jpegs_btm_mtx;
};

Network& GetNetwork();
void NetworkThread();

#define NetworkError(fmt) printf("[Network] " fmt ": %d\n", socketlasterr())

struct Packet {
    uint32_t magic;
    uint32_t seq;
    uint32_t type;
    uint32_t cmd;
    union {
        struct RemotePlay {
            uint32_t mode;
            uint32_t quality;
            uint32_t qos;
        } RemotePlay;
        uint32_t raw[16];
    } args;

    uint32_t length;
};

static uint32_t PacketSwap(uint32_t in) {
#ifdef __WIIU__
    return __builtin_bswap32(in);
#else
    return in;
#endif
}

#define UDP_PACKET_SIZE (1448)
