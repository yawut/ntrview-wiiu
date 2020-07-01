#include "Network.hpp"

#include <unistd.h>
#include <fstream>

Network g_network;
Network& GetNetwork() {
    return g_network;
}

void Network::ConnectDS() {
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
    ret = inet_pton(ds_addr.sin_family, "10.0.0.45", &(ds_addr.sin_addr));
    if (ret <= 0) {
        NetworkError("IP decode error");
        return;
    }

    ret = connect(ds_sock, (struct sockaddr*)&ds_addr, sizeof(ds_addr));
    if (ret < 0) {
        NetworkError("Can't connect");
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
    int jpeg_size = ret - 4;
    int jpeg_offset = seq * (UDP_PACKET_SIZE - 4);

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
        for (int i = 0; i < jpeg_size; i++) {
            jpeg[jpeg_offset + i] = jpeg_data[i];
        }
    } else {
        for (int i = 0; i < jpeg_size; i++) {
            jpeg[jpeg_offset + i] = jpeg_data[i];
        }
    }

    if (lastPacket) {
        jpegs_done[jpeg_ndx] = true;
        //printf("%s jpegs: %d:%s %d:%s %d:%s\n", isTop?"top":"btm", jpeg_map[0], jpegs_done[0]?"true":"false", jpeg_map[1],jpegs_done[1]?"true":"false", jpeg_map[2],jpegs_done[2]?"true":"false");
    }
}

void Network::SendRemotePlay(uint32_t mode, uint32_t quality, uint32_t qos) {
    Packet pac = {
        .magic = PacketSwap(0x12345678),
        .seq = PacketSwap(1),
        .type = PacketSwap(0),
        .cmd = PacketSwap(901),
        .args = { .RemotePlay = {
            .mode = PacketSwap(mode),
            .quality = PacketSwap(quality),
            .qos = PacketSwap(qos),
        }},

        .length = PacketSwap(0),
    };
    send(ds_sock, &pac, sizeof(pac), 0);
}

void Network::SendHeartbeat() {
    Packet pac = {
        .magic = PacketSwap(0x12345678),
        .seq = PacketSwap(1),
        .type = PacketSwap(0),
        .cmd = PacketSwap(0),
        .args = { .raw = { 0 } },

        .length = PacketSwap(0),
    };
    send(ds_sock, &pac, sizeof(pac), 0);
}


void NetworkThread() {
    Network& network = GetNetwork();
    network.ListenUDP();
    network.ConnectDS();
    network.SendRemotePlay(1 << 8 | 5, 90, 30 * 1024 * 1024 / 8);

    for (int i = 0; i < 2; i++) {
        network.SendHeartbeat();
        sleep(1);
    }
    while (!network.quit) {
        network.RecieveUDP();
    }
}

Network::Network() {
    for (auto& v : jpegs_top) {
        v.reserve(30000);
    }
    for (auto& v : jpegs_btm) {
        v.reserve(30000);
    }
}

Network::~Network() {
    if (ds_sock >= 0) {
        shutdown(ds_sock, 2); //HACK wiiu compat
    }
}
