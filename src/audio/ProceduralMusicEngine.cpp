#include "audio/ProceduralMusicEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace breakout {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * Clamp01(t);
}

float MidiToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

void AdvancePhase(float& phase, float frequency, float sampleRate) {
    phase += frequency / sampleRate;
    if (phase >= 1.0f) {
        phase -= std::floor(phase);
    }
}

float SoftClip(float sample) {
    return sample / (1.0f + std::fabs(sample));
}

float NextWhiteNoise(unsigned int& state) {
    state = state * 1664525u + 1013904223u;
    return (static_cast<float>((state >> 9) & 0x007FFFFFu) / 4194303.5f) - 1.0f;
}

bool IsEuclideanHit(unsigned int step, unsigned int pulses, unsigned int steps, unsigned int rotation) {
    const unsigned int rotated = (step + rotation) % steps;
    const unsigned int previous = (rotated * pulses) / steps;
    const unsigned int next = ((rotated + 1) * pulses) / steps;
    return next != previous;
}

int RootMidiForBar(unsigned int bar) {
    static constexpr std::array<int, 4> progression = {45, 43, 41, 48};  // A2 G2 F2 C3
    return progression[bar % progression.size()];
}

float ComputeSamplesPerStep(float bpm) {
    const float clampedBpm = std::max(40.0f, bpm);
    return 44100.0f * 60.0f / (clampedBpm * 4.0f);
}

}  // namespace

ProceduralMusicEngine* ProceduralMusicEngine::activeInstance_ = nullptr;

bool ProceduralMusicEngine::Init() {
    if (initialized_) {
        return true;
    }

    SetAudioStreamBufferSizeDefault(2048);
    stream_ = LoadAudioStream(kSampleRate, 32, kChannels);

    if (!IsAudioStreamValid(stream_)) {
        return false;
    }

    callbackState_.samplesUntilStep = ComputeSamplesPerStep(bpm_.load(std::memory_order_relaxed));
    activeInstance_ = this;

    SetAudioStreamCallback(stream_, AudioCallback);
    PlayAudioStream(stream_);

    initialized_ = true;
    return true;
}

void ProceduralMusicEngine::Shutdown() {
    if (!initialized_) {
        return;
    }

    StopAudioStream(stream_);
    UnloadAudioStream(stream_);

    if (activeInstance_ == this) {
        activeInstance_ = nullptr;
    }

    initialized_ = false;
}

void ProceduralMusicEngine::UpdateFromGame(const GameSession& session, const GameConfig& config) {
    const float speed = std::sqrt(session.ballSpeed.x * session.ballSpeed.x + session.ballSpeed.y * session.ballSpeed.y);
    const float speedNorm = Clamp01((speed - 4.0f) / 4.5f);

    int activeBricks = 0;
    for (const Brick& brick : session.bricks) {
        if (brick.active) {
            ++activeBricks;
        }
    }

    const float totalBricks = static_cast<float>(std::max(1, config.brickLayout.rows * config.brickLayout.columns));
    const float brickRatio = static_cast<float>(activeBricks) / totalBricks;
    const float progressNorm = 1.0f - brickRatio;
    const float danger = (session.lives <= 1) ? 1.0f : ((session.lives == 2) ? 0.45f : 0.0f);

    float targetIntensity = 0.45f * speedNorm + 0.35f * progressNorm + 0.20f * danger;
    targetIntensity = Clamp01(targetIntensity);

    int mode = 0;
    switch (session.state) {
        case GameState::Start:
            mode = 0;
            targetIntensity *= 0.35f;
            break;
        case GameState::Playing:
            mode = 1;
            break;
        case GameState::Win:
            mode = 2;
            targetIntensity = 0.78f;
            break;
        case GameState::GameOver:
            mode = 3;
            targetIntensity = 0.12f;
            break;
    }

    smoothedIntensity_ += (targetIntensity - smoothedIntensity_) * 0.08f;

    float targetBpm = 92.0f;
    if (mode == 1) {
        targetBpm = Lerp(95.0f, 162.0f, smoothedIntensity_);
    } else if (mode == 2) {
        targetBpm = 122.0f;
    } else if (mode == 3) {
        targetBpm = 70.0f;
    }

    intensity_.store(smoothedIntensity_, std::memory_order_relaxed);
    bpm_.store(targetBpm, std::memory_order_relaxed);
    mode_.store(mode, std::memory_order_relaxed);
    masterVolume_.store(0.32f, std::memory_order_relaxed);
}

void ProceduralMusicEngine::OnRoundStart() {
    PushEvent(roundStartEvents_);
}

void ProceduralMusicEngine::OnPaddleHit() {
    PushEvent(paddleHitEvents_);
}

void ProceduralMusicEngine::OnBrickHit() {
    PushEvent(brickHitEvents_);
}

void ProceduralMusicEngine::OnLifeLost() {
    PushEvent(lifeLossEvents_);
}

void ProceduralMusicEngine::OnWin() {
    PushEvent(winEvents_);
}

void ProceduralMusicEngine::OnGameOver() {
    PushEvent(gameOverEvents_);
}

void ProceduralMusicEngine::ToggleMute() {
    muted_.store(!muted_.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

bool ProceduralMusicEngine::IsMuted() const {
    return muted_.load(std::memory_order_relaxed);
}

void ProceduralMusicEngine::PushEvent(std::atomic<unsigned int>& counter) {
    const unsigned int current = counter.load(std::memory_order_relaxed);
    if (current < 64u) {
        counter.fetch_add(1u, std::memory_order_relaxed);
    }
}

void ProceduralMusicEngine::AudioCallback(void* bufferData, unsigned int frames) {
    float* output = static_cast<float*>(bufferData);

    if (output == nullptr) {
        return;
    }

    if (activeInstance_ == nullptr) {
        std::memset(output, 0, frames * kChannels * sizeof(float));
        return;
    }

    activeInstance_->RenderAudio(output, frames);
}

void ProceduralMusicEngine::RenderAudio(float* output, unsigned int frames) {
    CallbackState& state = callbackState_;

    const float intensity = intensity_.load(std::memory_order_relaxed);
    const float bpm = bpm_.load(std::memory_order_relaxed);
    const float masterVolume = masterVolume_.load(std::memory_order_relaxed);
    const int mode = mode_.load(std::memory_order_relaxed);
    const bool muted = muted_.load(std::memory_order_relaxed);

    const unsigned int roundStarts = roundStartEvents_.exchange(0u, std::memory_order_relaxed);
    const unsigned int paddleHits = paddleHitEvents_.exchange(0u, std::memory_order_relaxed);
    const unsigned int brickHits = brickHitEvents_.exchange(0u, std::memory_order_relaxed);
    const unsigned int lifeLosses = lifeLossEvents_.exchange(0u, std::memory_order_relaxed);
    const unsigned int wins = winEvents_.exchange(0u, std::memory_order_relaxed);
    const unsigned int gameOvers = gameOverEvents_.exchange(0u, std::memory_order_relaxed);

    const int root = RootMidiForBar(state.bar);
    static constexpr std::array<int, 8> melodicScale = {0, 2, 3, 5, 7, 8, 10, 12};

    if (roundStarts > 0) {
        state.kickAmp = std::max(state.kickAmp, 0.75f);
        state.padAmp = std::max(state.padAmp, 0.06f);
        state.bar = 0;
        state.step = 0;
    }

    if (paddleHits > 0) {
        state.leadFreq = MidiToFrequency(root + 7);
        state.leadAmp = std::max(state.leadAmp, 0.16f);
        state.hatAmp = std::max(state.hatAmp, 0.10f);
    }

    if (brickHits > 0) {
        const unsigned int bursts = std::min(brickHits, 8u);
        for (unsigned int i = 0; i < bursts; ++i) {
            const int noteIndex = static_cast<int>((state.rng >> (i % 4)) % melodicScale.size());
            state.leadFreq = MidiToFrequency(root + 12 + melodicScale[noteIndex]);
            state.leadAmp = std::max(state.leadAmp, 0.24f + 0.10f * intensity);
            state.hatAmp = std::max(state.hatAmp, 0.12f);
            state.rng = state.rng * 1103515245u + 12345u;
        }
    }

    if (lifeLosses > 0) {
        state.leadFreq = MidiToFrequency(root - 12);
        state.leadAmp = std::max(state.leadAmp, 0.28f);
        state.snareAmp = std::max(state.snareAmp, 0.75f);
    }

    if (wins > 0) {
        state.leadFreq = MidiToFrequency(root + 19);
        state.leadAmp = std::max(state.leadAmp, 0.42f);
        state.kickAmp = std::max(state.kickAmp, 0.58f);
        state.padAmp = std::max(state.padAmp, 0.10f);
    }

    if (gameOvers > 0) {
        state.snareAmp = std::max(state.snareAmp, 0.85f);
        state.leadFreq = MidiToFrequency(root - 17);
        state.leadAmp = std::max(state.leadAmp, 0.22f);
    }

    auto triggerStep = [&](unsigned int step) {
        const int stepRoot = RootMidiForBar(state.bar);

        if (step == 0) {
            state.padFreq1 = MidiToFrequency(stepRoot + 12);
            state.padFreq2 = MidiToFrequency(stepRoot + 15);
            state.padFreq3 = MidiToFrequency(stepRoot + 19);
        }

        const bool kick = (step == 0) || (step == 8) || ((intensity > 0.65f) && (step == 4));
        const bool snare = (step == 4) || (step == 12);
        const unsigned int hatPulses = (mode == 1) ? ((intensity > 0.7f) ? 11u : 9u) : 6u;
        const bool hat = IsEuclideanHit(step, hatPulses, kStepsPerBar, 1u);
        const bool bass = ((step % 4u) == 0u) || ((mode == 1) && (intensity > 0.75f) && ((step == 2) || (step == 10)));

        if (kick) {
            state.kickAmp = std::max(state.kickAmp, 0.70f + 0.18f * intensity);
        }

        if (snare) {
            state.snareAmp = std::max(state.snareAmp, 0.48f + 0.20f * intensity);
        }

        if (hat) {
            state.hatAmp = std::max(state.hatAmp, 0.10f + 0.09f * intensity);
        }

        if (bass) {
            static constexpr std::array<int, 4> bassIntervals = {0, 3, 7, 10};
            const int interval = bassIntervals[(step / 4u) % bassIntervals.size()];
            state.bassFreq = MidiToFrequency(stepRoot + interval);
            state.bassAmp = std::max(state.bassAmp, 0.18f + 0.22f * intensity);
        }
    };

    const float samplesPerStep = ComputeSamplesPerStep(bpm);
    float cutoff = Lerp(500.0f, 5200.0f, intensity);
    if (mode == 0) {
        cutoff = Lerp(340.0f, 2000.0f, intensity);
    } else if (mode == 3) {
        cutoff = Lerp(260.0f, 1100.0f, intensity);
    }
    const float filterAlpha = 1.0f - std::exp(-2.0f * kPi * cutoff / static_cast<float>(kSampleRate));

    for (unsigned int frame = 0; frame < frames; ++frame) {
        state.samplesUntilStep -= 1.0f;
        while (state.samplesUntilStep <= 0.0f) {
            triggerStep(state.step);
            state.step = (state.step + 1u) % kStepsPerBar;
            if (state.step == 0u) {
                ++state.bar;
            }
            state.samplesUntilStep += samplesPerStep;
        }

        float padTarget = 0.02f;
        if (mode == 1) {
            padTarget = 0.05f + 0.08f * intensity;
        } else if (mode == 2) {
            padTarget = 0.11f;
        } else if (mode == 3) {
            padTarget = 0.015f;
        }
        state.padAmp += (padTarget - state.padAmp) * 0.0022f;

        AdvancePhase(state.bassPhase, state.bassFreq, static_cast<float>(kSampleRate));
        AdvancePhase(state.leadPhase, state.leadFreq, static_cast<float>(kSampleRate));
        AdvancePhase(state.padPhase1, state.padFreq1, static_cast<float>(kSampleRate));
        AdvancePhase(state.padPhase2, state.padFreq2, static_cast<float>(kSampleRate));
        AdvancePhase(state.padPhase3, state.padFreq3, static_cast<float>(kSampleRate));

        const float bassWave = std::sin(kTwoPi * state.bassPhase) + 0.33f * std::sin(2.0f * kTwoPi * state.bassPhase);
        const float bass = state.bassAmp * bassWave;
        state.bassAmp *= 0.99922f;

        const float lead = state.leadAmp * std::sin(kTwoPi * state.leadPhase);
        state.leadAmp *= 0.99865f;

        const float pad = state.padAmp * (0.52f * std::sin(kTwoPi * state.padPhase1) +
                                          0.30f * std::sin(kTwoPi * state.padPhase2) +
                                          0.28f * std::sin(kTwoPi * state.padPhase3));

        const float kickFreq = 55.0f + 115.0f * state.kickAmp;
        AdvancePhase(state.kickPhase, kickFreq, static_cast<float>(kSampleRate));
        const float kick = state.kickAmp * std::sin(kTwoPi * state.kickPhase);
        state.kickAmp *= 0.9925f;

        const float noise = NextWhiteNoise(state.rng);
        const float snare = state.snareAmp * noise * 0.62f;
        state.snareAmp *= 0.964f;

        const float hatNoise = noise - (state.filterL * 0.08f);
        const float hat = state.hatAmp * hatNoise * 0.35f;
        state.hatAmp *= 0.932f;

        const float mixed = bass + lead + pad + kick + snare + hat;
        const float clipped = SoftClip(mixed);

        AdvancePhase(state.panPhase, 0.08f, static_cast<float>(kSampleRate));
        const float pan = 0.5f + 0.5f * std::sin(kTwoPi * state.panPhase);

        float left = clipped * (0.85f - 0.30f * pan);
        float right = clipped * (0.55f + 0.30f * pan);

        state.filterL += filterAlpha * (left - state.filterL);
        state.filterR += filterAlpha * (right - state.filterR);

        const float gain = muted ? 0.0f : masterVolume;
        output[frame * 2 + 0] = SoftClip(state.filterL) * gain;
        output[frame * 2 + 1] = SoftClip(state.filterR) * gain;
    }
}

}  // namespace breakout
