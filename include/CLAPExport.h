#pragma once

#include "MLSignalProcessor.h"
#include "MLAudioContext.h"
#include <clap/clap.h>
#include <algorithm>
#include <memory>

namespace ml {

// Ultra-simple wrapper - AudioContext handles everything!
// Template parameter: PluginClass = your SignalProcessor-derived class (e.g., ClapSawDemo)
template<typename PluginClass>
class CLAPPluginWrapper : public clap_plugin {
private:
  const clap_host* host;
  std::unique_ptr<PluginClass> processor;
  std::unique_ptr<AudioContext> audioContext;
  const clap_plugin_descriptor* descriptor;

public:
  CLAPPluginWrapper(const clap_host* h, const clap_plugin_descriptor* desc)
    : host(h), descriptor(desc) {

    // Initialize CLAP plugin structure with ultra-simple processing
    this->desc = descriptor;
    this->plugin_data = this;
    this->init = [](const clap_plugin* plugin) -> bool {
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      wrapper->processor = std::make_unique<PluginClass>();
      // AudioContext handles everything - no complex setup needed!
      wrapper->audioContext = std::make_unique<AudioContext>(0, 2, 48000.0);
      wrapper->processor->setAudioContext(wrapper->audioContext.get());
      return true;
    };
    this->destroy = [](const clap_plugin* plugin) {
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      delete wrapper;
    };
    this->activate = [](const clap_plugin* plugin, double sr, uint32_t min, uint32_t max) -> bool {
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      wrapper->audioContext->setSampleRate(sr);
      wrapper->processor->setSampleRate(sr);
      return true;
    };
    this->deactivate = [](const clap_plugin* plugin) {};
    this->start_processing = [](const clap_plugin* plugin) -> bool { return true; };
    this->stop_processing = [](const clap_plugin* plugin) {};
    this->reset = [](const clap_plugin* plugin) {};
    this->process = [](const clap_plugin* plugin, const clap_process* process) -> clap_process_status {
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      return wrapper->processAudio(process);
    };
    this->get_extension = [](const clap_plugin* plugin, const char* id) -> const void* {
      return nullptr;  // Simplified for now
    };
    this->on_main_thread = [](const clap_plugin* plugin) {};
  }

  clap_process_status processAudio(const clap_process* process) {
    // Ultra-simple: AudioContext handles chunking, events, everything!

    // Convert CLAP events to ml::Events - AudioContext handles them
    convertCLAPEventsToAudioContext(process->in_events);

    // Cast I/O pointers
    const float** inputs = const_cast<const float**>(
      reinterpret_cast<float**>(process->audio_inputs[0].data32));
    float** outputs = reinterpret_cast<float**>(process->audio_outputs[0].data32);

    // Ultra-simple processing loop - AudioContext handles chunking
    for (int i = 0; i < process->frames_count; i += kFloatsPerDSPVector) {
      int samplesThisChunk = std::min(static_cast<int>(kFloatsPerDSPVector), static_cast<int>(process->frames_count - i));

      // Copy inputs to AudioContext
      if (process->audio_inputs[0].channel_count > 0) {
        for (int j = 0; j < samplesThisChunk; ++j) {
          audioContext->inputs[0][j] = inputs[0][i + j];
          audioContext->inputs[1][j] = (process->audio_inputs[0].channel_count > 1) ?
                                      inputs[1][i + j] : inputs[0][i + j];
        }
      }

      // AudioContext processes everything (events, voices, timing)
      audioContext->processVector(i);

      // User processor just does DSP on processed context
      processor->processAudioContext();

      // Copy outputs from AudioContext
      for (int j = 0; j < samplesThisChunk; ++j) {
        outputs[0][i + j] = audioContext->outputs[0][j];
        outputs[1][i + j] = audioContext->outputs[1][j];
      }
    }

    return processor->hasActiveVoices() ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_SLEEP;
  }

private:
  void convertCLAPEventsToAudioContext(const clap_input_events* events) {
    // Ultra-simple: AudioContext handles all event complexity
    audioContext->clearInputEvents();

    for (uint32_t i = 0; i < events->size(events); ++i) {
      const clap_event_header* header = events->get(events, i);

      ml::Event mlEvent;
      mlEvent.time = header->time;

      switch (header->type) {
        case CLAP_EVENT_NOTE_ON: {
          auto* noteEvent = reinterpret_cast<const clap_event_note*>(header);
          mlEvent.type = ml::kNoteOn;
          mlEvent.channel = noteEvent->channel;
          mlEvent.sourceIdx = noteEvent->key;
          mlEvent.value1 = noteEvent->velocity;
          mlEvent.value2 = noteEvent->note_id;
          break;
        }
        case CLAP_EVENT_NOTE_OFF: {
          auto* noteEvent = reinterpret_cast<const clap_event_note*>(header);
          mlEvent.type = ml::kNoteOff;
          mlEvent.channel = noteEvent->channel;
          mlEvent.sourceIdx = noteEvent->key;
          mlEvent.value1 = noteEvent->velocity;
          mlEvent.value2 = noteEvent->note_id;
          break;
        }
        case CLAP_EVENT_PARAM_VALUE: {
          auto* paramEvent = reinterpret_cast<const clap_event_param_value*>(header);
          mlEvent.type = ml::kController;
          mlEvent.sourceIdx = paramEvent->param_id;
          mlEvent.value1 = paramEvent->value;
          break;
        }
        default:
          continue;  // Skip unknown events
      }

      // AudioContext handles all event processing internally!
      audioContext->addInputEvent(mlEvent);
    }
  }
};

// Export macro - generates complete CLAP plugin from ultra-simple SignalProcessor
// Parameters:
//   ClassName = your SignalProcessor-derived class (e.g., ClapSawDemo)
//   PluginName = display name for DAW (e.g., "Clap Saw Demo")
//   VendorName = company name (e.g., "Madrona Labs")
// Usage: MADRONALIB_EXPORT_CLAP_PLUGIN(ClapSawDemo, "Clap Saw Demo", "Madrona Labs")
#define MADRONALIB_EXPORT_CLAP_PLUGIN(ClassName, PluginName, VendorName) \
  extern "C" { \
    static const char* const features[] = { \
      CLAP_PLUGIN_FEATURE_INSTRUMENT, \
      CLAP_PLUGIN_FEATURE_SYNTHESIZER, \
      nullptr \
    }; \
    \
    static const clap_plugin_descriptor desc = { \
      CLAP_VERSION_INIT, \
      PluginName "-id", \
      PluginName, \
      VendorName, \
      "https://madronalabs.com", \
      "", \
      "", \
      "1.0.0", \
      "Synthesizer", \
      features \
    }; \
    \
    static const clap_plugin* plugin_create(const clap_plugin_factory* factory, const clap_host* host, const char* plugin_id) { \
      if (!clap_version_is_compatible(host->clap_version)) { \
        return nullptr; \
      } \
      if (!plugin_id || strcmp(plugin_id, desc.id) != 0) { \
        return nullptr; \
      } \
      return new ml::CLAPPluginWrapper<ClassName>(host, &desc); \
    } \
    \
    static const clap_plugin_factory plugin_factory = { \
      [](const clap_plugin_factory* factory) -> uint32_t { \
        return 1; \
      }, \
      [](const clap_plugin_factory* factory, uint32_t index) -> const clap_plugin_descriptor* { \
        return index == 0 ? &desc : nullptr; \
      }, \
      plugin_create \
    }; \
    \
    const CLAP_EXPORT clap_plugin_entry clap_entry = { \
      CLAP_VERSION_INIT, \
      [](const char* path) -> bool { return true; }, \
      []() {}, \
      [](const char* factory_id) -> const void* { \
        return strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &plugin_factory : nullptr; \
      } \
    }; \
  }

} // namespace ml
