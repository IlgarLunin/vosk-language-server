#pragma once
#include <functional>
#include <memory>
#include "SFML/Audio/SoundBufferRecorder.hpp"




class MicrophoneRecorder final :
        public sf::SoundBufferRecorder
{
    typedef std::function<void(const sf::Int16 *samples, size_t sampleCount)> VoiceChunkCallback;

public:
    typedef std::shared_ptr<MicrophoneRecorder> Pointer;

    explicit MicrophoneRecorder(VoiceChunkCallback callback);

protected:
    bool onProcessSamples(const sf::Int16 *samples, size_t sampleCount) override;
private:
    VoiceChunkCallback _callback = [](const sf::Int16 *samples, size_t sampleCount){};
};
