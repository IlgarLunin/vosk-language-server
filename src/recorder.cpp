#include "recorder.h"


MicrophoneRecorder::MicrophoneRecorder(VoiceChunkCallback callback)
{
    _callback = callback;
}

bool MicrophoneRecorder::onProcessSamples(const sf::Int16 *samples, size_t sampleCount)
{
    // invoke callback
    _callback(samples, sampleCount);

    return true;
}
