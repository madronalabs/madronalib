
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLProcContainer.h"

const floatAliasVec MLProcInfoBase::kMLProcNullAliasVec;

#pragma mark MLProc

// MLProc::prepareToProcess() is called after all connections in DSP graph are made. 
// This is where sample rates and block sizes propagate through the graph, and 
// are checked for consistency.
// Setup internal buffers and data to prepare for processing any attached input signals.
// MLProcs have no concept of enabled -- it's up to the enclosing container to disable
// itself if things go wrong here.
//
MLProc::err MLProc::prepareToProcess()
{
	MLProc::err e = OK;
	
	// debug() << "preparing " << getClassName() << " \"" << getName() << "\" : " ;
	
	int ins = getNumInputs();
	int outs = getNumOutputs();
	float rate = getContextSampleRate();
	int blockSize = getContextVectorSize();
	
	// debug() << ins << " ins, " << outs << " outs, rate " << rate << ", blockSize " << blockSize << "\n";
	
	// All inputs must have a signal connected.	
	// So connect unconnected inputs to null input signal.
	for (int i=0; i<ins; i++)
	{
		if (!mInputs[i])
		{
			mInputs[i] = &getContext()->getNullInput();
		}
	}
	
	// set size and rate of output signals
	// TODO it looks like this allocation may be redundant because outputs are allocated in compile() already!?
	for (int i=1; i<=outs; ++i)
	{
		MLSignal& out = getOutput(i);	
		if (&out)
		{
			out.setRate(rate);		
			MLSample* outData = out.setDims(blockSize, getOutputFrameSize(i));
			if (!outData) 
			{
				e = memErr;
				goto bail;
			}
		}
		else
		{
			debug() << "MLProc::prepareToProcess: null output " << i << " for " << getName() << "! \n";
		}
		//debug() << "    out " << i << ": " << (void *)(MLSignal*)(&out) << ", " << out.getSize() << " samples.\n";
	}
	e = resize();
	
	// recalc params for new sample rate
	mParamsChanged = true;	
bail:	
	return e;
}

void MLProc::clearProc()
{		
	const int op = getNumOutputs();
	
 //debug() << "clearing " << getName() << " (" << getClassName() << ")\n";
	
	// clear anything left in outputs
	for (int i=0; i<op; i++)
	{
		mOutputs[i]->clear();
	}
	
	// let subclass clear filter histories etc.
	clear();
	
}

void MLProc::clearInputs()
{		
	const int inputs = getNumInputs();
	for (int i=0; i<inputs; ++i)
	{
		mInputs[i] = 0;
	}
}

void MLProc::clearInput(const int i)
{		
	if(i <= getNumInputs())
	{
		mInputs[i-1] = 0;
	}
}

MLProc::err MLProc::setInput(const int idx, const MLSignal& srcSig)
{
	//debug() << "setInput " << idx << " of " << getName() << " to " << (void *)(&srcSig) << "\n";
	
	MLProc::err e = OK;	
	if (idx)
	{
		int ins = getNumInputs();
		if (idx > ins)
		{
			e = inputBoundsErr;
		}
		else if (mInputs[idx - 1])
		{
			if(mInputs[idx - 1] == &(getContext()->getNullInput()))
			{
				mInputs[idx - 1] = &srcSig;
			}
			else
			{
				// TODO this condition can cause crashes down the road, fix
				e = inputOccupiedErr;
			}
		}
		else
		{
			mInputs[idx - 1] = &srcSig;
		}
	}
	return e;
}

void MLProc::setOutput(const int idx, MLSignal& srcSig)
{	
	// if not allocated, make space for pointer and zero it
	if (idx > (int)mOutputs.size())
	{
		mOutputs.resize(idx, 0);
	}	
	mOutputs[idx-1] = &srcSig;
}

bool MLProc::paramExists(const ml::Symbol pname)
{
	bool r = false;
	if (procInfo().hasVariableParams())
	{
		// any param exists.
		r = true;
	}
	else
	{
		MLSymbolMap& map = procInfo().getParamMap();
		r = (map.getIndex(pname) > 0);
	}
	return r;
}

float MLProc::getParam(const ml::Symbol pname)
{
	return procInfo().getParamProperty(pname).getFloatValue();
}

const ml::Text MLProc::getTextParam(const ml::Symbol pname)
{
	return procInfo().getParamProperty(pname).getTextValue();
}

const MLSignal& MLProc::getSignalParam(const ml::Symbol pname)
{
	return procInfo().getParamProperty(pname).getSignalValue();
}

void MLProc::setParam(const ml::Symbol pname, const MLProperty& val)
{
	// TODO rather than setting directly here, the enclosing MLDSPContext can store a list of changes
	// to take effect before the next process() call. This way all the [if (mParamsChanged) doParams();]
	// code can be moved out of process() methods and mParamsChanged would not be needed!
	procInfo().setParamProperty(pname, val);
	mParamsChanged = true;
}

int MLProc::getInputIndex(const ml::Symbol name) 
{ 
	// get index
	int idx = 0;
	if (procInfo().hasVariableInputs())
	{
		idx = ml::textUtils::getFinalNumber(name);
	}
	else
	{	
		// will be > 0 for valid names, otherwise 0
		idx = procInfo().getInputMap().getIndex(name);
	}
	if (!idx)
	{
		debug() << "MLProc::getInputIndex: proc " << getName() << " has no input " << name << "\n";	
	}
	return idx;
}

int MLProc::getOutputIndex(const ml::Symbol name) 
{ 
	// get index
	int idx = 0;
	if (procInfo().hasVariableOutputs())
	{
		// variable outputs are indexed by symbols out1, out2, ...
		idx = ml::textUtils::getFinalNumber(name);
	}
	else
	{
		idx = procInfo().getOutputMap().getIndex(name); 
	}
	if (!idx)
	{
		debug() << "MLProc::getOutputIndex: null output index!\n";	
	}
	return idx; 
}


ml::Symbol MLProc::getOutputName(int index)
{	
	if (procInfo().hasVariableOutputs())
	{
		return ml::textUtils::addFinalNumber(ml::Symbol("out"), index);
	}
	else if (index <= (int)mOutputs.size())
	{		
		MLSymbolMap& map = procInfo().getOutputMap();
		for (MLSymbolMap::MLSymbolMapIter i = map.begin(); i != map.end(); i++)
		{
			if ((*i).second == index)
			{
				return ((*i).first);
			}
		}
	} 
	
	return ml::Symbol();
}

// TODO: sort out createinput / resizeInputs / resizeOutputs
// just creates room for Signal pointer and nulls it
void MLProc::createInput(const int index)
{
	if (index > (int)mInputs.size())
	{
		mInputs.resize(index, 0);
	}
}


// TODO there is some inconsistency with how these and setOutput() are used --  clean up

void MLProc::resizeInputs(int n)
{
	mInputs.resize(n, 0);
}

void MLProc::resizeOutputs(int n)
{
	mOutputs.resize(n, 0);
}

bool MLProc::inputIsValid(int idx)
{ 
	if (idx <= (int)mInputs.size())
	{
		return mInputs[idx-1] != 0; 
	}
	else
	{
		return 0;
	}
}

bool MLProc::outputIsValid(int idx)
{ 
	if (idx <= (int)mOutputs.size())
	{
		return mOutputs[idx-1] != 0; 
	}
	else
	{
		return 0;
	}
}

ml::Symbol MLProc::getNameWithCopyIndex()
{
	int c = mCopyIndex;
	if(c)
	{
		return ml::textUtils::addFinalNumber(mName, mCopyIndex);
	}
	else
	{
		return mName;
	}
}

void MLProc::dumpParams()
{
	MLSymbolMap& map = procInfo().getParamMap();
	
	debug() << getClassName() << "(" << static_cast<void *>(this) << ")" << " params:--------\n";	
	for (MLSymbolMap::MLSymbolMapIter i = map.begin(); i != map.end(); i++)
	{
		const ml::Symbol pName = ((*i).first);
		float val = getParam(pName);
		debug() << "[" << pName << " : " << val << "] ";
	}
	debug() << "\n";
}

// TODO null signals function but are not printed out quite right
// because sometimes the parent's null signal is used and sometimes not
void MLProc::dumpProc(int indent)
{
	int ins = getNumInputs();
	int outs = getNumOutputs();
	const MLSignal* pNullInput = &(getContext()->getNullInput());
	const MLSignal* pNullOutput = &(getContext()->getNullOutput());
	
	debug() << ml::textUtils::spaceStr(indent) << getName() << " (" << getClassName() << " " << (void *)&(*this) << ")";
	
	if (isContainer())
	{
		std::string enabledStr = isEnabled() ? " [ENABLED] " : " [DISABLED] ";
		debug() << enabledStr;
	}
	debug() << "\n";
	debug() << ml::textUtils::spaceStr(indent) << "inputs: ";
	if (ins)
	{
		for(int j = 1; j <= ins; ++j)
		{
			debug() << "[" << j << "] ";
			const MLSignal* pIn = &getInput(j);
			if (pIn == pNullInput)
			{
				debug() << "(null)  ";
			}
			else
			{
				debug() << "(" << (void *)(pIn) << ")  ";
			}
		}
	}
	else
	{
		debug() << "(none)";
	}
	debug() << "\n";
	debug() << ml::textUtils::spaceStr(indent) << "outputs: ";
	if (outs)
	{
		for(int j = 1; j <= outs; ++j)
		{
			debug() << "[" << j << "] ";
			const MLSignal* pOut = &getOutput(j);
			if(pOut == pNullOutput)
			{
				debug() << "(null)  ";
			}
			else
			{
				// can no longer determine isConstant here because signals are shared.
				debug() << "(" << (void *)(pOut)  << ")  ";
			}
		}
	}
	else
	{
		debug() << "(none)";
	}
	debug() << "\n";
}

void MLProc::printErr(MLProc::err e)
{
	debug() << "*** proc " << getName() << " error: ";
	switch(e)
	{
		case memErr:
			debug() << "memErr\n";
			break;		
		case inputBoundsErr:
			debug() << "inputBoundsErr\n";
			debug() << "wha\n";
			break;		
		case inputOccupiedErr:
			debug() << "inputOccupiedErr\n";
			break;		
		case inputRateErr:
			debug() << "inputRateErr\n";
			break;		
		case noInputErr:
			debug() << "noInputErr\n";
			break;		
		case inputMismatchErr:
			debug() << "inputMismatchErr\n";
			break;		
		case fractionalBlockSizeErr:
			debug() << "fractionalBlockSizeErr\n";
			break;		
		case connectScopeErr:
			debug() << "connectScopeErr\n";
			break;		
		case nameInUseErr:
			debug() << "nameInUseErr\n";
			break;
		case headNotContainerErr:
			debug() << "headNotContainerErr\n";
			break;
		case nameNotFoundErr:
			debug() << "nameNotFoundErr\n";
			break;
		case fileOpenErr:
			debug() << "fileOpenErr\n";
			break;
		case docSyntaxErr:
			debug() << "docSyntaxErr\n";
			break;
		case newProcErr:
			debug() << "newProcErr\n";
			break;
		case SSE2RequiredErr:
			debug() << "SSE2RequiredErr\n";
			break;
		case OK:
			debug() << "OK\n";
			break;
		default:
			debug() << "unknown error " << e << "\n";
			break;
	}
}

// ----------------------------------------------------------------
#pragma mark factory


MLProcFactory::MLProcFactory()
{
}

MLProcFactory::~MLProcFactory()
{
}

void MLProcFactory::registerFn(const ml::Symbol className, MLProcCreateFnT fn)
{
	procRegistry[className] = fn;
}

MLProcPtr MLProcFactory::create(const ml::Symbol className, MLDSPContext* context)
{
	MLProcCreateFnT fn;
	MLProcPtr resultProc;
	
	// get named entry from registry
	FnRegistryT::const_iterator regEntry = procRegistry.find(className);
	
	if (regEntry != procRegistry.end()) 
	{
		// get creator fn from entry
		fn = regEntry->second;
		// call creator fn returning new MLProc subclass instance
		resultProc = fn();
		resultProc->setContext(context);
		
		// give root context pointer to new containers
		if(resultProc->isContainer())
		{
			MLProcContainer& resultContainer = static_cast<MLProcContainer&>(*resultProc);
			resultContainer.setRootContext(context->getRootContext());
		}
	}
	else
	{
		// TODO anything? 
		debug() << "MLProcFactory::create: class " << className << " not found!!";
	}
	
	return resultProc;
}

void MLProcFactory::printRegistry(void)
{
	int size = (int)procRegistry.size();
	
	debug() << "---------------------------------------\n";
	debug() << "MLProc registry: " << size << " members\n";
	
	for (FnRegistryT::iterator i = procRegistry.begin(); i != procRegistry.end(); i++)
	{
		debug() << (*i).first << "\n";		
	}
}