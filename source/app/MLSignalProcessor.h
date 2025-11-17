// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLDSPUtils.h"
#include "MLParameters.h"
#include "MLPlatform.h"
#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

// SignalProcessor is the top level object in a DSP graph. The app or plugin calls it to generate
// audio as needed. It has facilities for sending audio data to outside the graph, and
// keeping a plugin in sync with a host's time.

class SignalProcessor
{
 public:
  SignalProcessor() {}
  virtual ~SignalProcessor() {}
  
  // SignalProcessor::PublishedSignal sends a signal from within a DSP
  // calculation to outside code like displays.
  struct PublishedSignal
  {
    std::vector<float> voiceRotateBuffer;
    DSPBuffer buffer_;
    size_t maxFrames_{0};
    size_t channels_{0};
    int octavesDown_{0};
    int downsampleCtr_{0};

    PublishedSignal(int frames, int maxVoices, int channels, int octavesDown);
    ~PublishedSignal() = default;

    inline size_t getNumChannels() const { return (size_t)channels_; }
    inline int getAvailableFrames() const { return (int)(channels_ ? (buffer_.getReadAvailable() / channels_) : 0); }
    inline int getReadAvailable() const { return (int)buffer_.getReadAvailable(); }
    
    // write frames from a DSPVectorArray< CHANNELS > of data into a published signal.
    // this data is for one voice of the signal.
    // don't use block size downsampler and get output samples as soon as they are available.
    //
    // frames are stored in buffer one by one, with a sample for each channel (frame major order), not in signal vectors!
    // When multiple voices are being sent, each voice is sent as one frame and the voices rotate:
    // frame0: [ voice0 [ chan0, chan1, chan2 ... chanN ] voice1 [ chan0, chan1, chan2 ... chanN ] ... ]
    // frame1: [ voice0 [ chan0, chan1, chan2 ... chanN ] voice1 [ chan0, chan1, chan2 ... chanN ] ... ]
    //
    // nothing here enforces the voice order- processors are responsible for calling
    // storePublishedSignal() for each voice in rotation.
    //
    // the voice param is not used currently but we can transmit the voice number in the future if needed.
    //
    template <size_t CHANNELS>
    inline void writeQuick(DSPVectorArray<CHANNELS> inputVector, size_t frames, size_t voice)
    {
      // on every (1<<octavesDown_)th frame, rotate and write to DSPBuffer
      int framesWritten = 0;
      for(int f=0; f<frames; ++f)
      {
        downsampleCtr_++;
        if(downsampleCtr_ >= (1 << octavesDown_))
        {
          // write accumulated frame.
          for(int j=0; j<CHANNELS; ++j)
          {
            voiceRotateBuffer[framesWritten*CHANNELS + j] = inputVector.row(j)[f];
          }
          framesWritten++;
          downsampleCtr_ = 0;
        }
      }
      
      if(framesWritten)
      {
        buffer_.write(voiceRotateBuffer.data(), framesWritten*CHANNELS);
      }
    }
    
    // write a single frame of signal with multiple contiguous channels
    inline void writeQuickVert(const float* inputVector, size_t channels, size_t voice)
    {
      // on every (1<<octavesDown_)th frame, rotate and write to DSPBuffer
      
      downsampleCtr_++;
      if(downsampleCtr_ >= (1 << octavesDown_))
      {
        buffer_.write(inputVector, channels);
        downsampleCtr_ = 0;
      }
    }
    
    // read the latest n frames of data, where each frame is a DSPVectorArray< CHANNELS >.
    size_t readLatest(float* pDest, size_t framesRequested);

    // read the next n frames of data.
    size_t read(float* pDest, size_t framesRequested);
    
    void peekLatest(float* pDest, size_t framesRequested);
  };

  // class used for assigning each instance of our SignalProcessor a unique ID
  class ProcessorRegistry
  {
    std::mutex idMutex_;
    size_t idCounter_{0};

   public:
    size_t getUniqueID()
    {
      std::unique_lock<std::mutex> lock(idMutex_);
      return ++idCounter_;
    }
  };

  virtual void processVector(const DSPVectorDynamic& inputs, DSPVectorDynamic& outputs, void* stateData = nullptr) {}

  // Sample rate access (needed by all adapters)
  virtual void setSampleRate(double sr) { sampleRate_ = sr; }
  double getSampleRate() const { return sampleRate_; }

  // Parameter tree access (needed for adapter initialization)
  ParameterTree& getParameterTree() { return params_; }
  const ParameterTree& getParameterTree() const { return params_; }

  // Convenience method for parameter count
  size_t getParameterCount() const { return params_.descriptions.size(); }

  // Public accessor for adapters to read published signals
  Tree<std::unique_ptr<PublishedSignal>>& getPublishedSignals() {
    return publishedSignals_;
  }
  const Tree<std::unique_ptr<PublishedSignal>>& getPublishedSignals() const {
    return publishedSignals_;
  }
  
  void setPublishedSignalsActive(bool b) { publishedSignalsAreActive_ = b; }

  inline void buildParams(const ParameterDescriptionList& paramList)
  {
    buildParameterTree(paramList, params_);
  };
  
  inline void setDefaultParams()
  {
    setDefaults(params_);
  };

  void setParamFromNormalizedValue(Path pname, float val)
  {
    params_.setFromNormalizedValue(pname, val);
  }

  void setParamFromRealValue(Path pname, float val)
  {
    params_.setFromRealValue(pname, val);
  }

  inline float getRealFloatParam(Path pname)
  {
    return params_.getRealFloatValueAtPath(pname);
  }

  inline float getNormalizedFloatParam(Path pname)
  {
    return params_.getNormalizedFloatValueAtPath(pname);
  }
  
 protected:

  ParameterTree params_;
  Tree< std::unique_ptr<PublishedSignal> > publishedSignals_;
  SharedResourcePointer<ProcessorRegistry> registry_;
  float sampleRate_{0.f};
  bool publishedSignalsAreActive_{false};
  
  // used by clients currently, don't delete! And TODO move relevant client code into this class.
  size_t uniqueID_;
  std::vector< ml::Path > paramNamesByID_;
  Tree<size_t> paramIDsByName_;
  
  inline void publishSignal(Path signalName, int maxFrames, int maxVoices, int channels, int octavesDown)
  {
    publishedSignals_[signalName] = std::make_unique<PublishedSignal>(maxFrames, maxVoices, channels, octavesDown);
  }

  // store a DSPVectorArray to the named signal buffer.
  // we need a buffer for each published signal here to move signals safely from the Processor
  // to the main thread.
  template <size_t CHANNELS>
  inline void storePublishedSignal(Path signalName, const DSPVectorArray<CHANNELS>& inputVec, int frames, int voice)
  {
    if(!publishedSignalsAreActive_) return;
    PublishedSignal* publishedSignal = publishedSignals_[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->writeQuick(inputVec, frames, voice);
    }
  }
  
  inline void storePublishedSignalVert(Path signalName, const float* pInput, int channels, int voice)
  {
    if(!publishedSignalsAreActive_) return;
    PublishedSignal* publishedSignal = publishedSignals_[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->writeQuickVert(pInput, channels, voice);
    }
  }
};

}  // namespace ml
