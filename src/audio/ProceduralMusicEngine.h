#pragma once

#include <atomic>

#include "core/GameConfig.h"
#include "core/GameSession.h"
#include "raylib.h"

namespace breakout {

class ProceduralMusicEngine {
public:
    bool Init();
    void Shutdown();

    void UpdateFromGame(const GameSession& session, const GameConfig& config);

    void OnRoundStart();
    void OnPaddleHit();
    void OnBrickHit();
    void OnLifeLost();
    void OnWin();
    void OnGameOver();

    void ToggleMute();
    bool IsMuted() const;

private:
    static void AudioCallback(void* bufferData, unsigned int frames);
    void RenderAudio(float* output, unsigned int frames);
    void PushEvent(std::atomic<unsigned int>& counter);

    struct CallbackState {
        unsigned int step = 0;
        unsigned int bar = 0;
        float samplesUntilStep = 0.0f;

        float bassPhase = 0.0f;
        float bassFreq = 110.0f;
        float bassAmp = 0.0f;

        float leadPhase = 0.0f;
        float leadFreq = 440.0f;
        float leadAmp = 0.0f;

        float padPhase1 = 0.0f;
        float padPhase2 = 0.0f;
        float padPhase3 = 0.0f;
        float padFreq1 = 220.0f;
        float padFreq2 = 261.63f;
        float padFreq3 = 329.63f;
        float padAmp = 0.04f;

        float kickPhase = 0.0f;
        float kickAmp = 0.0f;
        float snareAmp = 0.0f;
        float hatAmp = 0.0f;

        float filterL = 0.0f;
        float filterR = 0.0f;
        float panPhase = 0.0f;

        unsigned int rng = 0xA341316Cu;
    };

    static ProceduralMusicEngine* activeInstance_;

    static constexpr unsigned int kSampleRate = 44100;
    static constexpr unsigned int kChannels = 2;
    static constexpr unsigned int kStepsPerBeat = 4;
    static constexpr unsigned int kStepsPerBar = 16;

    AudioStream stream_{};
    CallbackState callbackState_{};
    bool initialized_ = false;
    float smoothedIntensity_ = 0.0f;

    std::atomic<float> intensity_{0.0f};
    std::atomic<float> bpm_{96.0f};
    std::atomic<float> masterVolume_{0.32f};
    std::atomic<int> mode_{0};  // 0=start, 1=playing, 2=win, 3=game over
    std::atomic<bool> muted_{false};

    std::atomic<unsigned int> roundStartEvents_{0};
    std::atomic<unsigned int> paddleHitEvents_{0};
    std::atomic<unsigned int> brickHitEvents_{0};
    std::atomic<unsigned int> lifeLossEvents_{0};
    std::atomic<unsigned int> winEvents_{0};
    std::atomic<unsigned int> gameOverEvents_{0};
};

}  // namespace breakout
