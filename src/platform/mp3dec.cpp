// SceAudiodec MP3 decoders in a fixed pool, each with its own aligned input and output buffers.

#include <platform/mp3dec.h>
#include <platform/log.h>
#include <platform/sync.h>

#include <psp2/audiodec.h>

#include <cstdlib>
#include <cstring>

namespace pf { namespace mp3
{

class Decoder
{
public:
    SceAudiodecCtrl ctrl;
    SceAudiodecInfo info;
    uint8_t* es = nullptr;
    int16_t* pcm = nullptr;
    bool used = false;
};

namespace
{

const uint32_t EsBytes = SCE_AUDIODEC_ROUND_UP(SCE_AUDIODEC_MP3_MAX_ES_SIZE);
const uint32_t PcmBytes = SCE_AUDIODEC_ROUND_UP(SCE_AUDIODEC_MP3_MAX_SAMPLES * 2 * sizeof(int16_t));

Decoder s_decoders[MaxDecoders];
LwMutex* s_lock = nullptr;
int s_state = 0;            // 0 not tried, 1 ready, -1 unavailable
int s_error = 0;

}

bool init()
{
    if (s_state)
        return s_state > 0;
    s_state = -1;
    SceAudiodecInitParam param;
    memset(&param, 0, sizeof(param));
    param.size = sizeof(param.mp3);
    param.mp3.totalStreams = MaxDecoders;
    const int result = sceAudiodecInitLibrary(SCE_AUDIODEC_TYPE_MP3, &param);
    if (result < 0 && result != int(SCE_AUDIODEC_ERROR_ALREADY_INITIALIZED))
    {
        s_error = result;
        pf::logError("mp3: sceAudiodecInitLibrary: 0x%08X", unsigned(result));
        return false;
    }
    s_lock = new LwMutex("mp3_pool");
    s_state = 1;
    return true;
}

Decoder* create()
{
    if (s_state <= 0)
        return nullptr;
    BasicLockGuard<LwMutex> guard(*s_lock);
    for (Decoder& d : s_decoders)
    {
        if (d.used)
            continue;
        if (!d.es)
        {
            d.es = static_cast<uint8_t*>(aligned_alloc(SCE_AUDIODEC_ALIGNMENT_SIZE, EsBytes));
            d.pcm = static_cast<int16_t*>(aligned_alloc(SCE_AUDIODEC_ALIGNMENT_SIZE, PcmBytes));
            if (!d.es || !d.pcm)
                return nullptr;
            memset(d.es, 0, EsBytes);
        }
        memset(&d.info, 0, sizeof(d.info));
        d.info.size = sizeof(d.info.mp3);
        d.info.mp3.size = sizeof(d.info.mp3);
        d.info.mp3.ch = 2;
        d.info.mp3.version = SCE_AUDIODEC_MP3_MPEG_VERSION_1;
        memset(&d.ctrl, 0, sizeof(d.ctrl));
        d.ctrl.size = sizeof(d.ctrl);
        d.ctrl.pEs = d.es;
        d.ctrl.maxEsSize = SCE_AUDIODEC_MP3_MAX_ES_SIZE;
        d.ctrl.pPcm = d.pcm;
        d.ctrl.maxPcmSize = PcmBytes;
        d.ctrl.wordLength = SCE_AUDIODEC_WORD_LENGTH_16BITS;
        d.ctrl.pInfo = &d.info;
        const int result = sceAudiodecCreateDecoder(&d.ctrl, SCE_AUDIODEC_TYPE_MP3);
        if (result < 0)
        {
            s_error = result;
            return nullptr;
        }
        d.used = true;
        return &d;
    }
    return nullptr;
}

void destroy(Decoder* decoder)
{
    if (!decoder)
        return;
    BasicLockGuard<LwMutex> guard(*s_lock);
    sceAudiodecDeleteDecoder(&decoder->ctrl);
    decoder->used = false;
}

bool reset(Decoder* decoder)
{
    const int result = sceAudiodecClearContext(&decoder->ctrl);
    if (result < 0)
        s_error = result;
    return result >= 0;
}

int decode(Decoder* decoder, const uint8_t* frame, uint32_t size, int16_t* pcm)
{
    if (size > SCE_AUDIODEC_MP3_MAX_ES_SIZE)
        return -1;
    memcpy(decoder->es, frame, size);
    decoder->ctrl.inputEsSize = 0;
    decoder->ctrl.outputPcmSize = 0;
    const int result = sceAudiodecDecode(&decoder->ctrl);
    if (result < 0)
    {
        s_error = result;
        return -1;
    }
    const uint32_t bytes = decoder->ctrl.outputPcmSize;
    memcpy(pcm, decoder->pcm, bytes);
    return int(bytes / (2 * sizeof(int16_t)));
}

int lastError()
{
    return s_error;
}

}}
