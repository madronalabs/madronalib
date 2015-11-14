
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcRingBuffer.h"

// ----------------------------------------------------------------
// registry section

// MLProcRingBuffer has no signal outputs-- use read method to get buffered signal.  
// We need a ringbuffer if we are transferring signals between procs that may run in
// different threads.

namespace
{
	MLProcRegistryEntry<MLProcRingBuffer> classReg("ringbuffer");
	ML_UNUSED MLProcParam<MLProcRingBuffer> params[2] = {"length", "mode"};
	ML_UNUSED MLProcInput<MLProcRingBuffer> inputs[] = {"in"};
}	

// ----------------------------------------------------------------
// implementation


MLProcRingBuffer::MLProcRingBuffer()
{
	// defaults
	// TODO set from component->engine.
	//  will want downsampling for viewing when running at 96k or higher...
	setParam("length", kMLRingBufferDefaultSize);	
	setParam("mode", eMLRingBufferNoTrash);
	mTrig1 = -1.f;
}


MLProcRingBuffer::~MLProcRingBuffer()
{
//	debug() << "MLProcRingBuffer destructor\n";
}

MLProc::err MLProcRingBuffer::resize() 
{	
	MLProc::err e = OK;
	int size = 1 << bitsToContain((int)getParam("length"));
	void * buf;
	
	// debug() << "allocating " << size << " samples for ringbuffer " << getName() << "\n";

	buf = mRing.setDims(size);
	
	if (!buf)
	{
		debug() << "MLRingBuffer: allocate failed!\n";
		e = memErr;
	}
	else
	{	
		PaUtil_InitializeRingBuffer( &mBuf, sizeof(MLSample), size, buf );
        
		// get trash signal
		if (getParam("mode") != eMLRingBufferNoTrash)
		{
			mTrashSignal.setDims(size);	
		}
	}

	return e;
}

void MLProcRingBuffer::doParams(void) 
{
	mParamsChanged = false;
}

void MLProcRingBuffer::process(const int frames)
{
	int written;
	const MLSignal& x = getInput(1);

	// build if needed
	if (mParamsChanged) doParams();

	if (mRing.getBuffer())
	{	
		if (x.isConstant())
		{
			written = (int)PaUtil_WriteRingBufferConstant( &mBuf, x[0], frames );	
		}
		else
		{
			written = (int)PaUtil_WriteRingBuffer(&mBuf, (void *)x.getConstBuffer(), frames );	
		}
	}
}

// read a ring buffer into the given row of the destination signal.
//
int MLProcRingBuffer::readToSignal(MLSignal& outSig, int samples, int row)
{
	int lastRead = 0;
	int skipped = 0;
	int available = 0;
	MLSample * outBuffer = outSig.getBuffer() + outSig.row(row);
	void * trashBuffer = (void *)mTrashSignal.getBuffer();
	MLSample * trashbufferAsSamples = reinterpret_cast<MLSample*>(trashBuffer);
	static MLSymbol modeSym("mode");
	int mode = (int)getParam(modeSym);
	bool underTrigger = false;
	MLSample triggerVal = 0.f;
		
	samples = min(samples, (int)outSig.getWidth());
	available = (int)PaUtil_GetRingBufferReadAvailable( &mBuf );
    
    // return if we have not accumulated enough signal.
	if (available < samples) return 0;
	
	// depending on trigger mode, trash samples up to the ones we will return.
	switch(mode)
	{
		default:
		case eMLRingBufferNoTrash:
		break;
		
		case eMLRingBufferUpTrig:		
			while (available >= samples+1)	
			{
				// read buffer
				lastRead = (int)PaUtil_ReadRingBuffer( &mBuf, trashBuffer, 1 );
				skipped += lastRead;
				available = (int)PaUtil_GetRingBufferReadAvailable( &mBuf );
				if(trashbufferAsSamples[0] < triggerVal)
				{
					underTrigger = true;
				}
				else
				{
					if (underTrigger == true) break;
					underTrigger = false;
				}
			}
		break;		

		case eMLRingBufferMostRecent:			
			// TODO modify pa ringbuffer instead of reading to trash buffer. 
			lastRead = (int)PaUtil_ReadRingBuffer( &mBuf, trashBuffer, available - samples );  
			// skipped += lastRead;		
		break;
	}
	
    lastRead = (int)PaUtil_ReadRingBuffer( &mBuf, outBuffer, samples );
	return lastRead;
}

