#pragma once

#include "MLSignalProcessor.h"
#include "MLAudioContext.h"
#include <clap/clap.h>
#include <sstream>
#include <clap/ext/audio-ports.h>
#include <clap/ext/note-ports.h>
// #include <clap/ext/params.h>  // TODO: Add back when implementing params extension
#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace ml {

// Template parameter: PluginClass = your SignalProcessor-derived class (e.g., ClapSawDemo)
template<typename PluginClass>
class CLAPPluginWrapper : public clap_plugin {
private:
  const clap_host* host;
  const clap_host_log* hostLog;
  std::unique_ptr<PluginClass> processor;
  std::unique_ptr<AudioContext> audioContext;
  const clap_plugin_descriptor* descriptor;

public:
  CLAPPluginWrapper(const clap_host* h, const clap_plugin_descriptor* desc)
    : host(h), descriptor(desc) {

    // Initialize CLAP logging extension
    hostLog = nullptr;
    if (host && host->get_extension) {
      hostLog = static_cast<const clap_host_log*>(host->get_extension(host, CLAP_EXT_LOG));
      fprintf(stderr, "[CLAP WRAPPER] Host log extension: %p\n", hostLog);
      fflush(stderr);
    }

    this->desc = descriptor;
    this->plugin_data = this;

    // Initialize CLAP plugin structure with simple processing
    this->init = [](const clap_plugin* plugin) -> bool {

      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      wrapper->processor = std::make_unique<PluginClass>();

      // AudioContext handles everything - no complex setup needed!
      // Create with 2 inputs and 2 outputs for stereo processing
      // TODO: generalize input/output channels
      wrapper->audioContext = std::make_unique<AudioContext>(2, 2, 48000.0);

      // Need to set polyphony for EventsToSignals
      // TODO: generalize polyphony. Does effect vs instrument matter here?
      wrapper->audioContext->setInputPolyphony(16);

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
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      return wrapper->getExtension(id);
    };

    this->on_main_thread = [](const clap_plugin* plugin) {};
  }

  // CLAP Logging (thread-safe(?) logging to host console)
  void log(clap_log_severity severity, const std::string& message) const {
    if (hostLog && hostLog->log) {
      hostLog->log(host, severity, message.c_str());
    } else {
      // Fallback to standard output if host doesn't support logging
      const char* severityStr = "";
      switch (severity) {
        case CLAP_LOG_DEBUG: severityStr = "DEBUG"; break;
        case CLAP_LOG_INFO: severityStr = "INFO"; break;
        case CLAP_LOG_WARNING: severityStr = "WARNING"; break;
        case CLAP_LOG_ERROR: severityStr = "ERROR"; break;
        default: severityStr = "LOG"; break;
      }
      printf("[CLAP %s] %s\n", severityStr, message.c_str());
    }
  }

  // logging helpers
  void logDebug(const std::string& message) const { log(CLAP_LOG_DEBUG, message); }
  void logInfo(const std::string& message) const { log(CLAP_LOG_INFO, message); }
  void logWarning(const std::string& message) const { log(CLAP_LOG_WARNING, message); }
  void logError(const std::string& message) const { log(CLAP_LOG_ERROR, message); }

  clap_process_status processAudio(const clap_process* process) {
    // Safety checks to prevent crashes
    if (!process || !audioContext || !processor) {
      return CLAP_PROCESS_CONTINUE;
    }

    if (process->audio_inputs_count == 0 || process->audio_outputs_count == 0) {
      return CLAP_PROCESS_CONTINUE;
    }

    // Convert CLAP events to ml::Events - AudioContext handles them
    if (process->in_events) {
      convertCLAPEventsToAudioContext(process->in_events);
    }

    // Get I/O pointers with safety checks (data32 is already float**)
    const float** inputs = nullptr;
    float** outputs = nullptr;
    if (process->audio_inputs && process->audio_inputs[0].data32) {
      inputs = const_cast<const float**>(process->audio_inputs[0].data32);
    }
    if (process->audio_outputs && process->audio_outputs[0].data32) {
      outputs = process->audio_outputs[0].data32;
    }
    if (!outputs) {
      return CLAP_PROCESS_CONTINUE; // Can't process without output buffers
    }

    // AudioContext handles chunking
    for (int i = 0; i < process->frames_count; i += kFloatsPerDSPVector) {
      int samplesThisChunk = std::min(static_cast<int>(kFloatsPerDSPVector), static_cast<int>(process->frames_count - i));

      // Copy inputs to AudioContext (if available)
      if (inputs && process->audio_inputs[0].channel_count > 0) {
        for (int j = 0; j < samplesThisChunk; ++j) {
          audioContext->inputs[0][j] = inputs[0][i + j];
          audioContext->inputs[1][j] = (process->audio_inputs[0].channel_count > 1) ?
                                      inputs[1][i + j] : inputs[0][i + j];  // Mono to stereo
        }
      } else {
        // Clear inputs if no input available
        for (int j = 0; j < samplesThisChunk; ++j) {
          audioContext->inputs[0][j] = 0.0f;
          audioContext->inputs[1][j] = 0.0f;
        }
      }

      // AudioContext processes everything (events, voices, timing)
      audioContext->processVector(i);

      // User processor just does DSP on processed context
      processor->processAudioContext();

      // Copy outputs from AudioContext to CLAP buffers
      if (process->audio_outputs[0].channel_count >= 1) {
        for (int j = 0; j < samplesThisChunk; ++j) {
          outputs[0][i + j] = audioContext->outputs[0][j];
          if (process->audio_outputs[0].channel_count >= 2) {
            outputs[1][i + j] = audioContext->outputs[1][j];
          }
        }
      }
    }

    return processor->hasActiveVoices() ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_SLEEP;
  }

  // Extension implementation
  const void* getExtension(const char* id) {
    if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &audioPortsExt;
    if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &notePortsExt;
    if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &paramsExt;
    if (strcmp(id, CLAP_EXT_STATE) == 0) return &stateExt;
    return nullptr;
  }

  // Audio Ports Extension - Essential for audio I/O
  static uint32_t audioPortsCount(const clap_plugin* plugin, bool is_input) {
    return 1; // One stereo input, one stereo output
  }

  static bool audioPortsGet(const clap_plugin* plugin, uint32_t index, bool is_input, clap_audio_port_info* info) {
    if (index != 0) return false;

    info->id = 0;
    snprintf(info->name, sizeof(info->name), "%s", is_input ? "Audio Input" : "Audio Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;

    // in-place processing: allow the host to use the same buffer for input and output
    // if supported set the pair port id.
    // if not supported set to CLAP_INVALID_ID
    info->in_place_pair = 0;
    return true;
  }

  static const clap_plugin_audio_ports audioPortsExt;

  // Note Ports Extension - Essential for MIDI input
  static uint32_t notePortsCount(const clap_plugin* plugin, bool is_input) {
    return is_input ? 1 : 0; // One MIDI input, no MIDI output
  }

  static bool notePortsGet(const clap_plugin* plugin, uint32_t index, bool is_input, clap_note_port_info* info) {
    if (!is_input || index != 0) return false;

    info->id = 0;
    snprintf(info->name, sizeof(info->name), "MIDI Input");
    info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    return true;
  }

  static const clap_plugin_note_ports notePortsExt;

  // Params Extension - integrate with ParameterTree

  static uint32_t paramsCount(const clap_plugin* plugin) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor) return 0;
    return wrapper->processor->getParameterCount();
  }

  static bool paramsInfo(const clap_plugin* plugin, uint32_t index, clap_param_info* info) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !info) return false;

    const auto& descriptions = wrapper->processor->getParameterTree().descriptions;
    uint32_t currentIndex = 0;

    for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
      if (currentIndex == index) {
        const auto& paramDesc = *it;
        if (!paramDesc) return false;

        auto paramName = paramDesc->getTextProperty("name");
        auto range = paramDesc->getMatrixPropertyWithDefault("range", ml::Matrix{0.0f, 1.0f});
        auto defaultVal = paramDesc->getFloatPropertyWithDefault("default", 0.5f);

        info->id = index;
        strncpy(info->name, paramName.getText(), CLAP_NAME_SIZE - 1);
        info->name[CLAP_NAME_SIZE - 1] = '\0';
        info->min_value = range[0];
        info->max_value = range[1];
        info->default_value = ml::clamp(defaultVal, (float)info->min_value, (float)info->max_value);
        info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        info->module[0] = '\0';

        return true;
      }
      currentIndex++;
    }

    return false;
  }

  static bool paramsValue(const clap_plugin* plugin, clap_id param_id, double* value) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !value) return false;

    const auto& descriptions = wrapper->processor->getParameterTree().descriptions;
    uint32_t currentIndex = 0;

    for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
      if (currentIndex == param_id) {
        const auto& paramDesc = *it;
        if (paramDesc) {
          auto paramName = paramDesc->getTextProperty("name");
          *value = wrapper->processor->getNormalizedFloatParam(paramName);
          return true;
        }
      }
      currentIndex++;
    }

    return false;
  }

  static bool paramsValueToText(const clap_plugin* plugin, clap_id param_id, double value,
                               char* out_buffer, uint32_t out_buffer_capacity) {
    if (!out_buffer) return false;
    snprintf(out_buffer, out_buffer_capacity, "%.3f", value);
    return true;
  }

  static bool paramsTextToValue(const clap_plugin* plugin, clap_id param_id,
                               const char* param_value_text, double* out_value) {
    if (!param_value_text || !out_value) return false;
    try {
      *out_value = std::stod(param_value_text);
      return true;
    } catch (const std::exception&) {
      return false;
    }
  }

  static void paramsFlush(const clap_plugin* plugin, const clap_input_events* in,
                         const clap_output_events* out) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !in) return;

    uint32_t eventCount = in->size(in);
    if (eventCount > 0) {
      std::ostringstream logMsg;
      logMsg << "paramsFlush: Processing " << eventCount << " parameter events";
      wrapper->logInfo(logMsg.str());
    }

    for (uint32_t i = 0; i < eventCount; ++i) {
      const clap_event_header* header = in->get(in, i);
      if (!header) continue;

      if (header->type == CLAP_EVENT_PARAM_VALUE) {
        const auto* paramEvent = reinterpret_cast<const clap_event_param_value*>(header);

        const auto& descriptions = wrapper->processor->getParameterTree().descriptions;
        uint32_t currentIndex = 0;

        for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
          if (currentIndex == paramEvent->param_id) {
            const auto& paramDesc = *it;
            if (paramDesc) {
              auto paramName = paramDesc->getTextProperty("name");
              wrapper->processor->setParamFromNormalizedValue(paramName, paramEvent->value);
              break;
            }
          }
          currentIndex++;
        }
      }
    }
  }

  static const clap_plugin_params paramsExt;

  // State Extension - Save/load plugin state

  static bool stateSave(const clap_plugin* plugin, const clap_ostream* stream) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !stream) return false;

    try {
      // Get normalized parameter values from ParameterTree
      const auto& paramValues = wrapper->processor->getParameterTree().getNormalizedValues();

      // Simple JSON serialization - more robust than binary for now
      std::string jsonData = "{";
      bool first = true;

      for (auto it = paramValues.begin(); it != paramValues.end(); ++it) {
        if (!first) jsonData += ",";
        first = false;

        ml::Path paramPath = it.getCurrentPath();
        ml::Value paramValue = *it;

                 jsonData += "\"" + std::string(pathToText(paramPath).getText()) + "\":" + std::to_string(paramValue.getFloatValue());
      }
      jsonData += "}";

      // Write to CLAP stream
      int64_t bytesWritten = stream->write(stream, jsonData.c_str(), jsonData.length());
      return bytesWritten == static_cast<int64_t>(jsonData.length());

    } catch (const std::exception&) {
      return false;
    }
  }

  static bool stateLoad(const clap_plugin* plugin, const clap_istream* stream) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !stream) return false;

    try {
      // Read all available data from stream
      std::string jsonData;
      char buffer[4096];

      int64_t bytesRead;
      while ((bytesRead = stream->read(stream, buffer, sizeof(buffer))) > 0) {
        jsonData.append(buffer, bytesRead);
      }

      if (jsonData.empty()) return false;

      // Simple JSON parsing - just look for "param":value patterns
      // This is a minimal implementation that handles our simple JSON format
      size_t pos = 0;
      while ((pos = jsonData.find("\"", pos)) != std::string::npos) {
        size_t nameStart = pos + 1;
        size_t nameEnd = jsonData.find("\"", nameStart);
        if (nameEnd == std::string::npos) break;

        std::string paramName = jsonData.substr(nameStart, nameEnd - nameStart);

        size_t colonPos = jsonData.find(":", nameEnd);
        if (colonPos == std::string::npos) break;

        size_t valueStart = colonPos + 1;
        size_t valueEnd = jsonData.find_first_of(",}", valueStart);
        if (valueEnd == std::string::npos) break;

        std::string valueStr = jsonData.substr(valueStart, valueEnd - valueStart);
        float value = std::stof(valueStr);

                 // Set the parameter value
         wrapper->processor->setParamFromNormalizedValue(ml::Path(ml::TextFragment(paramName.c_str())), value);

        pos = valueEnd;
      }

      return true;

    } catch (const std::exception&) {
      return false;
    }
  }

  static const clap_plugin_state stateExt;

private:
  void convertCLAPEventsToAudioContext(const clap_input_events* events) {
    // Ultra-simple: AudioContext handles all event complexity
    audioContext->clearInputEvents();

    for (uint32_t i = 0; i < events->size(events); ++i) {
      const clap_event_header* header = events->get(events, i);

      // std::ostringstream logMsg;
      // logMsg << "(" << header->time << ")" << " processing event type: " << header->type;
      // logDebug(logMsg.str());

      ml::Event mlEvent;
      mlEvent.time = header->time;

      switch (header->type) {
        case CLAP_EVENT_NOTE_ON: {
          auto* noteEvent = reinterpret_cast<const clap_event_note*>(header);
          mlEvent.type = ml::kNoteOn;
          mlEvent.channel = noteEvent->channel;
          mlEvent.sourceIdx = noteEvent->key;
          mlEvent.value1 = noteEvent->key;        // MIDI note number
          mlEvent.value2 = noteEvent->velocity;   // MIDI velocity
          break;
        }
        case CLAP_EVENT_NOTE_OFF: {
          auto* noteEvent = reinterpret_cast<const clap_event_note*>(header);
          mlEvent.type = ml::kNoteOff;
          mlEvent.channel = noteEvent->channel;
          mlEvent.sourceIdx = noteEvent->key;
          mlEvent.value1 = noteEvent->key;
          mlEvent.value2 = noteEvent->velocity;
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

// Export macro - generates complete CLAP plugin from a SignalProcessor
  // TODO: generalize CLAP_PLUGN_FEATURE_*
  // TODO: read features and descriptors from yml or json; anything to take from `python clone_plugin.py`?
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
    if (!host) { \
      return nullptr; \
    } \
    if (!clap_version_is_compatible(host->clap_version)) { \
      return nullptr; \
    } \
    if (!plugin_id) { \
      return nullptr; \
    } \
    if (strcmp(plugin_id, desc.id) != 0) { \
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

// Static extension structure definitions
template<typename PluginClass>
const clap_plugin_audio_ports CLAPPluginWrapper<PluginClass>::audioPortsExt = {
  CLAPPluginWrapper<PluginClass>::audioPortsCount,
  CLAPPluginWrapper<PluginClass>::audioPortsGet
};

template<typename PluginClass>
const clap_plugin_note_ports CLAPPluginWrapper<PluginClass>::notePortsExt = {
  CLAPPluginWrapper<PluginClass>::notePortsCount,
  CLAPPluginWrapper<PluginClass>::notePortsGet
};

template<typename PluginClass>
const clap_plugin_params CLAPPluginWrapper<PluginClass>::paramsExt = {
  CLAPPluginWrapper<PluginClass>::paramsCount,
  CLAPPluginWrapper<PluginClass>::paramsInfo,
  CLAPPluginWrapper<PluginClass>::paramsValue,
  CLAPPluginWrapper<PluginClass>::paramsValueToText,
  CLAPPluginWrapper<PluginClass>::paramsTextToValue,
  CLAPPluginWrapper<PluginClass>::paramsFlush
};

template<typename PluginClass>
const clap_plugin_state CLAPPluginWrapper<PluginClass>::stateExt = {
  CLAPPluginWrapper<PluginClass>::stateSave,
  CLAPPluginWrapper<PluginClass>::stateLoad
};

} // namespace ml
