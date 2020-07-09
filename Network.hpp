#pragma once

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
#endif

namespace Network {

typedef enum State {
    CONNECTING,
    CONNECTED_WAIT,
    CONNECTED_STREAMING
} State;

void ConnectDS(const std::string host);
void ListenUDP();
void RecieveUDP();
void SendRemotePlay(uint8_t priority, uint8_t priorityFactor, uint8_t jpegQuality, uint8_t QoS);
int SendHeartbeat();

State GetNetworkState();
int GetConnectionAttempts();

std::vector<uint8_t> GetBtmJPEG();
int GetBtmJPEGID();
std::vector<uint8_t> GetTopJPEG();
int GetTopJPEGID();

void mainLoop(const std::string host, uint8_t priority, uint8_t priorityFactor, uint8_t jpegQuality, uint8_t QoS);
void Quit();

#define NetworkError(fmt) printf("[Network] " fmt ": %d\n", socketlasterr())
#define NetworkErrorF(fmt, ...) printf("[Network] " fmt ": %d\n", __VA_ARGS__, socketlasterr())

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

static inline uint32_t Swap(uint32_t in) {
#ifdef __WIIU__
    return __builtin_bswap32(in);
#else
    return in;
#endif
}

#define UDP_PACKET_SIZE (1448)

} //namespace Network
