// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLSignalProcessor.h"
#include "MLAudioContext.h"
#include "MLEventsToSignals.h"
#include "mldsp.h"

namespace ml
{

// Utility function: convert MIDI pitch to frequency
// MIDI pitch 69 = A440
inline float pitchToFrequency(float pitch) {
  return 440.0f * powf(2.0f, (pitch - 69.0f) / 12.0f);
}

// Synth: Specialized base class for polyphonic synthesizers
// - Implements processVector() to handle voice iteration
// - Subclasses implement processVoice() for per-voice DSP
// - No knowledge of CLAP, RtAudio, or any specific hosting environment

class Synth : public SignalProcessor {
public:
  static constexpr int kDefaultNumVoices = 8;
  static constexpr int kNumVoices = 8;  // For adapter configuration (can be overridden by subclasses)

  Synth(int numVoices = kDefaultNumVoices)
    : numVoices_(numVoices) {}
  virtual ~Synth() = default;

  // Default implementation of processVector - handles voice iteration
  void processVector(const DSPVectorDynamic& inputs,
                    DSPVectorDynamic& outputs,
                    void* stateData) override {
    auto* audioContext = static_cast<AudioContext*>(stateData);
    if (!audioContext) return;

    // Clear output buffers
    for (int i = 0; i < outputs.size(); ++i) {
      outputs[i] = DSPVector{0.f};
    }

    // Process each voice and mix
    int activeCount = 0;
    for (int v = 0; v < numVoices_; ++v) {
      const auto& voice = audioContext->getInputVoice(v);

      // Check if voice is active (let subclass decide)
      if (isVoiceActive(v, voice)) {
        activeCount++;
        processVoice(v, voice, inputs, outputs, audioContext);
      }
    }

    activeVoiceCount_ = activeCount;
  }

  // Subclasses implement voice processing
  // voiceIndex: which voice (0 to numVoices-1)
  // voice: voice control signals (pitch, gate, velocity, etc.)
  // inputs: audio inputs (may be empty for synths)
  // outputs: audio outputs to mix into (use += to accumulate)
  // audioContext: provides sample rate, timing, etc.
  virtual void processVoice(int voiceIndex,
                           const EventsToSignals::Voice& voice,
                           const DSPVectorDynamic& inputs,
                           DSPVectorDynamic& outputs,
                           AudioContext* audioContext) = 0;

  // Subclasses can override to define when voices are active
  // Default: check gate signal
  virtual bool isVoiceActive(int voiceIndex,
                            const EventsToSignals::Voice& voice) {
    // return voice.outputs.constRow(ml::kGate)[0] > 0.f;
    // For now, just return true. Using the gate here might confuse new devs using an ADSR, since the release phase starts when the gate returns to 0
    return true;
  }

  // Query active voice count (for CLAP sleep/continue)
  int getActiveVoiceCount() const { return activeVoiceCount_; }
  int getNumVoices() const { return numVoices_; }

  // Check if any voices are active (for adapter sleep/continue logic)
  virtual bool hasActiveVoices() const { return activeVoiceCount_ > 0; }

protected:
  int numVoices_;
  int activeVoiceCount_ = 0;
};

} // namespace ml
