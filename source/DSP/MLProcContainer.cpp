
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcContainer.h"

void MLSignalStats::dump()
{
	debug() << "PROCS:  " << mProcs 
	<< "  BUFS:   " << mSignalBuffers
	<< "  CONSTS: " << mConstantSignals 
	<< "  NAN: " << mNanSignals << "\n";
}

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcContainer> classReg("container");
	ML_UNUSED MLProcParam<MLProcContainer> params[] = {"*"};
	ML_UNUSED MLProcInput<MLProcContainer> inputs[] = {"*"};  // variable size
	ML_UNUSED MLProcOutput<MLProcContainer> outputs[] = {"*"};	// variable size
}


// ----------------------------------------------------------------
// implementation


MLProcContainer::MLProcContainer() :
	theProcFactory(MLProcFactory::theFactory()),
	mStatsPtr(0)
{
	setParam("ratio", 1.f);
	setParam("order", 2);
//	debug() << "MLProcContainer constructor\n";
}


MLProcContainer::~MLProcContainer()
{
//	debug() << "    ~MLProcContainer destructor\n";
}

// ----------------------------------------------------------------
#pragma mark MLDSPContext methods
//

void MLProcContainer::setEnabled(bool t)
{	
	// debug() << "enabling container " << getName() << "\n";
	
	// set enabled states of children
	for (std::list<MLProcPtr>::iterator it = mProcList.begin(); it != mProcList.end(); ++it)
	{
		MLProcPtr p = (*it);	
		if (p->isContainer())
		{	
			MLProcContainer& pc = static_cast<MLProcContainer&>(*p);
			pc.setEnabled(t);
		}
	}
	mEnabled = t;
}


bool MLProcContainer::isEnabled() const
{
	return mEnabled;
}

// all of the procs in an MLProcContainer are enabled, if the container is enabled
bool MLProcContainer::isProcEnabled(const MLProc* p) const
{
	#pragma unused(p)
	return mEnabled;
}

void MLProcContainer::setup()
{
	float fr;
	fr = getParam("ratio");
	MLRatio r = getCommonRatios().getClosest(fr);	
	setResampleRatio(r);
	
	int u = (int)getParam("up_order");
	int d = (int)getParam("down_order");
	setResampleUpOrder(u);
	setResampleDownOrder(d);
}

// mark as own context, so we are the root of the size/rate tree used in prepareToProcess().
void MLProcContainer::makeRoot(const MLSymbol name)
{
	setName(name);
	setContext(this);
}

// TODO: This works OK for current Aalto graph.  But reordering the reverb so that all
// procs are first, and all connections afterward, breaks the compile when optimizing buffers.
// revisit this.

void MLProcContainer::compile()
{
	const bool dumpOutputs = false;
	const bool verbose = false;
	err e = OK;

    // debug() << "\nCOMPILING MLContainer " << getName() << ": \n";

	// TODO: this block will determine order of operations from graph.
	// currently Procs are added to ops list in order of creation,
	// in other words we just copy mProcList to mOpsVec.
	//
	// This means that when writing DSP graphs in XML, you must specify the
	// procs in the order that they are to be run.
	//
	// reads proc list, writes ops list
	for (std::list<MLProcPtr>::iterator it = mProcList.begin(); it != mProcList.end(); ++it)
	{
		MLProcPtr p = (*it);		
		mOpsVec.push_back(p.get());
	}

	// ----------------------------------------------------------------
	// translate ops list to compiled signal graph 
	//
	std::list<compileOp> compileOps;
	std::map<MLSymbol, compileOp*> compileOpsMap;	
	std::vector<MLSymbol> compileInputs;
	std::vector<MLSymbol> compileOutputs;
	std::map<MLSymbol, compileSignal> signals;
	MLNameMaker nameMaker;

	// make compileOps from ops list.
	// reads ops list, writes compile ops list, compile ops map. 
	// for each proc in ops list, 
	for (std::vector<MLProc*>::iterator it = mOpsVec.begin(); it != mOpsVec.end(); ++it)
	{
		MLProc* pRef = *it;
        
        // we get each proc name with the copy index attached because in multicontainers we can have
        // multiple procs with the same name.
		MLSymbol pName = pRef->getName();
        
		// make a new compileOp referencing the proc.
		compileOp c(pRef);
		
		// set number of inputs and outputs of compileOp to mirror the proc.
		c.inputs.resize(pRef->getNumInputs());
		c.outputs.resize(pRef->getNumOutputs());
		
		// mark each compileOp with its position in list
		c.listIdx = compileOps.size();
				
		// push copy of new compileOp to the compile ops list
		compileOps.push_back(c);
		
		// add map entry to reference compileOp by proc name.
		compileOpsMap[pName] = &compileOps.back();

	}

	// ----------------------------------------------------------------
	// name signals and get lifetimes
	
	// name input signals where they enter container:
	// reads published inputs, writes compile ops map, signals, compileinputs
	// for each input signal to this container,
	for(int i = 0; i < (int)mPublishedInputs.size(); ++i)
	{		
		// get destination proc and index of input
		MLPublishedInputPtr input = mPublishedInputs[i];
		MLProcPtr proc = input->mDest;	
		int inputIdx = input->mDestInputIndex;	
		
		// this makes numbered copies of voices resolve to the same compile signal.
		// this works because these copies are always processed as one chunk.
        // TODO but... what about outputs from an individual copy? this is not working right.
		MLSymbol pName = proc->getName();
        
		// set corresponding input of proc in ops map to a new compileSignal.
		compileOp* pOp = compileOpsMap[pName];
		if (pOp)
		{
			MLSymbol sigName = nameMaker.nextName();
			signals[sigName] = (compileSignal());
			pOp->inputs[inputIdx - 1] = sigName;
			
			// set lifespan of input signal, from start to op position.
			signals[sigName].setLifespan(0, pOp->listIdx);
			signals[sigName].mPublishedInput = i + 1;
			compileInputs.push_back(sigName);
		}
		else
		{
			debug() << "error: MLProcContainer " << getName() << " ::compile(): no compile op named " << pName << "\n";
		}
	}

	// name internal signals and
	// get lifetimes of all used signals:
	// reads pipe list, compile ops map
	// writes compile ops map, signals
	// for each pipe		
	for (std::list<MLPipePtr>::iterator i = mPipeList.begin(); i != mPipeList.end(); ++i)
	{
		MLPipePtr pipe = (*i);
		MLSymbol srcName = pipe->mSrc->getName();
		int srcIndex = pipe->mSrcIndex;
		MLSymbol destName = pipe->mDest->getName();
		int destIndex = pipe->mDestIndex;
        
		// MLTEST
        debug() << "compile() ADDING pipe: " << srcName << " (" << srcIndex << ")  -> " << destName << " (" << destIndex << ")\n";
        
		// resize inputs and outputs if needed for variable i/o procs
		compileOp* pSrcOp = compileOpsMap[srcName];
		compileOp* pDestOp = compileOpsMap[destName];
		
		if((!pSrcOp) || (!pDestOp))
		{
			debug() << "compile(): NULL operation!\n";
		}
        else
		{
			if ((int)pSrcOp->outputs.size() < srcIndex)
			{
				pSrcOp->outputs.resize(srcIndex);
			}
			if ((int)pDestOp->inputs.size() < destIndex)
			{
				pDestOp->inputs.resize(destIndex);
			}

			// if compileOpsMap output corresponding to pipe start has not yet been marked,
			MLSymbol* pPipeStartSym = &(compileOpsMap[srcName]->outputs[srcIndex - 1]);
			MLSymbol* pPipeEndSym = &(compileOpsMap[destName]->inputs[destIndex - 1]);
			MLSymbol sigName;

			if (!*pPipeStartSym)
			{		
				// add a new compileSignal to map
				sigName = nameMaker.nextName();
				signals[sigName] = (compileSignal());

				// mark the start and end of the pipe with the new signal.
				*pPipeStartSym = sigName;			
				*pPipeEndSym = sigName;		
			}
			else
			{
				sigName = *pPipeStartSym;
				*pPipeEndSym = *pPipeStartSym;		
			}
			
			// get pipe extent
			int pipeStartIdx = compileOpsMap[srcName]->listIdx;
			int pipeEndIdx = compileOpsMap[destName]->listIdx;

			// debug() << "adding span for " << sigName << ": [" << pipeStartIdx << ", " <<  pipeEndIdx << "]\n";
			
			// set signal lifetime to union of signal lifetime and pipe extent
			signals[sigName].addLifespan(pipeStartIdx, pipeEndIdx);
		}
		// TODO change MLPipes to store proc name symbols, not procPtrs.
	}
	
	// name output signals where they exit container, get lifespans:
	// reads published outputs, container procs (for name), compile ops map
	// writes compile ops map, signals
	// for each output signal from this container,    
	for(int i = 0; i < (int)mPublishedOutputs.size(); ++i)
	{
 		// get src proc and index of output
		MLPublishedOutputPtr output = mPublishedOutputs[i];
		MLProcPtr outputProc = output->mSrc;	
		int outputIdx = output->mSrcOutputIndex;	
		MLSymbol outputProcName = outputProc->getName();

        compileOp* pOutputOp = compileOpsMap[outputProcName];
		if (!pOutputOp)
        {
            debug() << "compile error: can’t connect output for proc " << outputProcName << " !\n";
        }
        else
        {
            MLSymbol sigName = pOutputOp->outputs[outputIdx - 1];
            
            // if output wasn't previously connected to anything
            if (!sigName)
            {
                // add a new compileSignal to map
                sigName = nameMaker.nextName();
                signals[sigName] = (compileSignal());
            
                // mark the output source with the new signal.
                compileOpsMap[outputProcName]->outputs[outputIdx - 1] = sigName;
            }

            // set corresponding output of proc in ops map to name of compileSignal.
            compileOp* pOp = compileOpsMap[outputProcName];
            pOp->outputs[outputIdx - 1] = sigName;		

            // set lifespan of output signal, from op's position to end.
            signals[sigName].addLifespan(pOp->listIdx, mOpsVec.size() - 1);
            // debug() << "    adding output span for " << sigName << ": [" << 0 << ", " <<  mOpsVec.size() - 1 << "]\n";
            
            // add published output to list
            signals[sigName].mPublishedOutput = i + 1;
            compileOutputs.push_back(sigName);
        }
	}
	
	// ----------------------------------------------------------------
	// recurse

	// depth first recurse into container subprocs
	for (std::vector<MLProc*>::iterator i = mOpsVec.begin(); i != mOpsVec.end(); ++i)
	{
		MLProc* p = (*i);
		if (p->isContainer())
		{
			MLProcContainer& pc = static_cast<MLProcContainer&>(*p);
			pc.compile();
		}
	}

	// ----------------------------------------------------------------
	// allocate a buffer for each internal or output signal in signal map.
	// if signal is an input, set to null signal awaiting input.
	//
	// reads signals, published outputs, procs
	// writes compile signals
	//	
	std::list<sharedBuffer> sharedBuffers;
	
	for (std::map<MLSymbol, compileSignal>::iterator it = signals.begin(); it != signals.end(); ++it)
	{
		MLSymbol sigName = ((*it).first);
		compileSignal* pCompileSig = &((*it).second);
		bool needsBuffer = true;
		
		if (pCompileSig->mPublishedInput > 0) 
		{		
			pCompileSig->mpSigBuffer = &getNullInput();
			needsBuffer = false;
		}
		else if (pCompileSig->mPublishedOutput > 0) 
		{		
			// get src proc and index of output
			int i = pCompileSig->mPublishedOutput;
            if(i <= (int)mPublishedOutputs.size())
            {
                MLPublishedOutputPtr output = mPublishedOutputs[i - 1];
                MLProcPtr outputProc = output->mSrc;	
                int outputIdx = output->mSrcOutputIndex;				

                // has a signal been allocated?
                if(outputProc->outputIsValid(outputIdx))
                {
                    pCompileSig->mpSigBuffer = &outputProc->getOutput(outputIdx);
                    needsBuffer = false;
                }
                else
                {
                    needsBuffer = true;
                }
            }
            else
            {
                // TODO this is not very informative. Try to explain what is happening at a higher level.
                debug() << "MLProcContainer::compile(): bad published output in " << getName() << " for signal " << sigName << "\n";
                debug() << "    (" << i + 1 << " of " << mPublishedOutputs.size() << ")\n";
            }
		}
		else 
		{	
			needsBuffer = true;
		}
		
		if (needsBuffer)
		{
			//packUsingWastefulAlgorithm(pCompileSig, sharedBuffers);
			packUsingFirstFitAlgorithm(pCompileSig, sharedBuffers);
		}
	}
	
	// ----------------------------------------------------------------
	// allocate

	// for each shared buffer made, allocate a new MLSignal buffer and point each compileSignal it contains to the new buffer.
	for (std::list<sharedBuffer>::const_iterator it = sharedBuffers.begin(); it != sharedBuffers.end(); ++it)
	{
		const sharedBuffer& buf = (*it);
		MLSignal* newBuf = allocBuffer();
		
		for (std::list<compileSignal*>::const_iterator jt = buf.mSignals.begin(); jt != buf.mSignals.end(); ++jt)
		{		
			compileSignal* sig = (*jt);
			sig->mpSigBuffer = newBuf;
		}
	}
	
	// ----------------------------------------------------------------
	// translate compiled signal graph back to ops list

	// resize proc i/o and set outputs to allocated buffers
	// reads compile ops, signals
	// writes procs
	// for each op in compile ops,
	for (std::list<compileOp>::const_iterator it = compileOps.begin(); it != compileOps.end(); ++it)
	{
		const compileOp& op = (*it);
	
		op.procRef->resizeInputs(op.inputs.size());
		op.procRef->resizeOutputs(op.outputs.size());
        
		// for each output of compile op, set output of proc to allocated buffer or null signal.
		for(int i=0; i<(int)op.outputs.size(); ++i)
		{
			MLSymbol sigName = op.outputs[i];
			MLSignal* pOutSig;
			if(sigName) 
			{
				pOutSig = signals[sigName].mpSigBuffer;
			}
			else
			{
				pOutSig = &getNullOutput();
			}
			op.procRef->setOutput(i + 1, *pOutSig);
		}
	}
	
	// set up connections between procs using allocated buffers
	// for each pipe		
	for (std::list<MLPipePtr>::iterator i = mPipeList.begin(); i != mPipeList.end(); ++i)
	{
		MLPipePtr pipe = (*i);		// TODO pipes use names, not pointers
		connectProcs(pipe->mSrc, pipe->mSrcIndex, pipe->mDest, pipe->mDestIndex);		
	}	

	const MLRatio myRatio = getResampleRatio();
	bool resampling = (!myRatio.isUnity());
	
	// setup this container's published outputs
	// reads compileoutputs, signals
	// writes MLProc outputs, output resamplers    
    
	for(int i=0; i<(int)compileOutputs.size(); ++i)
	{
		MLSymbol outName = compileOutputs[i];
		
		if (resampling)
		{
			// set up output resampler connection to allocated buffer
			MLProcPtr pR = mOutputResamplers[i];
			pR->setInput(1, *signals[outName].mpSigBuffer);
			
			// make new buffer for output
			pR->setOutput(1, *allocBuffer());
			
			// set resampler to inverse of our ratio
			pR->setParam("ratio_top", (float)myRatio.bottom);
			pR->setParam("ratio_bottom", (float)myRatio.top);
			pR->setParam("up_order", (float)getResampleUpOrder());
			pR->setParam("down_order", (float)getResampleDownOrder());
			pR->setup();
			
			// connect resampler output to main output
			setOutput(i + 1, pR->getOutput());
		}
		else
		{
			// connect src proc to main output
			setOutput(i + 1, *signals[outName].mpSigBuffer);
        }
	}
    
	// ----------------------------------------------------------------
	// dump some things:
    
    if(dumpOutputs)
	{
        // MLPublishedOutput
        MLPublishedOutputMapT::const_iterator it = mPublishedOutputMap.begin();
        debug() << mPublishedOutputMap.size() << " outputs:\n";
        for(; it != mPublishedOutputMap.end(); it++)
        {
            MLPublishedOutputPtr p = it->second;
            debug() << "[" << p->mName << ": " << p->mProc->getNameWithCopyIndex() << " " << p->mOutput << "] ";
#if DEBUG
            MLSymbol name = it->first;
            assert(name == p->mName);
#endif
        }
        debug() << "NULL: " << &getNullOutput() << "\n";
        debug() << "\n";
    }
	if (verbose)
	{
		// dump compile graph
		debug() << "\n\ncontainer " << getNameWithCopyIndex() << "\n";
		debug() << compileOps.size() << " operations: ----------------------------------------------------------------\n";
		int opIdx = 0;
		for (std::list<compileOp>::const_iterator it = compileOps.begin(); it != compileOps.end(); ++it)
		{
			const compileOp& op = (*it);
			debug() << opIdx++ << ": " << op << "\n";
		}	
		
		// dump signals
		debug() << signals.size() << " signals: ----------------------------------------------------------------\n";
		for (std::map<MLSymbol, compileSignal>::iterator it = signals.begin(); it != signals.end(); ++it)
		{
			MLSymbol sigName = ((*it).first);
			const compileSignal& sig = ((*it).second);
			debug() << sigName << ": life[" << sig.mLifeStart << ", " << sig.mLifeEnd << "] ";
			debug() << ", buffer = " <<  static_cast<void *>(sig.mpSigBuffer);
			if(sig.mPublishedInput)
			{
				debug() << " (input " << sig.mPublishedInput << ")";
			}
			if(sig.mPublishedOutput)
			{
				debug() << " (output " << sig.mPublishedOutput << ")";
			}
			debug() << "\n";
		}
		
		// dump stats
		if (e != OK)
		{
			printErr(e);
		}
		else
		{
	//		setEnabled(true); // WAT
			debug() << "compile done: " << mOpsVec.size() << " subprocs.\n";
		}
		
		// dump buffers
		debug() << sharedBuffers.size() << " buffers: ----------------------------------------------------------------\n";
		int nBufs = 0;
		for (std::list<sharedBuffer>::const_iterator it = sharedBuffers.begin(); it != sharedBuffers.end(); ++it)
		{
			const sharedBuffer& buf = (*it);
			nBufs++;			
			debug() << "buf " << nBufs << ": " << buf << "\n";
		}
	}
}

bool sharedBuffer::canFit(compileSignal* pSig)
{
	bool r;
	int a = pSig->mLifeStart;
	int b = pSig->mLifeEnd;
	
	if (mSignals.size())
	{
		std::list<compileSignal*>::const_iterator it;
		
		// walk to signal element with start > b
		for(it = mSignals.begin(); it != mSignals.end(); ++it)
		{
			if((*it)->mLifeStart > b) break;
		}
		
		if (it != mSignals.begin())
		{
			// get previous element
			it--;
		}
		
		// if end of previous sorted element is less than 
		// our start, we fit here. 
		r = (*it)->mLifeEnd < a;
	}
	else
	{
		r = true;
	}
	return r;
}

void sharedBuffer::insert(compileSignal* pSig)
{
	int b = pSig->mLifeEnd;
	
	std::list<compileSignal*>::iterator it;
	
	// walk to end or c > b
	for(it = mSignals.begin(); it != mSignals.end(); ++it)
	{
		if((*it)->mLifeStart > b) break;
	}
	
	// insert
	mSignals.insert(it, pSig);
}

void packUsingWastefulAlgorithm(compileSignal* pSig, std::list<sharedBuffer>& bufs)
{
	// always make new sharedBuffer.
	sharedBuffer newBuf;
	newBuf.insert(pSig);
	bufs.push_back(newBuf); // copy
}

void packUsingFirstFitAlgorithm(compileSignal* pSig, std::list<sharedBuffer>& bufs)
{
	bool found = false;
	std::list<sharedBuffer>::iterator it;
	
	for(it = bufs.begin(); it != bufs.end(); ++it)
	{
		if((*it).canFit(pSig)) 
		{
			found = true;
			break;
		}
	}

	if (found) 
	{
		(*it).insert(pSig);
	}
	else
	{
		sharedBuffer newBuf;
		newBuf.insert(pSig);
		bufs.push_back(newBuf); // copy
	}
}

// recurse on containers, preparing each proc.
MLProc::err MLProcContainer::prepareToProcess()
{
	MLProc::err e = MLProc::OK;
	
	int containerSize = getContextVectorSize();
	float containerRate = getContextSampleRate();
	const MLRatio myRatio = getResampleRatio();
	
	int mySize, ins, outs;
	float myRate;
	MLRatio mySizeAsRatio;
	mySizeAsRatio = MLRatio(containerSize) * myRatio;

	if (!mySizeAsRatio.isInteger())
	{
		e = fractionalBlockSizeErr;
	}
	else
	{
		mySize = mySizeAsRatio.top;
		myRate = (containerRate*myRatio);		
		setVectorSize(mySize);
		setSampleRate(myRate);

		// prepare all subprocs
		for (std::vector<MLProc*>::iterator i = mOpsVec.begin(); i != mOpsVec.end(); ++i)
		{
			MLProc* p = (*i);
			e = p->prepareToProcess();
			if(e != MLProc::OK)	break;
		}
		
		// prepare all output buffers
		outs = getNumOutputs();
		for (int i=1; i <= outs; ++i)
		{
			// leave output alone if marked timeless
			MLSignal& y = getOutput(i);
			if(&(y) == nullptr) // should be impossible, but happens with bad graphs
			{
				if (y.getRate() != kMLTimeless)
				{
					y.setDims(containerSize);
					y.setRate(containerRate);
				}
			}
		}	
		
		// resize resampler buffers // TODO this undoes prep above, make flag or something
		if (!myRatio.isUnity())
		{
			ins = mPublishedInputs.size();
			outs = mPublishedOutputs.size();
			for(int i=0; i<ins; ++i)
			{
				MLSignal& y = mInputResamplers[i]->getOutput();
				y.setDims(mySize);
				y.setRate(myRate);
				mInputResamplers[i]->resize();
			}
			for(int i=0; i<outs; ++i)
			{
				MLSignal& y = mOutputResamplers[i]->getOutput();
				y.setDims(containerSize);
				y.setRate(containerRate);
				mOutputResamplers[i]->resize();
			}
		}
	}
	
	mClock.stop();
	if (e != OK) printErr(e);
	return e;
}
 
void MLProcContainer::clear()
{
	// clear input resamplers.
	for (std::vector<MLProcPtr>::iterator i = mInputResamplers.begin(); i != mInputResamplers.end(); ++i)
	{
		(*i)->clearProc();
	}
	// iterate through ops list, clearing Processors.
	for (std::vector<MLProc*>::iterator i = mOpsVec.begin(); i != mOpsVec.end(); ++i)
	{
		(*i)->clearProc();
	}
}

// recurse into containers, setting stats ptr and collecting number of procs.
void MLProcContainer::collectStats(MLSignalStats* pStats)
{
	mStatsPtr = pStats;
	if (!isEnabled()) return;
	
	// if nonzero, get info
	if (mStatsPtr)
	{
		int ops = mOpsVec.size();
		pStats->mProcs += ops;
//		debug() << getName() << ": added " << ops << " ops \n";
	}

	// recurse, setting pointer to stats block in DSPEngine
	for (std::vector<MLProc*>::iterator it = mOpsVec.begin(); it != mOpsVec.end(); ++it)
	{
		MLProc* p = *it;
		if(p->isContainer())
		{	
			MLProcContainer& pc = static_cast<MLProcContainer&>(*p);
			pc.collectStats(pStats);
		}
	}
}


#pragma mark -
#pragma mark process

// process signals.
void MLProcContainer::process(const int extFrames)
{
	if (!isEnabled()) return;
	
	const MLRatio myRatio = getResampleRatio();
	const bool resample = !myRatio.isUnity();
	if (myRatio.isZero()) return;
	
	jassert((MLRatio(extFrames) * myRatio).isInteger());
	const int intFrames = (int)(extFrames * myRatio);
	
	mClock.advance(ml::samplesAtRateToTime(intFrames, static_cast<int>(getSampleRate())));
				   
	if (resample)
	{
		int ins = (int)mPublishedInputs.size();
		for(int i=0; i<ins; ++i)
		{
			mInputResamplers[i]->process(extFrames);
		}
	}
	
	// process ops vector, recursing into containers.
	int numOps = mOpsVec.size();
	for(int i = 0; i < numOps; ++i)
	{
		MLProc* p = mOpsVec[i];
		
		// set output buffers to not constant.
		// with this extra step here every proc can safely assume this condition. 
		int outs = p->getNumOutputs();
		for(int i=0; i<outs; ++i)
		{
			p->getOutput(i + 1).setConstant(false);
		}
		
		// process all procs!
		p->process(intFrames);
	}
	
	if (resample)
	{
		int outs = (int)mPublishedOutputs.size();
		for(int i=0; i<outs; ++i)
		{
			mOutputResamplers[i]->process(intFrames);
		}
	}

	// copy to outputs
	for(int i=0; i<(int)mPublishedOutputs.size(); ++i)
	{
		MLSignal& outSig = mPublishedOutputs[i]->mProc->getOutput(mPublishedOutputs[i]->mOutput);
		mOutputs[i]->copy(outSig);
	}
}

void MLProcContainer::clearInput(const int idx)
{	
	MLProc::clearInput(idx);	
	
	int ins = (int)mPublishedInputs.size(); 
	if (idx <= ins)
	{
		MLPublishedInputPtr input = mPublishedInputs[idx-1];
		MLProcPtr proc = input->mProc;
		const int procIdx = input->mProcInputIndex;			
		proc->clearInput(procIdx);
	}
}

// overrides MLProc::setInput to look up published container inputs
MLProc::err MLProcContainer::setInput(const int idx, const MLSignal& sig)
{
	err e = OK;

	// set base class input to sig for quick retrieval 
	// compile() propagates this input signal to subprocs
	e = MLProc::setInput(idx, sig);
		
	if (e == OK)
	{
		// now set input of subproc.
		// maps are made in publishInput.
		
		// TODO ins can be 0 here if graph is not well formed, 
		// leading to possible crash. 
		int ins = mPublishedInputs.size();  
		
		// MLTEST
		if(!idx)
		{
			debug() << "WHOA idx = 0 in " << getName() << " setInput \n"; 
			dumpGraph(0);
		}
		
		if (idx <= ins)
		{
			MLPublishedInputPtr input = mPublishedInputs[idx-1];
			MLProcPtr proc = input->mProc;
			const int procIdx = input->mProcInputIndex;		
			
			
			if(!idx)
			{
				debug() << "input name: " << input->mName << "    " << proc->getName() << " \n"; 
			}
			
			
			e = proc->setInput(procIdx, sig);
		}
		else
		{
			e = noInputErr; 
		}
	}
	return e;
}

// will be > 0 for valid aliases
int MLProcContainer::getInputIndex(const MLSymbol alias) 
{ 
	int r = 0;
	MLPublishedInputPtr p;
	MLPublishedInputMapT::const_iterator it = mPublishedInputMap.find(alias);

	if (it != mPublishedInputMap.end()) 
	{
		p = it->second;
		if (p)
		{
			r = p->mIndex;		
		}
		else
		{
			debug() << "getInputIndex: null ptr for input " << alias << " of proc " << getName() << "\n";
		}
	}
	else
	{
		debug() << "getInputIndex: input " << alias << " of proc " << getName() <<  " not found\n";
	}
	return r;
}


// will be > 0 for valid aliases
int MLProcContainer::getOutputIndex(const MLSymbol alias) 
{ 
	int idx = 0;
	MLPublishedOutputPtr p;	
	MLPublishedOutputMapT::const_iterator it = mPublishedOutputMap.find(alias);

	if (it != mPublishedOutputMap.end()) 
	{
		p = it->second;
		if (p)
		{
			idx = p->mIndex;		
		}
		else
		{
			debug() << "MLProcContainer::getOutputIndex: null ptr for output " << alias << " of proc " << getName() << "\n";
		}
	}
	else
	{
		debug() << "MLProcContainer::getOutputIndex: output " << alias << " of proc " << getName() << " not found\n";
	}
	return idx;
}

int MLProcContainer::getNumProcs() 
{ 
	return mProcList.size();
}

// ----------------------------------------------------------------
#pragma mark graph creation	

// test
void MLProcContainer::dumpMap()
{
	debug() << "dumping map: ------------\n";
	for(MLSymbolProcMapT::iterator it = mProcMap.begin(); it != mProcMap.end(); it++)
	{
		debug() << "key " << it->first.getString() << ", proc " << it->second->getName() << "\n";
	}
}

// make a new instance of a named subclass of MLProc. 
//
MLProcPtr MLProcContainer::newProc(const MLSymbol className, const MLSymbol procName) 
{
	MLProcPtr pNew;
	
	// call factory to get instance of processor class in this context
	pNew = theProcFactory.create(className, this);	
	if (!pNew)
	{
		debug() << "MLProcContainer: newProc: couldn't create!\n";
	}
	else
	{
		pNew->setName(procName);
		pNew->clear();
	}
	return pNew;
}

MLProc::err MLProcContainer::addProc(const MLSymbol className, const MLSymbol procName)
{
	MLProcPtr pNew;
	err e = OK;
		
	// is name in map already?
	MLSymbolProcMapT::iterator it = mProcMap.find(procName);
	if (it == mProcMap.end())
	{
		// if not, call factory to get instance of processor class
		pNew = newProc(className, procName);
		if (pNew)
		{
			mProcMap[procName] = pNew;
			mProcList.push_back(pNew);
			
			// add inputs and outputs to proc if needed. 
			pNew->createInput(pNew->getNumRequiredInputs());
			if (pNew->getNumOutputs() < pNew->getNumRequiredOutputs())
			{
				pNew->resizeOutputs(pNew->getNumRequiredOutputs());
			}
		}
		else
		{
			e = newProcErr;
		}
	}
	else
	{
		debug() << "MLProcContainer: addProc: name " << procName << " already in use!\n";
		e = nameInUseErr;
	}
	
	return e;
}

// TODO return MLProcPtr
MLProc::err MLProcContainer::addProcAfter(MLSymbol className, MLSymbol alias, MLSymbol afterProc)
{
	MLProcPtr pNew;
	err e = OK;
	
	// dies afterProc exist?
	MLSymbolProcMapT::iterator it;
	it = mProcMap.find(afterProc);
	if (it == mProcMap.end())
	{
		debug() << "MLProcContainer::addProcAfter: " << afterProc << " not found in container " << getName() << "!\n";
		return unknownErr;
	}
	
	// is name in map already?
	it = mProcMap.find(alias);
	if (it == mProcMap.end())
	{
		// if not, call factory to get instance of processor class
		pNew = newProc(className, alias);
		if (pNew)
		{
			std::list<MLProcPtr>::iterator jt;
			for(jt = mProcList.begin(); jt != mProcList.end(); ++jt)
			{
				if ((*jt)->getName() == afterProc) 
				{
					break;
				}
			}
			if (jt != mProcList.end())
			{
				// advance one to add after named proc.
				jt++;
			}
			mProcMap[alias] = pNew;
			mProcList.insert(jt, pNew);
				
			// add inputs and outputs to proc if needed. 
			pNew->createInput(pNew->getNumRequiredInputs());
			if (pNew->getNumOutputs() < pNew->getNumRequiredOutputs())
			{
				pNew->resizeOutputs(pNew->getNumRequiredOutputs());
			}
		}
		else
		{
			e = newProcErr;
		}
	}
	else
	{
		debug() << "MLProcContainer: addProcAfter: name " << alias << " already in use!\n";
		e = nameInUseErr;
	}
	
	return e;
}

MLProcPtr MLProcContainer::getProc(const MLPath & path)
{
	MLProcPtr r;
	MLProcPtr headProc;
	MLSymbolProcMapT::iterator it;
	err e = OK;

	const MLSymbol head = path.head();
	const MLPath tail = path.tail();

//debug() << "MLProcContainer(" << (void *)this << ") getProc: " << head << " / " << tail << "\n";
//debug() << "      proc map is (" << (void *)&mProcMap << ")\n";
//dumpMap();

	// look up head Proc in current scope's map
	it = mProcMap.find(head);
	// if found,
	if (it != mProcMap.end())
	{
		headProc = it->second;	
		if (!tail.empty())
		{
			if (headProc->isContainer())  
			{
				// recurse
				r = (static_cast<MLProcContainer&>(*headProc)).getProc(tail);
			}
			else
			{
				debug() << "ack, head proc in name is not container!\n";
				e = headNotContainerErr;
			}		
		}
		else // done.
		{
//debug() << "     found proc " << headProc->getName() << " (" << (void *)&*headProc << ")\n";
			r = headProc;
		}
	}
	else // return null
	{
		e = nameNotFoundErr;
	}

	return r; 
}

// TODO this can't possibly work with multis inside multis, since the copy # is specified
// for the entire path. Fix with new MLPath structure with copy or wildcard per branch.
// if enabledOnly is true, return only enabled procs.
void MLProcContainer::getProcList(MLProcList& pList, const MLPath & pathName, int copies, bool enabledOnly)
{
	pList.clear();
	for(int i=1; i<=copies; ++i)
	{
		MLPath pathI = pathName;
		pathI.setCopy(i);
		MLProcPtr proc = getProc(pathI);		
		if (proc)
		{
 			if(proc->isEnabled() || !enabledOnly)
			{
				pList.push_back(proc);
			}
		}
	}
}


// ----------------------------------------------------------------
// 
// addPipe() creates a new Pipe object and adds it to this container's
// pipe list.  The Pipe represents the graph edge but doesn't 
// otherwise implement anything.  The implementation is done in
// MLProcContainer::connectProcs().
//
void MLProcContainer::addPipe(const MLPath& src, const MLSymbol out, const MLPath& dest, const MLSymbol in)
{
	MLProcPtr srcProc, destProc;
	int srcIdx, destIdx;
	
	srcProc = getProc(src);
	destProc = getProc(dest);
	
	if (srcProc && destProc)
	{
		srcIdx = srcProc->getOutputIndex(out);
		destIdx = destProc->getInputIndex(in);
		
		if (srcIdx && destIdx)
		{
			mPipeList.push_back(MLPipePtr(new MLPipe(srcProc, srcIdx, destProc, destIdx)));
		}
		else
		{
			debug() << "MLProcContainer::addPipe failed";
			if (!srcIdx)
			{
				debug() << ": no src output "<< out << " of proc " << src << " in container " << getName();
			}
			if (!destIdx)
			{
				debug() << ": no dest input "<< in << " of proc " << dest << " in container " << getName();
			}
			debug() << "\n";
		}
	}
	else
	{
		debug() << "MLProcContainer::addPipe failed";
		if (!srcProc)
		{
			debug() << ": no src proc "<< src << " in container " << getName();
		}
		if (!destProc)
		{
			debug() << ": no dest proc "<< dest << " in container " << getName();
		}
		debug() << "\n";
	}
}	


// check that Pipe is doing something reasonable and setup connection
// between procs.
//
MLProc::err MLProcContainer::connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi)
{
	MLProc::err e = MLProc::OK;
	const MLDSPContext* srcContext = a->getContext();
	const MLDSPContext* destContext = b->getContext();		

	if (srcContext != destContext)
	{
		// processors are in different containers, can't connect.
		e = MLProc::connectScopeErr;
		goto bail;
	}
	
	if ((!ai) || (!bi))
	{
		e = MLProc::badIndexErr;
		goto bail;
	}	
	
#if DEBUG
	if (e != OK)
	{
		printErr(e);
        debug() << getName() << ": CONNECTING " <<  a->getName() << " (" << (void *)&(*a) << ") " << "[" << ai <<  "]" ;
        debug() << " ("  << (void *)&a->getOutput(ai) << ")";
        debug() << " to " << b->getName() << " (" << (void *)&(*b) << ") " << "[" << bi << "] ";
        debug() << "\n\n";
	}
#endif
    
	// construct input pointer if needed
	b->createInput(bi);
	
	// TODO fix crashing on ill-formed graphs
	
	e = b->setInput(bi, a->getOutput(ai));
    
bail:	
	return e;
}



// TODO inputs look just like outputs, refactor

// ----------------------------------------------------------------
#pragma mark I/O

void MLProcContainer::publishInput(const MLPath & procName, const MLSymbol inputName, const MLSymbol alias)
{
	err e = OK;
	MLPublishedInputPtr p;
	
	// MLTEST
	//debug() << "MLProcContainer::publishInput: publishing input " << inputName << " of " << procName << " as " << alias << "\n"; 

	const MLProcPtr proc = getProc(procName);	
	const MLRatio myRatio = getResampleRatio();

	if (proc)
	{	
		int inSize = (int)mPublishedInputs.size();
		const int inIndex = proc->getInputIndex(inputName);
					
		if (!myRatio.isUnity()) 
		{
			// make resampler
			MLSymbol resamplerName(getName().getString() + "_resamp_in");
			MLProcPtr resamplerProc = newProc(MLSymbol("resample"), resamplerName.withFinalNumber(inSize + 1));
			
			// would be cleaner to use buildProc() here, but right now that adds the new proc
			// to the ops list by default, and we need resamplers to be first. look at that
			// mess later.
			if (!resamplerProc) { e = newProcErr; goto bail; }
			
			int resamplerInIndex = resamplerProc->getInputIndex("in");
			int resamplerOutIndex = resamplerProc->getOutputIndex("out");
			
			// make output buffer and set resampler output to proc input
			resamplerProc->resizeInputs(resamplerInIndex);			
			resamplerProc->resizeOutputs(resamplerOutIndex);			
			resamplerProc->setOutput(resamplerOutIndex, *allocBuffer());						
			connectProcs(resamplerProc, resamplerOutIndex, proc, inIndex);
			
			// set resampler ratio to our ratio
			resamplerProc->setParam("ratio_top", myRatio.top);
			resamplerProc->setParam("ratio_bottom", myRatio.bottom);
			resamplerProc->setParam("up_order", getResampleUpOrder());
			resamplerProc->setParam("down_order", getResampleDownOrder());
			resamplerProc->setup();
							
			if (resamplerProc)
			{
				// save resampler for use in process()
				mInputResamplers.push_back(resamplerProc);
				resamplerProc->createInput(resamplerInIndex);
				
				// set to a valid input in case graph ends up incomplete
                // TODO: investigate: this is causing inputOccupiedErr sometimes?!
				resamplerProc->setContext(this);
				resamplerProc->setInput(resamplerInIndex, getNullInput());
				
				// publish resampler input
				p = MLPublishedInputPtr(new MLPublishedInput(resamplerProc, resamplerInIndex, inSize + 1));
				
				// set post-resampling destination
				if (p)
				{
					p->setDest(proc, inIndex);	
				}
			}
		}
		else // publish direct link to internal proc.
		{
			p = MLPublishedInputPtr(new MLPublishedInput(proc, inIndex, inSize + 1));
			proc->createInput(inIndex);	
		}
		
		if (p)
		{
			// store by index for setInput()
			// TODO some of mPublishedInputs / mPublishedInputMap mess can go away 
			p->mName = alias;
			mPublishedInputs.push_back(p);	
			
			// store by alias for getInputIndex()
			mPublishedInputMap[alias] = p; 
			
			// if not allocated, make space for input pointer and zero it
			createInput(inSize + 1);	
		}
	}
	else
	{
		debug() << "MLProcContainer::publishInput: proc " << procName << " not found in container " << getName() << "!\n";
	}
bail:
	if (e != OK) printErr(e);
	// TODO return err
}

// publish an output of a subproc by setting one of our output ptrs to the subproc's output signal.
// 
void MLProcContainer::publishOutput(const MLPath & srcProcName, const MLSymbol outputName, const MLSymbol alias)
{	
    err e = OK;
	MLPublishedOutputPtr p;
	const MLProcPtr sourceProc = getProc(srcProcName);
	const MLRatio myRatio = getResampleRatio();
	if (sourceProc)
	{	 
		int outSize = (int)mPublishedOutputs.size();
		const int srcProcOutputIndex = sourceProc->getOutputIndex(outputName);
		if (!srcProcOutputIndex) { e = badIndexErr; goto bail; }
		
		if (!myRatio.isUnity()) 
		{
			// make resampler
			MLSymbol resamplerName(getName().getString() + "_resamp_out");
			MLProcPtr resamplerProc = newProc(MLSymbol("resample"), resamplerName.withFinalNumber(outSize + 1)); 
			if (!resamplerProc) { e = newProcErr; goto bail; }
			
			// setup resampler i/o
			int resamplerInIndex = resamplerProc->getInputIndex("in");
			int resamplerOutIndex = resamplerProc->getOutputIndex("out");
			resamplerProc->resizeInputs(resamplerInIndex);			
			resamplerProc->resizeOutputs(resamplerOutIndex);
			
			// save resampler for use in process()
			mOutputResamplers.push_back(resamplerProc);
			
			// publish resampler output
			p = MLPublishedOutputPtr(new MLPublishedOutput(resamplerProc, resamplerOutIndex, outSize + 1));	
	
			// set pre-resampling source
			if (p)
			{
				p->setSrc(sourceProc, srcProcOutputIndex);	
			}
		}
		else
		{
			// publish source proc output
 			p = MLPublishedOutputPtr(new MLPublishedOutput(sourceProc, srcProcOutputIndex, outSize + 1));
            
            // make outputs in the source proc if needed
			if (srcProcOutputIndex > (int)sourceProc->mOutputs.size())
			{
				sourceProc->resizeOutputs(srcProcOutputIndex);	
			}			
		}
		
		if (p)
		{
			// store by alias for getOutputIndex()
			p->mName = alias;

            MLPublishedOutputMapT::const_iterator it = mPublishedOutputMap.find(alias);            
            if (it == mPublishedOutputMap.end())
            {
                mPublishedOutputMap[alias] = p;
                mPublishedOutputs.push_back(p);
                resizeOutputs(mPublishedOutputs.size());
            }
            else
            {
                debug() << "MLProcContainer::publishOutput: alias " << alias << " already in map for container " << getName() << "!\n";
            }
 		}
	}
	else
	{
		debug() << "MLProcContainer::publishOutput: proc " << srcProcName << " not found in container " << getName() << "!\n";
	}
bail:
	if (e != OK) printErr(e);
	// TODO return err
}

MLSymbol MLProcContainer::getOutputName(int index)
{	
	const int size = (int)mPublishedOutputs.size();
	if (index <= size)
	{
		MLPublishedOutputPtr p = mPublishedOutputs[index - 1];
		return p->mName;
	}
	else
	{
		debug() << "MLProcContainer::getOutputName: output " << index << " not found in container " << getName() << "!\n";
	}
	return MLSymbol();
}

// ----------------------------------------------------------------
#pragma mark published signals - the recursive part

MLProc::err MLProcContainer::addBufferHere(const MLPath & procName, MLSymbol outputName, MLSymbol alias, 
	int trigMode, int bufLength)
{
	err e = OK;
	
// debug() << "add buffer here:" << procName << " called " << alias << " after " << outputName << "\n";

	e = addProcAfter("ringbuffer", alias, procName.head());
	if (e == OK)
	{
		MLProcPtr bufferProc = getProc(MLPath(alias));	
		if (bufferProc)
		{
			bufferProc->setParam("length", bufLength);
			bufferProc->setParam("mode", trigMode);
			bufferProc->setup();
			
			// connect published output of head proc to ringbuffer input
			addPipe(procName, outputName, MLPath(alias), MLSymbol("in"));
		}
	}
	return e;
}

// recurse into graph, adding ring buffers where necessary to capture signals matching procAddress.
// this is necessary to get multiple signals that resolve to the same address.
//
MLProc::err MLProcContainer::addSignalBuffers(const MLPath & procAddress, const MLSymbol outputName, 
	const MLSymbol alias, int trigMode, int bufLength)
{
	err e = OK;

	MLProcPtr headProc;
	MLSymbolProcMapT::iterator it;

	const MLSymbol head = procAddress.head();
	const MLPath tail = procAddress.tail();
	//const int copy = procAddress.getCopy();
	//debug() << "MLProcContainer " << getName() << " addSignalBuffers to " << procAddress << "\n";
	
	// look up head Proc in current scope's map
	it = mProcMap.find(head);
	if (it != mProcMap.end())
	{
		headProc = it->second;	
		if (!tail.empty())
		{
			if (headProc->isContainer())  
			{
				// recurse
				MLProcContainer& headContainer = static_cast<MLProcContainer&>(*headProc);
				headContainer.addSignalBuffers(tail, outputName, alias, trigMode, bufLength);
			}
			else
			{
				debug() << "MLProcContainer::addSignalBuffers: ack, head proc " << head << " is not container!\n";
			}		
		}
		else // create buffers.
		{		
			if (outputName.hasWildCard())
			{		
				// debug() << "addSignalBuffers: wild card\n";
				// add a buffer for each possible output matching wildcard (quick and dirty)
				for(int i = 1; i <= kMLEngineMaxVoices; ++i)
				{				
					if (headProc->getOutputIndex(outputName.withWildCardNumber(i)))
					{
						addBufferHere(MLPath(head), outputName.withWildCardNumber(i), alias.withWildCardNumber(i), trigMode, bufLength);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				addBufferHere(MLPath(head), outputName, alias, trigMode, bufLength);
			}
		}
	}
	else 
	{
		debug() << "MLProcContainer::addSignalBuffers: proc " << head << " not found in container " << getName() << "!\n";
	}
	return e;
}

//
// recurse into graph, gathering signals matching procAddress into a list.
//
void MLProcContainer::gatherSignalBuffers(const MLPath & procAddress, const MLSymbol alias, MLProcList& signalBuffers)
{
	MLProcPtr headProc;
	MLSymbolProcMapT::iterator it;
	const MLSymbol head = procAddress.head();
	const MLPath tail = procAddress.tail();

//	debug() << "MLProcContainer " << getName() << " gatherSignalBuffers " << procAddress << " as " << alias << "\n";
	
	// look up head Proc in current scope's map
	it = mProcMap.find(head);
	if (it != mProcMap.end())
	{
		headProc = it->second;	
		if (!tail.empty())
		{
			if (headProc->isContainer())  
			{
				// recurse
				MLProcContainer& headContainer = static_cast<MLProcContainer&>(*headProc);
				headContainer.gatherSignalBuffers(tail, alias, signalBuffers);
			}
			else
			{
				debug() << "MLProcContainer::gatherSignalBuffers: ack, head proc " << head << " is not container!\n";
			}		
		}
		else 
		{
			// get container of last head proc
			MLProcContainer& context = static_cast<MLProcContainer&>(*headProc->getContext());		
			if (alias.hasWildCard())
			{		
				/// debug() << "gatherSignalBuffers: wild card\n";
				// gather each buffer matching wildcard (quick and dirty)
				for(int i = 1; i <= kMLEngineMaxVoices; ++i)
				{
					MLProcPtr bufferProc = context.getProc(MLPath(alias.withWildCardNumber(i)));		
					if (bufferProc)	
					{
						signalBuffers.push_back(bufferProc);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				MLProcPtr bufferProc = context.getProc(MLPath(alias));		
				if (bufferProc)	
				{
					signalBuffers.push_back(bufferProc);
				}
			}
		}
	}
	else 
	{
		debug() << "MLProcContainer::gatherSignalBuffers: proc " << head << " not found in container " << getName() << "!\n";
	}
}

// ----------------------------------------------------------------
#pragma mark parameters

// return a new MLPublishedParamPtr that can be called upon to set the given param.
// 
MLPublishedParamPtr MLProcContainer::publishParam(const MLPath & procPath, const MLSymbol param, const MLSymbol alias, const MLSymbol type)
{
	MLPublishedParamPtr p;	
	const int i = (int)mPublishedParams.size();
	p = MLPublishedParamPtr(new MLPublishedParam(procPath, param, alias, type, (int)i));
	mPublishedParams.push_back(p);
	// debug() << "\nMLProcContainer::publishParam: pushed back " << p->getType() << " param idx " << i << "\n"; /
	mPublishedParamMap[alias] = p; 
	return p;
}

void MLProcContainer::addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const MLSymbol paramName)
{
	p->addAddress(procName, paramName);
}

void MLProcContainer::setPublishedParam(int index, const MLProperty& val)
{
	const int size = (int)mPublishedParams.size();
	if (index < size)
	{
		MLPublishedParamPtr p = mPublishedParams[index];
		if (p)
		{
			p->setValueProperty(val);
			
			for(MLPublishedParam::AddressIterator it = p->beginAddress(); it != p->endAddress(); ++it)
			{		
				routeParam(it->procAddress, it->paramName, p->getValueProperty());
			}
		}
	}
}

MLParamValue MLProcContainer::getParam(const MLSymbol alias)
{
	MLParamValue r = 0.f;
	MLPublishedParamPtr p;	// default constructor sets up bool test
	MLPublishedParamMapT::const_iterator it = mPublishedParamMap.find(alias);
	if (it != mPublishedParamMap.end()) 
	{
		p = it->second;
		if (p)
		{	
			r = p->getValue();		
		}
		else
		{
			debug() << "MLProcContainer::getParam: null param ptr for " << alias << "\n";
		}
	}
	else // use our own param map.
	// TODO ???
	{
		r = MLProc::getParam(alias);
	}
	return r;
}

// perform our node's part of sending the parameter to the address.  if the address tail
// is empty, we are done-- look for the named proc and set the param.
// TODO verify why this doesn't just use getProcList().
void MLProcContainer::routeParam(const MLPath & procAddress, const MLSymbol paramName, const MLProperty& val)
{
	MLProcPtr headProc;
	MLSymbolProcMapT::iterator it;

	const MLSymbol head = procAddress.head();
	const MLPath tail = procAddress.tail();
	
	// debug() << "MLProcContainer(" << (void *)this << ") routeParam: " << head << " / " << tail << "\n";
	
	// look up head Proc in current scope's map
	it = mProcMap.find(head);
	// if found,
	if (it != mProcMap.end())
	{
		headProc = it->second;	
		if (!tail.empty())
		{
			if (headProc->isContainer())  
			{
				// recurse
				MLProcContainer& headContainer = static_cast<MLProcContainer&>(*headProc);
				headContainer.routeParam(tail, paramName, val);
			}
			else
			{
				debug() << "ack, head proc in param address is not container!\n";
			}		
		}
		else // should be done.
		{
			headProc->setParam(paramName, val);
		}
	}
	else 
	{
		if (head == MLSymbol("this"))
		{
			MLProc::setParam(paramName, val);
		}
		else
		{
			debug() << "MLProcContainer::routeParam: proc " << head << " not found in container " << getName() << "!\n";
		}
	}
}

// ----------------------------------------------------------------
#pragma mark engine params

MLPublishedParamPtr MLProcContainer::getParamPtr(int index) const
{
	MLPublishedParamPtr p;
	const int size = (int)mPublishedParams.size();
	if (index < size)
	{
		p = mPublishedParams[index];
	}
	return p;
}

int MLProcContainer::getParamIndex(const MLSymbol paramName)
{
	int r = -1;
	MLPublishedParamPtr p;	// default constructor sets up bool test
	MLPublishedParamMapT::const_iterator it = mPublishedParamMap.find(paramName);
	if (it != mPublishedParamMap.end()) 
	{
		p = it->second;
		if (p)
		{
			r = p->getIndex();		
		}
		else
		{
			debug() << "MLProcContainer::getParamIndex: null param ptr for " << paramName << "\n";
		}
	}
	else
	{
		// debug() << "MLProcContainer::getParamIndex: param " << paramName << " not found!\n";
	}
	return r;
}

const std::string& MLProcContainer::getParamGroupName(int index)
{
	return mParamGroups.getGroupName(index);
}

MLParamValue MLProcContainer::getParamByIndex(int index)
{
	MLParamValue r = 0.f;
	const int size = (int)mPublishedParams.size();
	if (within(index, 0, size))
	{
		MLPublishedParamPtr p = mPublishedParams[index];
		r = p->getValue();
//		debug() << "param " << index << " is " << r << "\n";	
	}
	else
	{
		debug() << "MLProcContainer::getParamByIndex *** param index out of range!\n";
	}
	return r;
}

int MLProcContainer::getPublishedParams()
{
	return mPublishedParams.size();
}

// ----------------------------------------------------------------
#pragma mark xml loading / saving
// TODO go back to tinyXML to got rid of Juce dependencies
// TODO ditch XML altogether and make scriptable with e.g. Javascript

MLSymbol stringToSymbol(const juce::String& str);
MLPath stringToPath(const juce::String& str);

MLSymbol stringToSymbol(const juce::String& str)
{
	return MLSymbol(static_cast<const char *>(str.toUTF8()));
}

MLPath stringToPath(const juce::String& str)
{
	return MLPath(static_cast<const char *>(str.toUTF8()));
}

void MLProcContainer::scanDoc(juce::XmlDocument* pDoc, int* numParameters)
{
	juce::ScopedPointer<juce::XmlElement> pElem(pDoc->getDocumentElement());
	if (pElem)
	{
		*numParameters = countPublishedParamsInDoc(pElem);
		
		/*
		/// TEST dump parsed XML
		String test = pElem->createDocument("TEST");
		debug() << "**************\n";
		debug() << test;
		*/
	}
	else
	{	
		juce::String error = pDoc->getLastParseError();
		error << "description parse error: " << error << "\n";
	}
}

MLSymbol MLProcContainer::RequiredAttribute(juce::XmlElement* parent, const char * name)
{
	if (parent->hasAttribute(name))
	{
		return stringToSymbol(parent->getStringAttribute(name));	
	}
	else
	{
		debug() << parent->getTagName() << ": required attribute " << name << " missing \n";
		return MLSymbol();
	}
}

MLPath MLProcContainer::RequiredPathAttribute(juce::XmlElement* parent, const char * name)
{
	if (parent->hasAttribute(name))
	{
		return stringToPath(parent->getStringAttribute(name));	
	}
	else
	{
		debug() << parent->getTagName() << ": required path attribute " << name << " missing \n";
		return MLPath();
	}
}

// build graph of the given element.
void MLProcContainer::buildGraph(juce::XmlElement* parent)
{
	if (!parent) return;

	forEachXmlChildElement(*parent, child)	
	{
		if (child->hasTagName("rootproc"))
		{
			buildGraph(child);
		}
		else if (child->hasTagName("proc"))
		{
			buildProc(child);
		}
		else if (child->hasTagName("input"))
		{
			MLPath arg1 = RequiredPathAttribute(child, "proc");
			MLSymbol arg2 = RequiredAttribute(child, "input");
			MLSymbol arg3 = RequiredAttribute(child, "alias");
			if (arg1 && arg2 && arg3)
			{
				// add optional copy attribute
				int copy = 0;
				copy = child->getIntAttribute("copy", copy);
				arg1.setCopy(copy);
				publishInput(arg1, arg2, arg3);
			}
		}
		else if (child->hasTagName("output"))
		{
			MLPath arg1 = RequiredPathAttribute(child, "proc");
			MLSymbol arg2 = RequiredAttribute(child, "output");
			MLSymbol arg3 = RequiredAttribute(child, "alias");
			if (arg1 && arg2 && arg3)
			{
				// add optional copy attribute
				int copy = 0;
				copy = child->getIntAttribute("copy", copy);
				arg1.setCopy(copy);
				publishOutput(arg1, arg2, arg3);
			}
		}
		else if (child->hasTagName("connect"))
		{
			MLPath arg1 = RequiredPathAttribute(child, "from");
			MLSymbol arg2 = RequiredAttribute(child, "output");
			MLPath arg3 = RequiredPathAttribute(child, "to");
			MLSymbol arg4 = RequiredAttribute(child, "input");
			
			if (arg1 && arg2 && arg3 && arg4)
			{
				addPipe(arg1, arg2, arg3, arg4);
			}
		}
		else if (child->hasTagName("paramgroup"))
		{
			MLSymbol arg1 = RequiredAttribute(child, "name");
			if (arg1)
			{
				mParamGroups.setGroup(arg1);
			
				// recurse					
				buildGraph(child);
			}
		}				
		else if (child->hasTagName("param"))
		{
			MLPath arg1 = RequiredPathAttribute(child, "proc");
			MLSymbol arg2 = RequiredAttribute(child, "param");
			MLSymbol arg3 = RequiredAttribute(child, "alias");
			
			if (arg1 && arg2 && arg3)
			{
				// optional param type attribute
				MLSymbol type = stringToSymbol(child->getStringAttribute("type"));
				
				// publish param and set attributes
				MLPublishedParamPtr p = publishParam(arg1, arg2, arg3, type);
				MLSymbol createdType = p->getType();
				if (createdType == "float")
				{
					setPublishedParamAttrs(p, child);
					setPublishedParam(p->mIndex, MLProperty(p->getDefault()));
					mParamGroups.addParamToCurrentGroup(p);
				}
				else if (createdType == "string")
				{
					setPublishedParamAttrs(p, child);
					mParamGroups.addParamToCurrentGroup(p);
				}
				else if (createdType == "signal")
				{
					setPublishedParamAttrs(p, child);
					mParamGroups.addParamToCurrentGroup(p);
				}
			}
		}
	}
}

MLProc::err MLProcContainer::buildProc(juce::XmlElement* parent)
{
	err e = OK;
	const MLSymbol newProcClass ((const char *)parent->getStringAttribute("class").toUTF8());
	const MLSymbol newProcName ((const char *)parent->getStringAttribute("name").toUTF8());

	// debug() << "MLProcContainer::buildProc (class=" << newProcClass << ", name=" << newProcName << ")\n";
	// debug() << "MLProcContainer::buildProc (class=" << parent->getStringAttribute("class") << ", name=" << parent->getStringAttribute("name") << ")\n";

	// add the specified proc to this container.  if this container is a multiple, 
	// MLProcMultiple::addProc makes a MultProxy here to manage the copies. 

	e = addProc(newProcClass, newProcName);		
	if (e == MLProc::OK)
	{
		MLPath newProcPath(newProcName);	
		
		setProcParams(newProcPath, parent);

		// within multiples, this gets the appropriate multproxy class.
		MLProcPtr p = getProc(newProcPath);	
		if(p)
		{	
			p->setup();			
			if (p->isContainer())
			{		
				MLProcContainer* pc = static_cast<MLProcContainer*>(&(*p));
				pc->buildGraph(parent);
			}
		}		
		else
		{
			debug() << "MLProcContainer::buildProc: getProc failed for new proc!\n";
		}
	}
	
	return e;
}

void MLProcContainer::setProcParams(const MLPath& procName, juce::XmlElement* parent)
{
	MLProcPtr p;
	int ac = 0;

	const int numAttrs = parent->getNumAttributes();
	
	// attrs to ignore
	std::string classStr("class");
	std::string nameStr("name");
	
	p = getProc(procName);
	if (p)
	{
		// parse new proc's parameter attributes
		for(int i=0; i<numAttrs; ++i)
		{
			const juce::String& attrName = parent->getAttributeName(i);
			
			// set
			bool isClass = (!classStr.compare(attrName.toUTF8()));
			bool isName = (!nameStr.compare(attrName.toUTF8()));
			MLParamValue paramVal;
			
			if (!isClass && !isName) // TODO a better way of ignoring certain attributes
			{
				paramVal = (MLParamValue)parent->getDoubleAttribute(attrName);
				p->setParam((const char *)attrName.toUTF8(), paramVal);
			}
			++ac;
		}
	}
	else
	{
		debug() << "MLProcContainer::setProcParams: getProc failed!\n";
	}
}

// set up any attributes that a parameter might have. we don't recurse into param elements.
void MLProcContainer::setPublishedParamAttrs(MLPublishedParamPtr p, juce::XmlElement* parent)
{
	std::string valStr;
	
	forEachXmlChildElement(*parent, child)	
	{
		if (child->hasTagName("range"))
		{
			MLParamValue low = 0.f;
			MLParamValue high = 1.f;
			MLParamValue interval = 0.01f;
			int logAttr = 0;
			MLParamValue zeroThresh = -2<<16;
			low = (MLParamValue)child->getDoubleAttribute("low", low);
			high = (MLParamValue)child->getDoubleAttribute("high", high);
			interval = (MLParamValue)child->getDoubleAttribute("interval", interval);
			logAttr = child->getIntAttribute("log", logAttr);
			zeroThresh = (MLParamValue)child->getDoubleAttribute("zt", zeroThresh);
			p->setRange(low, high, max(interval, 0.001f), MLParamValue(logAttr != 0), zeroThresh);
		}
		else if(child->hasTagName("default"))
		{
			p->setDefault((MLParamValue)child->getDoubleAttribute("value", 0.f));
		}
		else if(child->hasTagName("alsosets"))
		{
			addSetterToParam(p, stringToPath(child->getStringAttribute("proc")),
							 stringToSymbol(child->getStringAttribute("param")));
		}
		else if(child->hasTagName("size"))
		{
			MLSymbol type = p->getType();
			if(type == "signal")
			{
				// create storage for the signal parameter.
				int width = 1;
				int height = 1;
				int depth = 1;
				width = child->getIntAttribute("width", width);
				height = child->getIntAttribute("height", height);
				depth = child->getIntAttribute("depth", depth);
				p->setValueProperty(MLSignal(width, height, depth));
			}
		}
		else if(child->hasTagName("length"))
		{
			MLSymbol type = p->getType();
			if(type == "string")
			{
				// create storage for the string parameter.
				int len = 256;
				len = child->getIntAttribute("length", len);
				p->setValueProperty(std::string((size_t)len, '\0'));
			}
		}
		else if(child->hasTagName("automatable"))
		{
			p->setAutomatable(child->getIntAttribute("value", 0));
		}
	}
}

// count param elements, but just at this level-- don't recurse into procs.
// do recurse into paramgroup elements. 
int MLProcContainer::countPublishedParamsInDoc(juce::XmlElement* parent)
{
	int sum = 0;
	if (!parent) return 0;

	forEachXmlChildElement(*parent, child)	
	{	
		if (child->hasTagName("rootproc"))
		{
			sum += countPublishedParamsInDoc(child);
		}
		else if (child->hasTagName("paramgroup"))
		{
			sum += countPublishedParamsInDoc(child);
		}
		else if (child->hasTagName("param"))
		{
			++sum;
		}
	}
	return sum;
}

void MLProcContainer::dumpGraph(int indent)
{
	const MLRatio myRatio = getResampleRatio();
	if (!myRatio.isUnity()) 
	{
		debug() << ml::stringUtils::spaceStr(indent) << getName() << " input resamplers: \n";
		int ins = mPublishedInputs.size();
		for(int i=0; i<ins; ++i)
		{
			MLProcPtr pIn = mInputResamplers[i];
			debug() << ml::stringUtils::spaceStr(indent) << "in: (" << (void *)&pIn->getInput(1) << ") out: (" << (void *)&pIn->getOutput() << ")\n";
		}
	}
	
	dumpProc(indent);

	// dump children
	debug() << ml::stringUtils::spaceStr(indent) << "null input: (" << (void *)&getNullInput() << ") \n";	
	debug() << ml::stringUtils::spaceStr(indent) << "null output: (" << (void *)&getNullOutput() << ") \n";	
	debug() << ml::stringUtils::spaceStr(indent) << "ops list: " << mOpsVec.size() << " elements: \n";	

	int ops = 0;
	for (std::vector<MLProc*>::iterator it = mOpsVec.begin(); it != mOpsVec.end(); ++it, ++ops)
	{		
		MLProc* p = (*it);
		debug() << ml::stringUtils::spaceStr(indent) << ops << ":\n";
		if (p->isContainer())
		{
			MLProcContainer& pc = static_cast<MLProcContainer&>(*p);
			pc.dumpGraph(indent+1);
		}
		else
		{
			p->dumpProc(indent+1);
		}
	}
	
	if (!myRatio.isUnity()) 
	{
		debug() << ml::stringUtils::spaceStr(indent) << getName() <<  " output resamplers: \n";
		int outs = (int)mPublishedOutputs.size();
		for(int i=0; i<outs; ++i)
		{
			MLProcPtr pOut = mOutputResamplers[i];
			debug() << ml::stringUtils::spaceStr(indent) << "in: (" << &pOut->getInput(1) << ") out: (" << &pOut->getOutput() << ")\n";
		}
	}
}

// ----------------------------------------------------------------
#pragma mark buffer pool
//
const float kBufferFree = -16.f; // ooh, a hack

MLSignal* MLProcContainer::allocBuffer()
{
	MLSignal* r = 0;
	MLSignal* pPoolSig;
	for(std::list<MLSignalPtr>::iterator it = mBufferPool.begin(); it != mBufferPool.end(); ++it)
	{
		pPoolSig = &(**it); 
		// return first free signal.
		if (pPoolSig->getRate() == kBufferFree) 
		{
			pPoolSig->setRate(getSampleRate());
			return (pPoolSig);
		}
	}

	r = new MLSignal(kMLProcessChunkSize);
	r->setRate(getSampleRate());
	mBufferPool.push_back(MLSignalPtr(r));
	return r;
}

void MLProcContainer::freeBuffer(MLSignal* pBuf)
{
	pBuf->setRate(kBufferFree);
}

std::ostream& operator<< (std::ostream& out, const compileOp & r)
{
	out << r.procRef->getName() << ":";
	
	//out << "  " << r.inputs.size() << " inputs, " << r.outputs.size() << " outputs, ";
	
	for(std::vector<MLSymbol>::const_iterator it = r.inputs.begin(); it != r.inputs.end(); it++)
	{
		out << (*it) << " ";
	}
	
	out << "-> ";
	
	for(std::vector<MLSymbol>::const_iterator it = r.outputs.begin(); it != r.outputs.end(); it++)
	{
		out << (*it) << " ";
	}
	
	return out;
}


std::ostream& operator<< (std::ostream& out, const sharedBuffer & r)
{
	std::list<compileSignal*>::const_iterator it = r.mSignals.begin();
	out << "(" << static_cast<void*>((*it)->mpSigBuffer) << ") ";
	for(it = r.mSignals.begin(); it != r.mSignals.end(); it++)
	{
		out << "[" << (*it)->mLifeStart << " " << (*it)->mLifeEnd << "]  ";
	}
	
	return out;
}

