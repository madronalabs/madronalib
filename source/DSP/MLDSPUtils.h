
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "../core/MLProjection.h"
#include "MLSignalBuffer.h"
#include <array>

namespace ml
{
	inline void mapIndices(float* pDest, size_t size, Projection p)
	{
		for(int i=0; i<size; ++i)
		{
			pDest[i] = p(i);
		}
	}
	
	inline void makeWindow(float* pDest, size_t size, Projection windowShape)
	{
		IntervalProjection domainToUnity{ {0.f, size - 1.f}, {0.f, 1.f} };
		mapIndices(pDest, size, compose(windowShape, domainToUnity));
	}
	
	namespace windows
	{
		const Projection rectangle( [](float x){ return 1.f; } );
		const Projection triangle( [](float x){ return (x > 0.5f) ? (2.f - 2.f*x) : (2.f*x); } );
		const Projection raisedCosine( [](float x){ return 0.5f - 0.5f*cosf(kTwoPi*x); } );
		const Projection hamming( [](float x){ return 0.54f - 0.46f*cosf(kTwoPi*x); } );
		const Projection blackman( [](float x){ return 0.42f - 0.5f*cosf(kTwoPi*x) + 0.08f*cosf(2.f*kTwoPi*x); } );
		const Projection flatTop( [](float x){
			const float a0 = 0.21557895;
			const float a1 = 0.41663158;
			const float a2 = 0.277263158;
			const float a3 = 0.083578947;
			const float a4 = 0.006947368;
			return a0 - a1*cosf(kTwoPi*x) + a2*cosf(2.f*kTwoPi*x) - a3*cosf(3.f*kTwoPi*x) + a4*cosf(4.f*kTwoPi*x); } );
	}
	
	// VectorProcessBuffer: utility class to serve a main loop with varying arbitrary chunk sizes, buffer inputs and outputs,
	// and compute DSP in DSPVector-sized chunks
	template<int VECTORS, int MAX_FRAMES>
	class VectorProcessBuffer
	{
	public:
		VectorProcessBuffer()
		{
			for(int i=0; i < VECTORS; ++i)
			{
				mInputBuffers[i].resize(MAX_FRAMES);
				mOutputBuffers[i].resize(MAX_FRAMES);
			}
		}
		
		~VectorProcessBuffer(){}
		
		void process(float** inputs, float** outputs, int nChans, int nFrames, std::function<DSPVectorArray<VECTORS>(const DSPVectorArray<VECTORS>&, int chans)> fn)
		{
			// write
			for(int c = 0; c < nChans; c++)
			{
				mInputBuffers[c].write(inputs[c], nFrames);
			}
			
			DSPVectorArray<VECTORS> inputVectors;
			DSPVectorArray<VECTORS> outputVectors;
			
			// process
			while(mInputBuffers[0].getReadAvailable() >= kFloatsPerDSPVector)
			{
				// buffers to process input
				for(int c = 0; c < nChans; c++)
				{
					inputVectors.setRowVectorUnchecked(c, mInputBuffers[c].read());
				}
				
				outputVectors = fn(inputVectors, nChans);
				
				for(int c = 0; c < nChans; c++)
				{
					mOutputBuffers[c].write(outputVectors.getRowVectorUnchecked(c));
				}
			}
			
			// read
			for(int c = 0; c < nChans; c++)
			{
				mOutputBuffers[c].read(outputs[c], nFrames);
			}
		}
		
	private:
		std::array<ml::SignalBuffer, VECTORS> mInputBuffers;
		std::array<ml::SignalBuffer, VECTORS> mOutputBuffers;
	};
	
}

