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
    DSPBuffer _buffer;
    size_t maxFrames_{0};
    size_t _channels{0};
    int octavesDown_{0};
    int downsampleCtr_{0};

    PublishedSignal(int frames, int maxVoices, int channels, int octavesDown);
    ~PublishedSignal() = default;

    inline size_t getNumChannels() const { return (size_t)_channels; }
    inline int getAvailableFrames() const { return (int)(_channels ? (_buffer.getReadAvailable() / _channels) : 0); }
    inline int getReadAvailable() const { return (int)_buffer.getReadAvailable(); }

    
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
        _buffer.write(voiceRotateBuffer.data(), framesWritten*CHANNELS);
      }
    }
    
    // write a single frame of signal with multiple contiguous channels
    inline void writeQuickVert(const float* inputVector, size_t channels, size_t voice)
    {
      // on every (1<<octavesDown_)th frame, rotate and write to DSPBuffer
      
      downsampleCtr_++;
      if(downsampleCtr_ >= (1 << octavesDown_))
      {
        _buffer.write(inputVector, channels);
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
  // TODO refactor w/ ProcessorRegistry etc.
  class ProcessorRegistry
  {
    std::mutex _IDMutex;
    size_t _IDCounter{0};

   public:
    size_t getUniqueID()
    {
      std::unique_lock<std::mutex> lock(_IDMutex);
      return ++_IDCounter;
    }
  };

  virtual void processVector(const DSPVectorDynamic& inputs, DSPVectorDynamic outputs, void* stateData = nullptr) {}

  void setParamFromNormalizedValue(Path pname, float val)
  {
    _params.setFromNormalizedValue(pname, val);
  }

  void setParamFromRealValue(Path pname, float val)
  {
    _params.setFromRealValue(pname, val);
  }

  inline void buildParams(const ParameterDescriptionList& paramList)
  {
    buildParameterTree(paramList, _params);
  };
  
  inline void setDefaultParams()
  {
    setDefaults(_params);
  };

  inline float getRealFloatParam(Path pname)
  {
    return _params.getRealFloatValue(pname);
  }
  
  inline float getNormalizedFloatParam(Path pname)
  {
    return _params.getNormalizedFloatValue(pname);
  }
  
 protected:

  ParameterTree _params;

  SharedResourcePointer<ProcessorRegistry> _registry;
  size_t _uniqueID;

  float _sampleRate{0.f};


  std::vector< ml::Path > _paramNamesByID;  // needed?
  Tree< size_t > _paramIDsByName;

  Tree< std::unique_ptr< PublishedSignal > > _publishedSignals;

  inline void publishSignal(Path signalName, int maxFrames, int maxVoices, int channels, int octavesDown)
  {
    _publishedSignals[signalName] = std::make_unique<PublishedSignal>(maxFrames, maxVoices, channels, octavesDown);
  }

  // store a DSPVectorArray to the named signal buffer.
  // we need a buffer for each published signal here to move signals safely from the Processor
  // to the main thread.
  template <size_t CHANNELS>
  inline void storePublishedSignal(Path signalName, const DSPVectorArray<CHANNELS>& inputVec, int frames, int voice)
  {
    PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->writeQuick(inputVec, frames, voice);
    }
  }
  
  inline void storePublishedSignalVert(Path signalName, const float* pInput, int channels, int voice)
  {
    PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->writeQuickVert(pInput, channels, voice);
    }
  }
};

}  // namespace ml
