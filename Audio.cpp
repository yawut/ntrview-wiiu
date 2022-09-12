#include "Audio.h"

#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include <coreinit/cache.h>
#include <vector>
#include <span>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

static std::vector<uint16_t> voice_1click_data;
static std::vector<uint16_t> voice_2click_data;
static AXVoice* voice_1click;
static AXVoice* voice_2click;

const static unsigned char wav_header[32] = {
    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x7d, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00,
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61
};

static void read_wav(const char* path, std::vector<uint16_t>& samples) {
    FILE* fd = fopen(path, "rb");
    if (!fd)
        return;

    struct stat stats;
    if(fstat(fileno(fd), &stats))
        return;
    auto size = stats.st_size;
    if (size < sizeof(wav_header) + 12) return;

    std::vector<unsigned char> buffer(size);
    auto res = fread(buffer.data(), size, 1, fd);
    fclose(fd);
    if (res != 1) return;

    if (memcmp(buffer.data() + 8, wav_header, sizeof(wav_header)) != 0)
        return;

    size_t doff = sizeof(wav_header) + 8;
    size_t data_size = buffer[doff] | (buffer[doff+1] << 8) | (buffer[doff+2] << 16) | (buffer[doff+3] << 24);

    std::span<unsigned char> sample_data {buffer.begin() + sizeof(wav_header) + 12, data_size};
    samples.resize(sample_data.size() / 2);
    for (uint i = 0; i < samples.size(); i++) {
        samples[i] = sample_data[i*2] | (sample_data[i*2+1] << 8);
    }
    DCFlushRange(samples.data(), samples.size() * sizeof(samples[0]));
}

static AXVoiceDeviceMixData mono_mix[6 /* channels */] = {
    { .bus = {
        {.volume = 0xC000},
    }},
    { .bus = {
        {.volume = 0xC000},
    }},
};

static void setup_voice(AXVoice* voice, const std::vector<uint16_t>& samples) {
    AXVoiceVeData vol = {
        .volume = 0xC000,
    };

    AXVoiceBegin(voice);
    AXSetVoiceType(voice, 0);
    AXSetVoiceVe(voice, &vol);
    AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_DRC, 0, mono_mix);
    AXSetVoiceDeviceMix(voice, AX_DEVICE_TYPE_TV, 0, mono_mix);
    AXSetVoiceSrcType(voice, AX_VOICE_SRC_TYPE_NONE);

    AXVoiceOffsets offs = {
        .dataType = AX_VOICE_FORMAT_LPCM16,
        .loopingEnabled = AX_VOICE_LOOP_DISABLED,
        .endOffset = samples.size(),
        .data = samples.data(),
    };
    AXSetVoiceOffsets(voice, &offs);

    AXVoiceEnd(voice);
}

void Audio::Init() {
    // read in voice data
    read_wav("fs:/vol/content/1click.wav", voice_1click_data);
    read_wav("fs:/vol/content/2click.wav", voice_2click_data);

    // init AX
    AXInitParams params = {
        .renderer = AX_INIT_RENDERER_32KHZ,
        .pipeline = AX_INIT_PIPELINE_SINGLE,
    };
    AXInitWithParams(&params);

    voice_1click = AXAcquireVoice(31, nullptr, nullptr);
    setup_voice(voice_1click, voice_1click_data);
    voice_2click = AXAcquireVoice(31, nullptr, nullptr);
    setup_voice(voice_2click, voice_2click_data);
}

void Audio::Shutdown() {
    AXFreeVoice(voice_1click);
    AXFreeVoice(voice_2click);
    AXQuit();
}

void Audio::Play1Click() {
    AXSetVoiceCurrentOffset(voice_1click, 0);
    AXSetVoiceState(voice_1click, AX_VOICE_STATE_PLAYING);
}

void Audio::Play2Click() {
    AXSetVoiceCurrentOffset(voice_2click, 0);
    AXSetVoiceState(voice_2click, AX_VOICE_STATE_PLAYING);
}
