// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "MLParameters.h"
#include "MLDSPUtils.h"

constexpr size_t kPublishedSignalMaxChannels = 2;
constexpr size_t kPublishedSignalReadFrames = 128;

namespace ml {


// SignalProcessor is the top level object in a DSP graph. The app or plugin calls it to generate
// audio as needed. It has facilities for sending audio data to outside the graph, and
// keeping in sync with a host's time.

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
    PublishedSignal(int channels, int octavesDown) :
    _downsampler(channels, octavesDown),
    _channels(channels)
    {
      _buffer.resize(kPublishedSignalReadFrames*_channels);
    }
    
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
    inline size_t readLatest(float* pDest, size_t framesRequested)
    {
      size_t available = getAvailableFrames();
      if(available > framesRequested)
      {
        _buffer.discard((available - framesRequested)*_channels);
      }
      auto readResult = _buffer.read(pDest, framesRequested*_channels);
      return readResult;
    }
    
    inline size_t read(float* pDest, size_t framesRequested)
    {
      return _buffer.read(pDest, framesRequested*_channels);
    }
  };
  
  // SignalProcessor::ProcessTime maintains the current time in a DSP process and can track
  // the time in the host application if there is one.
  
  class ProcessTime
  {
  public:
    ProcessTime() = default;
    ~ProcessTime() = default;
    
    // Set the time and bpm. The time refers to the start of the current engine processing block.
    inline void setTimeAndRate(const double secs, const double ppqPos, const double bpm, bool isPlaying, double sampleRate)
    {
      // working around a bug I can't reproduce, so I'm covering all the bases.
      if ( ((ml::isNaN(ppqPos)) || (ml::isInfinite(ppqPos)))
          || ((ml::isNaN(bpm)) || (ml::isInfinite(bpm)))
          || ((ml::isNaN(secs)) || (ml::isInfinite(secs))) )
      {
        //debug << "PluginProcessor::ProcessTime::setTimeAndRate: bad input! \n";
        return;
      }
      
      bool active = (_ppqPos1 != ppqPos) && isPlaying;
      bool justStarted = isPlaying && !_playing1;
      
      double ppqPhase = 0.;
      
      if (active)
      {
        if(ppqPos > 0.f)
        {
          ppqPhase = ppqPos - floor(ppqPos);
        }
        else
        {
          ppqPhase = ppqPos;
        }
        
        _omega = ppqPhase;
        
        if(justStarted)
        {
          // just start at 0 and don't attempt to match the playhead position.
          // this works well when we start at any 1/4 note.
          // there is still some weirdness when we try to lock onto other 16ths.
          _omega = 0.;
          // std::cout << "phasor START: " << mOmega << "\n";
          _dpdt = 0.;
          _dsdt = 0.;
        }
        else
        {
          double dPhase = ppqPhase - _ppqPhase1;
          if(dPhase < 0.)
          {
            dPhase += 1.;
          }
          _dpdt = ml::clamp(dPhase/static_cast<double>(_samplesSincePreviousTime), 0., 1.);
          _dsdt = static_cast<float>(1./sampleRate);
        }
        
        _secondsCounter = secs;
        _secondsPhaseCounter = fmodl(secs, 1.0);
      }
      else
      {
        _omega = -1.f;
        _dpdt = 0.;
        _secondsCounter = -1.;
        _dsdt = 0.;
        _secondsPhaseCounter = -1.;
      }
      
      _ppqPos1 = ppqPos;
      _ppqPhase1 = ppqPhase;
      _active1 = active;
      _playing1 = isPlaying;
      _samplesSincePreviousTime = 0;
    }
    
    inline void clear(void)
    {
      _dpdt = 0.;
      _dsdt = 0.;
      _active1 = false;
      _playing1 = false;
    }
    
    // generate phasors from the input parameters
    inline void process()
    {
      for (int n=0; n<kFloatsPerDSPVector; ++n)
      {
        _quarterNotesPhase[n] = _omega;
        _omega += _dpdt;
        if(_omega > 1.f)
        {
          _omega -= 1.f;
        }
      }
      for (int n=0; n<kFloatsPerDSPVector; ++n)
      {
        _seconds[n] = _secondsCounter;
        _secondsCounter += _dsdt;
        _secondsPhase[n] = _secondsPhaseCounter;
        _secondsPhaseCounter += _dsdt;
      }
      _samplesSincePreviousTime += kFloatsPerDSPVector;
    }
    
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
  
  SignalProcessor(size_t inChans, size_t outChans) {};
  ~SignalProcessor() {};
  
  virtual DspVectorDynamic processVector(const DspVectorDynamic& inputs) = 0;
  
protected:
  
  // the plain parameter values are stored here.
  ParameterTreePlain _params;
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
  
  template<size_t CHANNELS>
  inline void storePublishedSignal(Path signalName, DSPVectorArray< CHANNELS > v)
  {
    PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->write(v);
    }
  }
};

} // namespaces


