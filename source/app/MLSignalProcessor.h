// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "MLParameters.h"
#include "MLDSPUtils.h"
#include "MLActor.h"

constexpr size_t kPublishedSignalMaxChannels = 2;
constexpr size_t kPublishedSignalReadFrames = 128;

namespace ml {

// SignalProcessor is the top level object in a DSP graph. The app or plugin calls it to generate
// audio as needed. It has facilities for sending audio data to outside the graph, and
// keeping a plugin in sync with a host's time.

class SignalProcessor
{
public:
  
  // SignalProcessor::PublishedSignal sends a signal from within a DSP
  // calculation to outside code like displays.
  class PublishedSignal
  {
    Downsampler _downsampler;
    DSPBuffer _buffer;
    size_t _channels{0};
    
  public:
    PublishedSignal(int channels, int octavesDown);
    ~PublishedSignal() = default;
    
    inline size_t getNumChannels() const { return _channels; }
    inline int getAvailableFrames() const { return _buffer.getReadAvailable()/_channels; }
    
    // write a single DSPVectorArray< CHANNELS > of data.
    template< size_t CHANNELS >
    inline void write(DSPVectorArray< CHANNELS > v)
    {
      // write to downsampler, which returns true if there is a new output vector
      if(_downsampler.write(v))
      {
        // write output vector array to the buffer
        _buffer.write(_downsampler.read< CHANNELS >());
      }
    }
    
    // read the latest n frames of data, where each frame is a DSPVectorArray< CHANNELS >.
    size_t readLatest(float* pDest, size_t framesRequested);
    
    // read the next n frames of data.
    size_t read(float* pDest, size_t framesRequested);
  };
  
  // SignalProcessor::ProcessTime maintains the current time in a DSP process and can track
  // the time in the host application if there is one.
  
  class ProcessTime
  {
  public:
    ProcessTime() = default;
    ~ProcessTime() = default;
    
    // Set the time and bpm. The time refers to the start of the current engine processing block.
    void setTimeAndRate(const double secs, const double ppqPos, const double bpm, bool isPlaying, double sampleRate);
    
    // clear state
    void clear();

    // generate phasors from the input parameters
    void process();
     
    // signals containing time from score start
    DSPVector _quarterNotesPhase;
    DSPVector _seconds;
    DSPVector _secondsPhase;

  private:
    float _omega{0};
    bool _playing1{false};
    bool _active1 {false};
    double _dpdt{0};
    double _dsdt{0};
    double _phase1{0};
    size_t _samplesSincePreviousTime{0};
    double _ppqPos1{0};
    double _ppqPhase1{0};
    double _secondsCounter{0};
    double _secondsPhaseCounter{0};
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
  
  SignalProcessor(size_t nInputs, size_t nOutputs) :
    processBuffer(nInputs, nOutputs, kMaxProcessBlockFrames)
  {}
  
  virtual ~SignalProcessor() {}
  
  virtual void processVector(MainInputs inputs, MainOutputs outputs, void *stateData = nullptr) {}
  
  void setParam(Path pname, float val)
  {
    _params.setParamFromNormalizedValue(pname, val);
  }

  inline void buildParams(const ParameterDescriptionList& paramList)
  {
    buildParameterTree(paramList, _params);
  };
  
  inline void dumpParams()
  {
    _params.dump();
  };
  

protected:
  
  // the maximum amount of input frames that can be proceesed at once. This determines the
  // maximum signal vector size of the plugin host or enclosing app.
  static constexpr int kMaxProcessBlockFrames {4096};
  
  // the plain parameter values are stored here.
  ParameterTreePlain _params;  

  SharedResourcePointer< ProcessorRegistry > _registry ;
  size_t _uniqueID;
    
  float _sampleRate{0.f};
  ProcessTime _currentTime;

  // buffer object to call processVector() from process() calls of arbitrary frame sizes
  VectorProcessBuffer processBuffer;


  std::vector< ml::Path > _paramNamesByID; // needed?
  Tree< size_t > _paramIDsByName;
  
  // single buffer for reading from signals
  std::vector< float > _readBuffer;
  

  // brief way to access params:
  inline float getParamNormalized(Path pname) { return getNormalizedValue(_params, pname); }
  inline float getParam(Path pname) { return getPlainValue(_params, pname); }
  
  Tree< std::unique_ptr< PublishedSignal > > _publishedSignals;
  
  inline void publishSignal(Path signalName, int channels, int octavesDown)
  {
    _publishedSignals[signalName] = ml::make_unique< PublishedSignal >(channels, octavesDown);
  }
  
  // store a DSPVectorArray to the named signal buffer.
  // we need a buffer for each published signal here in the Processor because we can't communicate with
  // the Controller in the audio thread.
  
  template< size_t CHANNELS >
  inline void storePublishedSignal(Path signalName, DSPVectorArray< CHANNELS > v)
  {
    PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->write(v);
    }
  }
};

class SignalProcessorActor :
  public SignalProcessor,
  public Actor
{
public:
  SignalProcessorActor(size_t nInputs, size_t nOutputs) :
    SignalProcessor(nInputs, nOutputs) {};
  
  virtual ~SignalProcessorActor() = default;
  
  // Actor implementation
  inline void onMessage(Message msg) override
  {
    //std::cout << "SignalProcessorActor: " << msg.address << " -> " << msg.value << "\n";
    
    switch(hash(head(msg.address)))
    {
      case(hash("set_param")):
      {
        setParam(tail(msg.address), msg.value.getFloatValue());
        break;
      }
      case(hash("set_prop")):
      {
        break;
      }
      case(hash("do")):
      {
        break;
      }
      default:
      {
        std::cout << " SignalProcessorActor: uncaught message " << msg << "! \n";
        break;
      }
    }
  }
};


} // namespaces


