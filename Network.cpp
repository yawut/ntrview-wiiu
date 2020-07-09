#include "Network.hpp"

#include <unistd.h>
#include <fstream>
#include <thread>

#ifdef __WIIU__
//cool
#else
static int socketlasterr() {
    return errno;
}
#endif

using namespace Network;

State state = CONNECTING;
bool quit = false;

int ds_sock = -1;
int udp_sock = -1;

int connect_attempts = 0;

std::array<int, 3> jpeg_map_top = { 0, 1, 2 };
std::array<bool, 3> jpegs_done_top = { false, false, false };
std::array<std::vector<uint8_t>, 3> jpegs_top;
std::recursive_mutex jpegs_top_mtx;

std::array<int, 3> jpeg_map_btm = { 0, 1, 2 };
std::array<bool, 3> jpegs_done_btm = { false, false, false };
std::array<std::vector<uint8_t>, 3> jpegs_btm;
std::recursive_mutex jpegs_btm_mtx;

void Network::ConnectDS(const std::string host) {
    int ret;

    ds_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ds_sock < 0) {
        NetworkError("Couldn't make socket");
        return;
    }

    struct sockaddr_in ds_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8000),
        .sin_addr = {0},
        .sin_zero = {0},
    }; //TODO
    ret = inet_pton(ds_addr.sin_family, host.c_str(), &(ds_addr.sin_addr));
    if (ret <= 0) {
        NetworkErrorF("Address %s invalid - check your config file", host.c_str());
        if (ds_sock >= 0) {
            shutdown(ds_sock, 2); //HACK wiiu compat
            ds_sock = -1;
        }
        return;
    }

    ret = connect(ds_sock, (struct sockaddr*)&ds_addr, sizeof(ds_addr));
    if (ret < 0) {
        NetworkErrorF("Can't connect to DS (%s)", host.c_str());
        if (ds_sock >= 0) {
            shutdown(ds_sock, 2); //HACK wiiu compat
            ds_sock = -1;
        }
        return;
    }
}

void Network::ListenUDP() {
    int ret;

    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in listen_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8001),
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        },
        .sin_zero = {0},
    };
    ret = bind(udp_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    if (ret < 0) {
        NetworkError("Can't bind to UDP");
        if (udp_sock >= 0) {
            shutdown(udp_sock, 2); //HACK wiiu compat
            udp_sock = -1;
        }
        return;
    }
}

void Network::RecieveUDP() {
    int ret;
    uint8_t buf[UDP_PACKET_SIZE];
    struct sockaddr_storage remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);

    ret = recvfrom(udp_sock, buf, UDP_PACKET_SIZE, 0, (struct sockaddr*)&remote_addr, &remote_addr_len);
    if (ret <= 0) {
        if (quit) return;
        NetworkError("RecieveUDP failed");
        return;
    }

    uint8_t id = buf[0];
    uint8_t flags = buf[1];
    uint8_t fmt = buf[2];
    uint8_t seq = buf[3];

    int isTop = flags & 1;
    int lastPacket = flags & 0x10;

    uint8_t* jpeg_data = buf + 4;
    uint jpeg_size = ret - 4;
    uint jpeg_offset = seq * (UDP_PACKET_SIZE - 4);

    int jpeg_ndx = -1;
    int lowest_ndx = 0;

    auto& jpeg_map = (isTop) ? jpeg_map_top : jpeg_map_btm;
    auto& jpegs_done = (isTop) ? jpegs_done_top : jpegs_done_btm;
    auto& jpegs = (isTop) ? jpegs_top : jpegs_btm;

    for (uint i = 0; i < jpeg_map.size(); i++) {
        if (jpeg_map[i] == id) {
            jpeg_ndx = i;
            break;
        } else if (jpeg_map[i] < jpeg_map[lowest_ndx]) {
            lowest_ndx = i;
        }
    }

    if (jpeg_ndx == -1) {
    /*  Overflow special-cases */
        if (id == 0 || id == 1 || id == 2) {
            int ndx_255 = -1, ndx_254 = -1, ndx_253 = -1;
            for (uint i = 0; i < jpeg_map.size(); i++) {
                if (jpeg_map[i] == 255) ndx_255 = i;
                if (jpeg_map[i] == 254) ndx_254 = i;
                if (jpeg_map[i] == 253) ndx_253 = i;
            }
            if (id == 1 && ndx_254 != -1) {
                jpeg_ndx = ndx_254;
            } else if (id == 2 && ndx_255 != -1) {
                jpeg_ndx = ndx_255;
            } else if (id == 0 && ndx_253 != -1) {
                jpeg_ndx = ndx_253;
            } else {
                jpeg_ndx = lowest_ndx;
            }
        } else {
            jpeg_ndx = lowest_ndx;
        }

        jpeg_map[jpeg_ndx] = id;
        jpegs_done[jpeg_ndx] = false;
    }

    auto& jpeg = jpegs[jpeg_ndx];
    if (jpeg.size() < (jpeg_offset + jpeg_size)) {
        //printf("%s: resizing jpeg %d/%ld to %d(%d)\n", isTop?"top":"btm",jpeg_ndx, jpeg.size(), (jpeg_offset + jpeg_size), (jpeg_offset + jpeg_size) / UDP_PACKET_SIZE);
        jpeg.resize(jpeg_offset + jpeg_size);
        for (uint i = 0; i < jpeg_size; i++) {
            jpeg[jpeg_offset + i] = jpeg_data[i];
        }
    } else {
        for (uint i = 0; i < jpeg_size; i++) {
            jpeg[jpeg_offset + i] = jpeg_data[i];
        }
    }

    if (lastPacket) {
        jpegs_done[jpeg_ndx] = true;
        state = CONNECTED_STREAMING;
        //printf("%s jpegs: %d:%s %d:%s %d:%s\n", isTop?"top":"btm", jpeg_map[0], jpegs_done[0]?"true":"false", jpeg_map[1],jpegs_done[1]?"true":"false", jpeg_map[2],jpegs_done[2]?"true":"false");
    }
}

void Network::SendRemotePlay(uint8_t priority, uint8_t priorityFactor, uint8_t jpegQuality, uint8_t QoS) {
    Packet pac = {
        .magic = Swap(0x12345678),
        .seq = Swap(1),
        .type = Swap(0),
        .cmd = Swap(901),
        .args = { .RemotePlay = {
            .mode = Swap(priority << 8 | priorityFactor),
            .quality = Swap(jpegQuality),
            .qos = Swap(QoS*2 << 16),
        }},

        .length = Swap(0),
    };
    send(ds_sock, &pac, sizeof(pac), 0);
}

int Network::SendHeartbeat() {
    Packet pac = {
        .magic = Swap(0x12345678),
        .seq = Swap(1),
        .type = Swap(0),
        .cmd = Swap(0),
        .args = { .raw = { 0 } },

        .length = Swap(0),
    };
    int ret = send(ds_sock, &pac, sizeof(pac), 0);
    if (ret < 0) {
        return socketlasterr();
    } else return 0;
}

static void heartbeatLoop() {
    while (!quit) {
        SendHeartbeat();
        sleep(1);
    }
}

void Network::mainLoop(const std::string host, uint8_t priority, uint8_t priorityFactor, uint8_t jpegQuality, uint8_t QoS) {
    //init bits
    for (auto& v : jpegs_top) {
        v.reserve(30000);
    }
    for (auto& v : jpegs_btm) {
        v.reserve(30000);
    }

    //connect
    ListenUDP();
    while (udp_sock < 0 && !quit) {
        sleep(1);
        connect_attempts++;
        ListenUDP();
    }
    ConnectDS(host);
    while (ds_sock < 0 && !quit) {
        sleep(1);
        connect_attempts++;
        ConnectDS(host);
    }
    if (quit) {
        printf("[Network] quit requested\n");
        if (ds_sock >= 0) {
            printf("[Network] Tearing down ds sock\n");
            shutdown(ds_sock, 2); //HACK wiiu compat
        }
        if (udp_sock >= 0) {
            printf("[Network] Tearing down udp sock\n");
            shutdown(udp_sock, 2); //HACK wiiu compat
        }
        printf("[Network] bye!\n");
        return;
    }

    state = CONNECTED_WAIT;
    sleep(2); //I know, I know

    SendRemotePlay(priority, priorityFactor, jpegQuality, QoS);

    for (int i = 0; i < 2 && !quit; i++) {
        SendHeartbeat();
        sleep(1);
    }
    std::thread heartbeatThread(heartbeatLoop);
    while (!quit) {
        RecieveUDP();
    }
    heartbeatThread.join();

    if (ds_sock >= 0) {
        shutdown(ds_sock, 2); //HACK wiiu compat
        ds_sock = -1;
    }
    if (udp_sock >= 0) {
        shutdown(udp_sock, 2); //HACK wiiu compat
        udp_sock = -1;
    }
}

void Network::Quit() {
    if (ds_sock >= 0) {
        shutdown(ds_sock, 2); //HACK wiiu compat
        ds_sock = -1;
    }
    if (udp_sock >= 0) {
        shutdown(udp_sock, 2); //HACK wiiu compat
        udp_sock = -1;
    }
    quit = true;
}

State Network::GetNetworkState() {
    return state;
}

int Network::GetConnectionAttempts() {
    return connect_attempts;
}

int Network::GetTopJPEGID() {
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
std::vector<uint8_t> Network::GetTopJPEG() {
    const std::lock_guard<std::recursive_mutex> lock(jpegs_top_mtx);
    int id = GetTopJPEGID();
    for (uint i = 0; i < jpeg_map_top.size(); i++) {
        if (jpeg_map_top[i] == id) {
            return jpegs_top[i];
        }
    }
    return {};
}
int Network::GetBtmJPEGID() {
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
std::vector<uint8_t> Network::GetBtmJPEG() {
    const std::lock_guard<std::recursive_mutex> lock(jpegs_btm_mtx);
    int id = GetBtmJPEGID();
    for (uint i = 0; i < jpeg_map_btm.size(); i++) {
        if (jpeg_map_btm[i] == id) {
            return jpegs_btm[i];
        }
    }
    return {};
}
