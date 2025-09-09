#pragma once

#include "MLSignalProcessor.h"
#include "MLAudioContext.h"

#include <clap/clap.h>
#include <clap/ext/log.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/note-ports.h>
#include <clap/ext/params.h>
#include <clap/ext/state.h>
#include <clap/ext/gui.h>
#include "../external/cJSON/cJSON.h"

#ifdef HAS_GUI
#include "MLPlatformView.h"
#include "MLAppView.h"
#include "MLDialBasic.h"
#include "MLTextLabelBasic.h"
#include "MLWidget.h"
#include "MLDrawContext.h"
#include "MLResizer.h"
#endif

#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <mutex>

namespace ml {

// Generic base class for CLAP SignalProcessors - handles all CLAP-specific boilerplate
template <typename BaseProcessorClass = ml::SignalProcessor>
class CLAPSignalProcessor : public BaseProcessorClass {
protected:
  // CLAP callback storage
  std::function<void(int, const char*)> hostLogCallback;
  std::function<void()> hostParameterFlushCallback;
  std::function<void(int32_t, double)> parameterChangedCallback;

public:
  template<typename... Args>
  CLAPSignalProcessor(Args&&... args) : BaseProcessorClass(std::forward<Args>(args)...) {}
  virtual ~CLAPSignalProcessor() = default;

  // Generic CLAP interface methods (no plugin-specific code)
  uint32_t getParameterCount() const { return this->_params.descriptions.size(); }

  // CLAP logging interface
  void setHostLogCallback(std::function<void(int, const char*)> callback) {
    hostLogCallback = callback;
    if (hostLogCallback) {
      logToHost(CLAP_LOG_INFO, "CLAPSignalProcessor: Host logging callback initialized successfully");
    }
  }

  void logToHost(int severity, const char* message) {
    if (hostLogCallback) {
      hostLogCallback(severity, message);
    }
  }

  // CLAP parameter flush interface
  void setHostParameterFlushCallback(std::function<void()> callback) {
    hostParameterFlushCallback = callback;
    logToHost(CLAP_LOG_INFO, "Host parameter flush callback set");
  }

  void requestHostParameterFlush() {
    if (hostParameterFlushCallback) {
      logToHost(CLAP_LOG_INFO, "Calling host parameter flush callback");
      hostParameterFlushCallback();
    } else {
      logToHost(CLAP_LOG_WARNING, "No host parameter flush callback available");
    }
  }

  // Generic parameter ID lookup by name
  int32_t getParameterIdByName(const std::string& name) const {
    const auto& descriptions = this->_params.descriptions;
    int32_t currentIndex = 0;

    for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
      const auto& paramDesc = *it;
      if (paramDesc) {
        std::string paramName = std::string(paramDesc->getTextProperty("name").getText());
        if (paramName == name) {
          return currentIndex;
        }
      }
      currentIndex++;
    }
    return -1; // Not found
  }

  // CLAP parameter change tracking
  void setParameterChangedCallback(std::function<void(int32_t, double)> callback) {
    parameterChangedCallback = callback;
    logToHost(CLAP_LOG_INFO, "Parameter changed callback set");
  }

  void notifyParameterChanged(const std::string& name, double realValue) {
    int32_t paramId = getParameterIdByName(name);
    if (paramId >= 0 && parameterChangedCallback) {
      std::string logMsg = "Notifying wrapper: param " + name + " (id=" + std::to_string(paramId) +
                          ") changed to " + std::to_string(realValue);
      logToHost(CLAP_LOG_INFO, logMsg.c_str());
      parameterChangedCallback(paramId, realValue);
    }
  }

  // CLAP lifecycle methods
  virtual void setSampleRate(double sr) {}
  
  // Default voice activity - plugins can override
  virtual bool hasActiveVoices() const { return false; }
};

#ifdef HAS_GUI
// Generic base class for CLAP AppViews - handles all common functionality
template <typename ProcessorClass>
class CLAPAppView : public ml::AppView {
protected:
  ProcessorClass* processor;

public:
  CLAPAppView(const std::string& name, ProcessorClass* proc)
    : ml::AppView(name.c_str(), 1), processor(proc) {

    // Set up default grid system
    setGridSizeDefault(60);
    setGridSizeLimits(30, 120);
    setFixedAspectRatio({10, 4});  // Default 10x4 layout
  }

  virtual ~CLAPAppView() = default;

  // Plugin-specific methods that subclasses MUST implement
  virtual void makeWidgets() = 0;
  virtual void initializeResources(NativeDrawContext* nvg) override = 0;

  // Generic parameter connection (handles all the boilerplate)
  void connectParameters() {
    if (!processor) return;

    // Convert ParameterTree to ParameterDescriptionList for _setupWidgets
    ml::ParameterDescriptionList pdl;
    for (const auto& paramDesc : processor->getParameterTree().descriptions) {
      pdl.push_back(std::make_unique<ml::ParameterDescription>(*paramDesc));
    }

    // Set all widgets visible
    ml::forEach<ml::Widget>(_view->_widgets, [&](ml::Widget& w) {
      w.setProperty("visible", true);
    });

    // Connect widgets to parameters
    _setupWidgets(pdl);

    // Initial sync: Update widgets with current processor values
    for (const auto& paramDesc : processor->getParameterTree().descriptions) {
      std::string paramName = std::string(paramDesc->getTextProperty("name").getText());
      float normalizedValue = processor->getNormalizedFloatParam(ml::Path(paramName.c_str()));

      ml::Path msgPath = ml::Path("set_param", paramName.c_str());
      ml::Message msg{msgPath, normalizedValue};
      msg.flags |= ml::kMsgFromController;  // Prevent echo back
      enqueueMessage(msg);
    }
  }

  // Generic parameter message handling
  void onMessage(Message msg) override {
    if (processor && msg.address) {
      ml::Path addr = msg.address;

      // Handle parameter changes from widgets
      if (addr.getSize() > 2 && second(addr) == "set_param") {
        ml::Path paramName = tail(tail(addr));
        float normalizedValue = msg.value.getFloatValue();
        std::string paramNameStr = std::string(pathToText(paramName).getText());

        // Update processor parameter
        processor->setParamFromNormalizedValue(paramNameStr.c_str(), normalizedValue);

        // Get real value for host notification
        float realValue = processor->getRealFloatParam(ml::Path(paramNameStr.c_str()));

        // Notify wrapper about parameter change (for GUI→Host sync)
        processor->notifyParameterChanged(paramNameStr, realValue);

        // Request host parameter flush
        processor->requestHostParameterFlush();
      }
    }

    AppView::onMessage(msg);
  }

  // Force event processing for CLAP plugins
  void animate(NativeDrawContext* nvg) override {
    // CRITICAL: Force event processing for CLAP plugins
    _handleGUIEvents();

    AppView::animate(nvg);
  }

  // Generic widget layout
  void layoutView(DrawContext dc) override {
    ml::forEach<ml::Widget>(_view->_widgets, [&](ml::Widget& w) {
      w.resize(dc);
    });
  }

  // Default render - subclasses can override if needed
  void render(NativeDrawContext* nvg) override {
    AppView::render(nvg);
  }

  // Default clear resources - subclasses can override if needed
  void clearResources() override {
    // Default implementation does nothing
  }

  // Default GUI event handling - subclasses can override if needed
  void onGUIEvent(const GUIEvent& event) override {
    // Default implementation does nothing
  }



  // Default resize handling - subclasses can override if needed
  void onResize(Vec2 newSize) override {
    // Trigger a redraw when resized
    setDirty(true);
  }
};
#endif

// Template parameters:
// PluginClass = your SignalProcessor-derived class (e.g., ClapSawDemo)
// GUIClass = your AppView-derived class (e.g., ClapSawDemoGUI) - use void for no GUI
template<typename PluginClass, typename GUIClass = void>
class CLAPPluginWrapper : public clap_plugin {
private:
  const clap_host* host;
  const clap_host_log* hostLog;
  const clap_host_params* hostParams;  // For GUI->Host parameter notifications
  std::unique_ptr<PluginClass> processor;
  std::unique_ptr<AudioContext> audioContext;
  const clap_plugin_descriptor* descriptor;

#ifdef HAS_GUI
  std::unique_ptr<GUIClass> guiInstance;
  std::unique_ptr<PlatformView> platformView;
  uint32_t guiWidth = 400;
  uint32_t guiHeight = 300;
  bool guiCreated = false;
  bool widgetsCreated = false;  // Per-instance widget creation flag
#endif

public:
  CLAPPluginWrapper(const clap_host* h, const clap_plugin_descriptor* desc)
    : host(h), descriptor(desc) {

    // Initialize CLAP extensions
    hostLog = nullptr;
    hostParams = nullptr;
    if (host && host->get_extension) {
      hostLog = static_cast<const clap_host_log*>(host->get_extension(host, CLAP_EXT_LOG));
      hostParams = static_cast<const clap_host_params*>(host->get_extension(host, CLAP_EXT_PARAMS));
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

#ifdef HAS_GUI
      // Set up logging callback for GUI debugging
      wrapper->processor->setHostLogCallback([wrapper](int severity, const char* message) {
        wrapper->log(static_cast<clap_log_severity>(severity), std::string(message));
      });
#endif

      wrapper->logInfo("CLAPPluginWrapper: Plugin initialized successfully");
      wrapper->logInfo("CLAPPluginWrapper: State extension available: " + std::string(wrapper->getExtension(CLAP_EXT_STATE) ? "YES" : "NO"));
      wrapper->logInfo("CLAPPluginWrapper: Parameters extension available: " + std::string(wrapper->getExtension(CLAP_EXT_PARAMS) ? "YES" : "NO"));

      // Set up parameter flush callback for GUI->Host sync
      wrapper->processor->setHostParameterFlushCallback([wrapper]() {
        wrapper->requestHostParameterFlush();
      });

      // Set up parameter changed callback for GUI->Host sync
      wrapper->processor->setParameterChangedCallback([wrapper](int32_t paramId, double realValue) {
        wrapper->markParameterChanged(paramId, realValue);
      });

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

  // CLAP logging (thread-safe?) to host console
  void log(clap_log_severity severity, const std::string& message) const {
    if (hostLog && hostLog->log) {
      hostLog->log(host, severity, message.c_str());
    }
  }

  // logging helpers
  void logDebug(const std::string& message) const { log(CLAP_LOG_DEBUG, message); }
  void logInfo(const std::string& message) const { log(CLAP_LOG_INFO, message); }
  void logWarning(const std::string& message) const { log(CLAP_LOG_WARNING, message); }
  void logError(const std::string& message) const { log(CLAP_LOG_ERROR, message); }

    // Parameter synchronization methods
  void notifyGUIParameterChange(const Path& paramName, float normalizedValue) {
#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      if (guiInstance) {
        // Send message to GUI using mlvg messaging system
        ml::Path msgPath = ml::Path("set_param", paramName);
        ml::Message msg{msgPath, normalizedValue};
        msg.flags |= ml::kMsgFromController;  // Prevent echo back to processor
        guiInstance->enqueueMessage(msg);
      }
    }
#endif
  }

  void requestHostParameterFlush() {
    if (hostParams && hostParams->request_flush) {
      logInfo("Requesting host parameter flush");
      hostParams->request_flush(host);
    } else {
      logWarning("Host parameter extension not available for flush request");
    }
  }

  // Track parameter changes for GUI->Host sync
  struct ChangedParam {
    clap_id id;
    double value;
  };
  std::vector<ChangedParam> changedParams;
  std::mutex changedParamsMutex;

  void markParameterChanged(clap_id paramId, double realValue) {
    std::lock_guard<std::mutex> lock(changedParamsMutex);
    // Check if already in list
    for (auto& cp : changedParams) {
      if (cp.id == paramId) {
        cp.value = realValue;
        return;
      }
    }
    // Add new changed parameter
    changedParams.push_back({paramId, realValue});
  }

  clap_process_status processAudio(const clap_process* process) {
    // Safety checks to prevent crashes
    if (!process || !audioContext || !processor) {
      logWarning("processAudio: Missing required components");
      return CLAP_PROCESS_CONTINUE;
    }

    if (process->audio_inputs_count == 0 || process->audio_outputs_count == 0) {
      logWarning("processAudio: No audio I/O available");
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

    // Send any pending parameter changes from GUI to host
    if (process->out_events) {
      std::lock_guard<std::mutex> lock(changedParamsMutex);
      if (!changedParams.empty()) {
        logInfo("process: Sending " + std::to_string(changedParams.size()) + " parameter changes");

        for (const auto& cp : changedParams) {
          // Create parameter value event
          clap_event_param_value event;
          event.header.size = sizeof(clap_event_param_value);
          event.header.time = 0;  // Beginning of this process block
          event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
          event.header.type = CLAP_EVENT_PARAM_VALUE;
          event.header.flags = 0;

          event.param_id = cp.id;
          event.cookie = nullptr;
          event.note_id = -1;  // Global parameter change
          event.port_index = -1;
          event.channel = -1;
          event.key = -1;
          event.value = cp.value;  // Real value matching reported range

          // Try to push the event
          if (!process->out_events->try_push(process->out_events, &event.header)) {
            logWarning("process: Failed to push parameter change event for param " + std::to_string(cp.id));
          } else {
            logInfo("process: Sent parameter change: id=" + std::to_string(cp.id) +
                    " value=" + std::to_string(cp.value));
          }
        }
        // Clear the changed parameters list
        changedParams.clear();
      }
    }

    return processor->hasActiveVoices() ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_SLEEP;
  }

  // Extension implementation
  const void* getExtension(const char* id) {
    logInfo("getExtension: Requested extension: " + std::string(id ? id : "null"));
    
    if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &audioPortsExt;
    if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &notePortsExt;
    if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &paramsExt;
    if (strcmp(id, CLAP_EXT_STATE) == 0) {
      logInfo("getExtension: Returning state extension");
      return &stateExt;
    }
    if (strcmp(id, CLAP_EXT_GUI) == 0) return &guiExt;
    
    logInfo("getExtension: Extension not found: " + std::string(id ? id : "null"));
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

        // Get default value - try plaindefault first (real value), then default
        float realDefaultVal = 0.5f * (range[0] + range[1]); // middle of range as fallback
        if (paramDesc->hasProperty("plaindefault")) {
          realDefaultVal = paramDesc->getFloatProperty("plaindefault");
        } else if (paramDesc->hasProperty("default")) {
          // Legacy: if using "default", treat as real value
          realDefaultVal = paramDesc->getFloatProperty("default");
        }

        info->id = index;
        strncpy(info->name, paramName.getText(), CLAP_NAME_SIZE - 1);
        info->name[CLAP_NAME_SIZE - 1] = '\0';
        info->min_value = range[0];
        info->max_value = range[1];
        info->default_value = ml::clamp(realDefaultVal, (float)info->min_value, (float)info->max_value);
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
          // CLAP expects real values matching the min/max range we reported
          *value = wrapper->processor->getRealFloatParam(paramName);
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

    // auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    // if (wrapper) {
    //   std::ostringstream logMsg;
    //   logMsg << "paramsTextToValue: param_id=" << param_id << " text='" << param_value_text << "'";
    //   wrapper->logInfo(logMsg.str());
    // }

    // string to double conversion
    char* endptr = nullptr;
    *out_value = std::strtod(param_value_text, &endptr);
    
    // Check for conversion errors
    if (endptr == param_value_text || *endptr != '\0') {
      auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
      if (wrapper) wrapper->logError("paramsTextToValue: Failed to convert text to value");
      return false;
    }

    // If Bitwig is using this method, we need to actually set the parameter. This isn't clear to me.
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (wrapper && wrapper->processor) {
      const auto& descriptions = wrapper->processor->getParameterTree().descriptions;
      uint32_t currentIndex = 0;

      for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
        if (currentIndex == param_id) {
          const auto& paramDesc = *it;
          if (paramDesc) {
            auto paramName = paramDesc->getTextProperty("name");
            // wrapper->logInfo("paramsTextToValue: Setting parameter " + std::string(paramName.getText()) + " to " + std::to_string(*out_value));
            wrapper->processor->setParamFromNormalizedValue(paramName, *out_value);
            break;
          }
        }
        currentIndex++;
      }
    }

    return true;
  }

  static void paramsFlush(const clap_plugin* plugin, const clap_input_events* in,
                         const clap_output_events* out) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !in) {
      if (wrapper) wrapper->logWarning("paramsFlush called with null components");
      return;
    }

    wrapper->logInfo("paramsFlush called, pending changes: " + std::to_string(wrapper->changedParams.size()));

    // Send any pending parameter changes from GUI to host
    if (out) {
      std::lock_guard<std::mutex> lock(wrapper->changedParamsMutex);
      for (const auto& cp : wrapper->changedParams) {
        // Create parameter value event
        clap_event_param_value event;
        event.header.size = sizeof(clap_event_param_value);
        event.header.time = 0;
        event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        event.header.type = CLAP_EVENT_PARAM_VALUE;
        event.header.flags = 0;

        event.param_id = cp.id;
        event.cookie = nullptr;
        event.note_id = -1;  // Global parameter change
        event.port_index = -1;
        event.channel = -1;
        event.key = -1;
        event.value = cp.value;  // Real value matching reported range

        // Try to push the event
        if (!out->try_push(out, &event.header)) {
          wrapper->logWarning("Failed to push parameter change event for param " + std::to_string(cp.id));
        } else {
          wrapper->logInfo("Sent parameter change: id=" + std::to_string(cp.id) +
                          " value=" + std::to_string(cp.value));
        }
      }
      // Clear the changed parameters list
      wrapper->changedParams.clear();
    }

    uint32_t eventCount = in->size(in);

    // if (eventCount > 0) {
    //   std::ostringstream eventLog;
    //   eventLog << "paramsFlush: Processing " << eventCount << " parameter events";
    //   wrapper->logInfo(eventLog.str());
    // } else {
    //   wrapper->logInfo("paramsFlush: No events to process");
    // }

    for (uint32_t i = 0; i < eventCount; ++i) {
      const clap_event_header* header = in->get(in, i);
      if (!header) continue;

      if (header->type == CLAP_EVENT_PARAM_VALUE) {
        // Validate namespace ID - only process events from core event space
        if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
          continue; // Skip events with wrong namespace ID
        }
        
        const auto* paramEvent = reinterpret_cast<const clap_event_param_value*>(header);

        const auto& descriptions = wrapper->processor->getParameterTree().descriptions;
        uint32_t currentIndex = 0;

        for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
          if (currentIndex == paramEvent->param_id) {
            const auto& paramDesc = *it;
            if (paramDesc) {
              auto paramName = paramDesc->getTextProperty("name");

              // CLAP sends real values matching the reported range
              wrapper->processor->setParamFromRealValue(paramName, paramEvent->value);

              // Get normalized value for GUI notification
              float normalizedValue = wrapper->processor->getNormalizedFloatParam(paramName);

              // Notify GUI of parameter change (Host->GUI sync)
              wrapper->notifyGUIParameterChange(paramName, normalizedValue);
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
    if (!wrapper || !wrapper->processor || !stream) {
      if (wrapper) wrapper->logError("stateSave: Missing required components");
      return false;
    }

    wrapper->logInfo("stateSave: Starting state save operation");

    // Get normalized parameter values from ParameterTree
    const auto& paramValues = wrapper->processor->getParameterTree().getNormalizedValues();
    wrapper->logInfo("stateSave: Found " + std::to_string(paramValues.size()) + " parameters to save");

    // Write parameter count first
    uint32_t paramCount = 0;
    for (auto it = paramValues.begin(); it != paramValues.end(); ++it) {
      paramCount++;
    }
    
    int64_t bytesWritten = stream->write(stream, &paramCount, sizeof(paramCount));
    if (bytesWritten != sizeof(paramCount)) {
      wrapper->logError("stateSave: Failed to write parameter count");
      return false;
    }

    // Write each parameter as binary data (name length, name, value)
    for (auto it = paramValues.begin(); it != paramValues.end(); ++it) {
      ml::Path paramPath = it.getCurrentPath();
      ml::Value paramValue = *it;
      
      std::string paramName = std::string(pathToText(paramPath).getText());
      float value = paramValue.getFloatValue();
      
      // Write name length
      uint32_t nameLength = static_cast<uint32_t>(paramName.length());
      bytesWritten = stream->write(stream, &nameLength, sizeof(nameLength));
      if (bytesWritten != sizeof(nameLength)) {
        wrapper->logError("stateSave: Failed to write parameter name length");
        return false;
      }
      
      // Write name
      bytesWritten = stream->write(stream, paramName.c_str(), nameLength);
      if (bytesWritten != static_cast<int64_t>(nameLength)) {
        wrapper->logError("stateSave: Failed to write parameter name");
        return false;
      }
      
      // Write value
      bytesWritten = stream->write(stream, &value, sizeof(value));
      if (bytesWritten != sizeof(value)) {
        wrapper->logError("stateSave: Failed to write parameter value");
        return false;
      }
    }

    wrapper->logInfo("stateSave: SUCCESS - wrote " + std::to_string(paramCount) + " parameters");
    return true;
  }

  static bool stateLoad(const clap_plugin* plugin, const clap_istream* stream) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !wrapper->processor || !stream) {
      if (wrapper) wrapper->logError("stateLoad: Missing required components");
      return false;
    }

    wrapper->logInfo("stateLoad: Starting state load operation");

    // Read parameter count
    uint32_t paramCount;
    int64_t bytesRead = stream->read(stream, &paramCount, sizeof(paramCount));
    if (bytesRead != sizeof(paramCount)) {
      wrapper->logError("stateLoad: Failed to read parameter count");
      return false;
    }

    wrapper->logInfo("stateLoad: Reading " + std::to_string(paramCount) + " parameters");

    // Read each parameter
    int restoredCount = 0;
    for (uint32_t i = 0; i < paramCount; ++i) {
      // Read name length
      uint32_t nameLength;
      bytesRead = stream->read(stream, &nameLength, sizeof(nameLength));
      if (bytesRead != sizeof(nameLength)) {
        wrapper->logError("stateLoad: Failed to read parameter name length");
        return false;
      }
      
      // Read name
      std::string paramName(nameLength, '\0');
      bytesRead = stream->read(stream, &paramName[0], nameLength);
      if (bytesRead != static_cast<int64_t>(nameLength)) {
        wrapper->logError("stateLoad: Failed to read parameter name");
        return false;
      }
      
      // Read value
      float value;
      bytesRead = stream->read(stream, &value, sizeof(value));
      if (bytesRead != sizeof(value)) {
        wrapper->logError("stateLoad: Failed to read parameter value");
        return false;
      }

      wrapper->logInfo("stateLoad: Setting parameter '" + paramName + "' = " + std::to_string(value));

      // Set the parameter value
      wrapper->processor->setParamFromNormalizedValue(ml::Path(ml::TextFragment(paramName.c_str())), value);
      restoredCount++;
    }

    wrapper->logInfo("stateLoad: Successfully restored " + std::to_string(restoredCount) + " parameters");
    wrapper->logInfo("stateLoad: SUCCESS");
    return true;
  }

  static const clap_plugin_state stateExt;

  // GUI Extension - Simple GUI support
  static bool guiIsApiSupported(const clap_plugin* plugin, const char* api, bool is_floating) {
    // For now, support all APIs - can be customized per plugin
    return true;
  }

  static bool guiGetPreferredApi(const clap_plugin* plugin, const char** api, bool* is_floating) {
    // Default to cocoa on macOS, win32 on Windows, x11 on Linux
    #ifdef __APPLE__
    *api = CLAP_WINDOW_API_COCOA;
    #elif defined(WIN32)
    *api = CLAP_WINDOW_API_WIN32;
    #else
    *api = CLAP_WINDOW_API_X11;
    #endif
    *is_floating = false;
    return true;
  }

  static bool guiCreate(const clap_plugin* plugin, const char* api, bool is_floating) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper) return false;

    wrapper->logInfo("GUI: Creating GUI with API: " + std::string(api ? api : "null"));

#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      // Create GUI instance directly - no need for processor to handle it
      wrapper->guiInstance = std::make_unique<GUIClass>(wrapper->processor.get());
      wrapper->guiCreated = true;
      wrapper->logInfo("GUI: Successfully created GUI instance");
      return true;
    } else {
      wrapper->logInfo("GUI: No GUI class specified");
      return false;
    }
#else
    wrapper->logInfo("GUI: GUI support disabled at compile time");
    return false;
#endif
  }

  static void guiDestroy(const clap_plugin* plugin) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (wrapper) {
      wrapper->logInfo("GUI: Destroying GUI");
#ifdef HAS_GUI
      if constexpr (!std::is_void_v<GUIClass>) {
        if (wrapper->guiInstance) {
          wrapper->guiInstance->stopTimersAndActor();
          wrapper->logInfo("GUI: Stopped AppView timers");

          wrapper->guiInstance->clearResources();
          wrapper->logInfo("GUI: Cleared AppView resources");
        }

        // Explicitly destroy PlatformView first, then the GUI instance
        wrapper->platformView.reset();
        wrapper->logInfo("GUI: PlatformView reset");

        wrapper->guiInstance.reset();
        wrapper->logInfo("GUI: GUI instance reset");

        wrapper->guiCreated = false;
        wrapper->widgetsCreated = false;  // Reset widget creation flag for next creation
      }
#endif
    }
  }

  static bool guiSetScale(const clap_plugin* plugin, double scale) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper) return false;

    wrapper->logInfo("GUI: Setting scale to " + std::to_string(scale));
    return true;
  }

  static bool guiGetSize(const clap_plugin* plugin, uint32_t* width, uint32_t* height) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !width || !height) return false;

#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      if (wrapper->guiInstance) {
        // Get size from MLVG's grid system using getDefaultDims()
        auto defaultDims = wrapper->guiInstance->getDefaultDims();

        *width = static_cast<uint32_t>(defaultDims.x());
        *height = static_cast<uint32_t>(defaultDims.y());

        wrapper->logInfo("GUI: Reporting size from MLVG: " + std::to_string(*width) + "x" + std::to_string(*height));
        return true;
      }
    }

    // Fallback to default size if no GUI instance
    *width = wrapper->guiWidth;
    *height = wrapper->guiHeight;
#else
    *width = 400;
    *height = 300;
#endif
    return true;
  }

  static bool guiCanResize(const clap_plugin* plugin) {
    // Allow resizing but we'll enforce aspect ratio constraints
    return true;
  }

  static bool guiGetResizeHints(const clap_plugin* plugin, clap_gui_resize_hints* hints) {
    if (!hints) return false;
    // Allow both horizontal and vertical resizing, but preserve aspect ratio
    hints->can_resize_horizontally = true;
    hints->can_resize_vertically = true;
    hints->preserve_aspect_ratio = true;
    return true;
  }

  static bool guiAdjustSize(const clap_plugin* plugin, uint32_t* width, uint32_t* height) {
    if (!width || !height) return false;

    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper) return false;

#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      if (wrapper->guiInstance) {
        // Get the fixed aspect ratio from the GUI instance
        auto fixedRatio = wrapper->guiInstance->getFixedAspectRatio();
        if (fixedRatio.x() > 0 && fixedRatio.y() > 0) {
          float targetRatio = fixedRatio.x() / fixedRatio.y();
          float currentRatio = static_cast<float>(*width) / static_cast<float>(*height);

          // Apply aspect ratio constraint
          if (currentRatio < targetRatio) {
            // Too narrow - adjust width to match height
            *width = static_cast<uint32_t>(*height * targetRatio);
          } else {
            // Too wide - adjust height to match width
            *height = static_cast<uint32_t>(*width / targetRatio);
          }

          wrapper->logInfo("GUI: Adjusted size to " + std::to_string(*width) + "x" + std::to_string(*height) + " (ratio: " + std::to_string(targetRatio) + ")");
        }
      }
    }
#endif

    return true;
  }

  static bool guiSetSize(const clap_plugin* plugin, uint32_t width, uint32_t height) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper) return false;

    wrapper->logInfo("GUI: Setting size to " + std::to_string(width) + "x" + std::to_string(height));

#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      if (wrapper->guiInstance && wrapper->platformView) {
        // Update the platform view size to match the new dimensions
        wrapper->platformView->setPlatformViewSize(width, height);

        // Notify the GUI instance of the resize
        wrapper->guiInstance->onResize(ml::Vec2(width, height));

        wrapper->logInfo("GUI: Platform view and GUI instance updated with new size");
      }
    }
#endif

    return true;
  }

  static bool guiSetParent(const clap_plugin* plugin, const clap_window* window) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !window) return false;

    wrapper->logInfo("GUI: Setting parent window with API: " + std::string(window->api));

#ifdef HAS_GUI
    if constexpr (!std::is_void_v<GUIClass>) {
      if (!wrapper->guiInstance) {
        wrapper->logError("GUI: No GUI instance to attach");
        return false;
      }

      // Extract platform window handle
      void* nativeWindow = nullptr;
      if (strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0) {
        nativeWindow = window->cocoa;
      } else if (strcmp(window->api, CLAP_WINDOW_API_X11) == 0) {
        nativeWindow = reinterpret_cast<void*>(window->x11);
      } else if (strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0) {
        nativeWindow = window->win32;
      }

      if (!nativeWindow) {
        wrapper->logError("GUI: Unsupported platform window API");
        return false;
      }

      // Debug: Log the native window pointer and AppView pointer
      wrapper->logInfo("GUI: Creating PlatformView with window: " +
                      std::to_string(reinterpret_cast<uintptr_t>(nativeWindow)));
      wrapper->logInfo("GUI: Creating PlatformView with AppView: " +
                      std::to_string(reinterpret_cast<uintptr_t>(wrapper->guiInstance.get())));

      // Create PlatformView internally - plugin never sees it!
      wrapper->platformView = std::make_unique<PlatformView>(
        wrapper->descriptor->name, nativeWindow, wrapper->guiInstance.get(), nullptr, 0, 60
      );

      // Initialize resources
      wrapper->guiInstance->initializeResources(wrapper->platformView->getNativeDrawContext());

      // Inform AppView of its initial size to set up coordinate system
      uint32_t width, height;
      guiGetSize(plugin, &width, &height);
      wrapper->logInfo("GUI: Informing AppView of initial size: " + std::to_string(width) + "x" + std::to_string(height));
      float displayScale = PlatformView::getDeviceScaleForWindow(nativeWindow);
      wrapper->guiInstance->viewResized(wrapper->platformView->getNativeDrawContext(), {(float)width, (float)height}, displayScale);

      // Create widgets immediately—AUv2 doesn't call guiShow
      if (!wrapper->widgetsCreated) {
        wrapper->logInfo("GUI: Creating widgets in guiSetParent (for AUv2 compatibility)");
        wrapper->guiInstance->makeWidgets();

        // Connect widgets to parameters automatically
        wrapper->guiInstance->connectParameters();
        wrapper->logInfo("GUI: Connected widgets to parameters");

        // Attach before starting timers
        wrapper->platformView->attachViewToParent();
        wrapper->logInfo("GUI: Platform view attached in guiSetParent");

        // Start AppView timers AFTER attachment
        wrapper->guiInstance->startTimersAndActor();
        wrapper->logInfo("GUI: Started AppView timers for event processing");

        wrapper->widgetsCreated = true;
        wrapper->logInfo("GUI: Widgets created successfully in guiSetParent");
      }

      wrapper->logInfo("GUI: Platform view created successfully, parent set");
      return true;
    } else {
      wrapper->logInfo("GUI: No GUI class specified");
      return false;
    }
#else
    wrapper->logInfo("GUI: GUI support disabled at compile time");
    return false;
#endif
  }

  static bool guiSetTransient(const clap_plugin* plugin, const clap_window* window) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (!wrapper || !window) return false;

    wrapper->logInfo("GUI: Setting transient window");
    return true;
  }

  static void guiSuggestTitle(const clap_plugin* plugin, const char* title) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (wrapper && title) {
      wrapper->logInfo("GUI: Suggesting title: " + std::string(title));
    }
  }

  static bool guiShow(const clap_plugin* plugin) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (wrapper) {
      wrapper->logInfo("GUI: Showing GUI");
#ifdef HAS_GUI
      if constexpr (!std::is_void_v<GUIClass>) {
        if (wrapper->platformView && wrapper->guiInstance) {
          // Widgets are created in guiSetParent for AUv2
          if (!wrapper->widgetsCreated) {
            wrapper->logInfo("GUI: Creating widgets in guiShow");
            wrapper->guiInstance->makeWidgets();

            // Connect widgets to parameters automatically
            wrapper->guiInstance->connectParameters();
            wrapper->logInfo("GUI: Connected widgets to parameters");

            // Ensure view size is properly set before starting render loop
            uint32_t width, height;
            guiGetSize(plugin, &width, &height);
            if (width > 0 && height > 0) {
              wrapper->logInfo("GUI: Ensuring view size is set: " + std::to_string(width) + "x" + std::to_string(height));
              wrapper->guiInstance->viewResized(wrapper->platformView->getNativeDrawContext(), {(float)width, (float)height}, 1.0f);
            }

            // Attach before starting timers
            wrapper->platformView->attachViewToParent();
            wrapper->logInfo("GUI: Platform view attached in guiShow");

            // Start AppView timers AFTER attachment
            wrapper->guiInstance->startTimersAndActor();
            wrapper->logInfo("GUI: Started AppView timers for event processing");

            wrapper->widgetsCreated = true;
            wrapper->logInfo("GUI: Widgets created successfully");
          } else {
            wrapper->logInfo("GUI: Widgets already created in guiSetParent - GUI ready");
          }

          wrapper->logInfo("GUI: Platform view visible");
        }
      }
#endif
    }
    return true;
  }

  static bool guiHide(const clap_plugin* plugin) {
    auto* wrapper = static_cast<CLAPPluginWrapper*>(plugin->plugin_data);
    if (wrapper) {
      wrapper->logInfo("GUI: Hiding GUI");
#ifdef HAS_GUI
      if constexpr (!std::is_void_v<GUIClass>) {
        if (wrapper->platformView) {
          // PlatformView handles hiding automatically when detached
          wrapper->logInfo("GUI: Platform view hidden");
        }
      }
#endif
    }
    return true;
  }



  static const clap_plugin_gui guiExt;

private:
  void convertCLAPEventsToAudioContext(const clap_input_events* events) {
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
          // Validate namespace ID - only process events from core event space
          if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
            continue; // Skip events with wrong namespace ID
          }
          
          auto* paramEvent = reinterpret_cast<const clap_event_param_value*>(header);

          // std::ostringstream paramLog;
          // paramLog << "PARAM EVENT: ID=" << paramEvent->param_id
          //          << " Value=" << paramEvent->value;
          // logInfo(paramLog.str());

          // Directly update the ParameterTree
          const auto& descriptions = processor->getParameterTree().descriptions;
          uint32_t currentIndex = 0;

          for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
            if (currentIndex == paramEvent->param_id) {
              const auto& paramDesc = *it;
              if (paramDesc) {
                auto paramName = paramDesc->getTextProperty("name");

                // CLAP sends parameter values in their real ranges,
                // madronalib handles the conversion internally
                processor->setParamFromRealValue(paramName, paramEvent->value);

                // Get normalized value for GUI notification
                float normalizedValue = processor->getNormalizedFloatParam(paramName);

                // Notify GUI of parameter change (Host->GUI sync)
                notifyGUIParameterChange(paramName, normalizedValue);
                break;
              }
            }
            currentIndex++;
          }

          // Also send as controller event for compatibility
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

// Export macro for plugins with GUI
// Parameters:
//   ClassName = your SignalProcessor-derived class (e.g., ClapSawDemo)
//   GUIClass = your AppView-derived class (e.g., ClapSawDemoGUI)
//   PluginName = display name for DAW (e.g., "Clap Saw Demo")
//   VendorName = company name (e.g., "Madrona Labs")
// Usage: MADRONALIB_EXPORT_CLAP_PLUGIN_WITH_GUI(ClapSawDemo, ClapSawDemoGUI, "Clap Saw Demo", "Madrona Labs")
#define MADRONALIB_EXPORT_CLAP_PLUGIN_WITH_GUI(ClassName, GUIClass, PluginName, VendorName) \
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
    return new ml::CLAPPluginWrapper<ClassName, GUIClass>(host, &desc); \
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
template<typename PluginClass, typename GUIClass>
const clap_plugin_audio_ports CLAPPluginWrapper<PluginClass, GUIClass>::audioPortsExt = {
  CLAPPluginWrapper<PluginClass, GUIClass>::audioPortsCount,
  CLAPPluginWrapper<PluginClass, GUIClass>::audioPortsGet
};

template<typename PluginClass, typename GUIClass>
const clap_plugin_note_ports CLAPPluginWrapper<PluginClass, GUIClass>::notePortsExt = {
  CLAPPluginWrapper<PluginClass, GUIClass>::notePortsCount,
  CLAPPluginWrapper<PluginClass, GUIClass>::notePortsGet
};

template<typename PluginClass, typename GUIClass>
const clap_plugin_params CLAPPluginWrapper<PluginClass, GUIClass>::paramsExt = {
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsCount,
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsInfo,
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsValue,
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsValueToText,
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsTextToValue,
  CLAPPluginWrapper<PluginClass, GUIClass>::paramsFlush
};

template<typename PluginClass, typename GUIClass>
const clap_plugin_state CLAPPluginWrapper<PluginClass, GUIClass>::stateExt = {
  CLAPPluginWrapper<PluginClass, GUIClass>::stateSave,
  CLAPPluginWrapper<PluginClass, GUIClass>::stateLoad
};

template<typename PluginClass, typename GUIClass>
const clap_plugin_gui CLAPPluginWrapper<PluginClass, GUIClass>::guiExt = {
  CLAPPluginWrapper<PluginClass, GUIClass>::guiIsApiSupported,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiGetPreferredApi,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiCreate,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiDestroy,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiSetScale,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiGetSize,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiCanResize,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiGetResizeHints,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiAdjustSize,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiSetSize,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiSetParent,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiSetTransient,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiSuggestTitle,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiShow,
  CLAPPluginWrapper<PluginClass, GUIClass>::guiHide
};

} // namespace ml
