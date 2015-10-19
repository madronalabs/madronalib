
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMultProxy.h"

// ----------------------------------------------------------------
#pragma mark MLMultProxy
//

MLMultProxy::MLMultProxy()
{
}

MLMultProxy::~MLMultProxy()
{
}

void MLMultProxy::setTemplate(MLProcPtr pTemplate)
{
	mTemplate = pTemplate;
	mTemplate->mCopyIndex = 0;
	mEnabledCopies = 0;
}

// make the multiple procs.  Must set template beforehand.
// new copies are intially not enabled.
void MLMultProxy::setCopies(const int newSize)
{
	if (newSize < 1) return;
	if (mTemplate)
	{        
		int oldSize = (int)mCopies.size();
		if (newSize != oldSize)
		{
			mCopies.resize(newSize);
            // initialize new copies
            MLSymbol className = mTemplate->getClassName();
            MLSymbol procName = mTemplate->getName();
            for(int i = 0; i < newSize; ++i)
            {
                // make new proc.  we use the template object's container for the call to 
                // newProc(), but the new proc is not added to the container.
                // instead we keep a ProcPtr in mCopies.
                MLProcContainer* c = static_cast<MLProcContainer*>(mTemplate->getContext());
                mCopies[i] = c->newProc(className, procName);
                mCopies[i]->setCopyIndex(i + 1);
                mCopies[i]->clearProc();
            }
		}
	}
	else
	{
		debug() << "MLMultProxy: no template for setCopies()!\n";
	}
}

void MLMultProxy::setEnabledCopies(const int c)
{
	int copies = (int)mCopies.size();
	mEnabledCopies = min(c, copies);
	
	for (int i=0; i < copies; ++i)
	{
		MLProcContainer* pCopy = getCopyAsContainer(i);
		if (i < mEnabledCopies)
		{	
			if(!pCopy->isEnabled())
			{
				pCopy->clearProc();
				pCopy->setEnabled(true);
			}
		}
		else
		{
			if(pCopy->isEnabled())
			{
				pCopy->setEnabled(false);
				pCopy->clearProc();
			}
		}
	}
}

MLProcContainer* MLMultProxy::getCopyAsContainer(int c)
{
	MLProcPtr p = mCopies[c];
	if (p->isContainer())
	{
		return static_cast<MLProcContainer*>(&(*p));
	}
	else
	{
		debug() << "MLMultProxy::getCopyAsContainer: error, copy " << c << " of template " << mTemplate->getName() << "is not container!\n";
		return 0;
	}
}

MLProcPtr MLMultProxy::getCopy(int c)
{
	MLProcPtr p = mCopies[c];
	return p;
}

// ----------------------------------------------------------------
#pragma mark MLMultiProc
//

namespace multiProc 
{
	MLProcRegistryEntry<MLMultiProc> classReg("multiproc");
	//MLProcParam<MLMultiProc> params[0] = {"multiples"};
	MLProcInput<MLMultiProc> inputs[] = {"*"};
	MLProcOutput<MLMultiProc> outputs[] = {"*"};
}	

MLMultiProc::MLMultiProc()
{	
}

MLMultiProc::~MLMultiProc()
{
//	debug() << "~MLMultiProc destructor\n";
}

MLProcInfoBase& MLMultiProc::procInfo()
{ 
	if (mTemplate)
	{
		return mTemplate->procInfo();
	}
	else
	{
		debug() << "MLMultiProc::procInfo(): no template!\n";
		return mInfo; 
	}
}


void MLMultiProc::process(const int n)
{
	const int outs = mTemplate->getNumOutputs();
	
	// for each enabled copy, process.  
	// TODO this can be dispatched to multiple threads.
	for (int i=0; i < mEnabledCopies; ++i)
	{
		mCopies[i]->process(n);
	}
	
	// TODO create a voice in/out fade interval, and fade instead of just adding during change. 
	
	// for each output,
	for (int i=1; i <= outs; ++i)
	{
		// sum outputs of copies to our MLProc output.
		MLSignal& out = MLProc::getOutput(i);
		out.clear();
		for(int j=0; j < mEnabledCopies; ++j)
		{
			out.add(mCopies[j]->getOutput(i));
		}
	}
}


// Setup internal buffers and data to prepare for processing any attached input signals.
//
// prepare all copies so we can change number of enabled copies dynamically.
//
MLProc::err MLMultiProc::prepareToProcess()
{
	err e = OK;
	const int copies = (int)mCopies.size();
	
	for(int i=0; i<copies; i++)
	{
		e = mCopies[i]->prepareToProcess();
		if (e != OK) break;
	}
	e = MLProc::prepareToProcess();
	return e;
}

void MLMultiProc::clear()
{
	const int copies = (int)mCopies.size();
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->clear();
	}
}

void MLMultiProc::clearInputs()
{		
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->clearInputs();
	}
	MLProc::clearInputs();
}

void MLMultiProc::clearInput(int idx)
{		
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->clearInput(idx);
	}
	MLProc::clearInput(idx);
}

MLProc::err MLMultiProc::setInput(const int idx, const MLSignal& srcSig)
{
	err e = OK;
//debug() << "MLMultiProc::setInput (" << getName() << ")\n";

	e = MLProc::setInput(idx, srcSig);
	const int copies = (int)mCopies.size();	
	if (e == OK)
	{
		for(int i=0; i<copies; i++)
		{
//debug() << "                setInput #" << i << "\n";
			e = mCopies[i]->setInput(idx, srcSig);
			if (e != OK) break;
		}
	}
	return e;
}

// we override setParam but not getParam.  Since all copies share parameters,
// we just store them in our MLProc and return those for getParam().
void MLMultiProc::setParam(const MLSymbol p, MLParamValue v)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->setParam(p, v);
	}
	MLProc::setParam(p, v);
}	

int MLMultiProc::getInputIndex(const MLSymbol name)
{
	return mTemplate->getInputIndex(name);
}

int MLMultiProc::getOutputIndex(const MLSymbol name)		
{
	return mTemplate->getOutputIndex(name);
}

void MLMultiProc::createInput(const int idx)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->createInput(idx);
	}
	MLProc::createInput(idx);
}

void MLMultiProc::resizeInputs(const int n)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->resizeInputs(n);
	}
	MLProc::resizeInputs(n);
}

void MLMultiProc::resizeOutputs(const int n)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->resizeOutputs(n);
	}
	MLProc::resizeOutputs(n);
}

void MLMultiProc::dumpProc(int indent)
{
	const int copies = (int)mCopies.size();	

	debug() << ml::stringUtils::spaceStr(indent) << getName() << " (multiproc " << (void *)&(*this) << ")\n";

	for(int i=0; i<copies; i++)
	{
	debug() << ml::stringUtils::spaceStr(indent) <<  " copy " << i + 1 << ": \n";
	
		getCopy(i)->dumpProc(indent + 1);
	}
}

// ----------------------------------------------------------------
#pragma mark MLMultiContainer
//

namespace multiContainer
{
	MLProcRegistryEntry<MLMultiContainer> classReg("multicontainer");
	//MLProcParam<MLMultiContainer> params[0] = {"multiples"};
	MLProcInput<MLMultiContainer> inputs[] = {"*"};
	MLProcOutput<MLMultiContainer> outputs[] = {"*"};
}

MLMultiContainer::MLMultiContainer() //: theProcFactory(MLProcFactory::theFactory())
{
}

MLMultiContainer::~MLMultiContainer()
{
//	debug() << "~MLMultiContainer destructor\n";
}

// ----------------------------------------------------------------
#pragma mark MLDSPContext methods
//

void MLMultiContainer::setEnabled(bool t)
{
	const int copies = (int)mCopies.size();	
	for (int i=0; i < copies; ++i)
	{
		getCopyAsContainer(i)->setEnabled(i < mEnabledCopies);
	}
	MLProcContainer::setEnabled(t);
}

bool MLMultiContainer::isEnabled() const
{
	return mEnabled;
}

bool MLMultiContainer::isProcEnabled(const MLProc* p) const
{
//debug() << "MLMultiContainer::isProcEnabled, copy " << p->mCopyIndex << " ? " <<
//	(int)((p->mCopyIndex < mEnabledCopies) && (getContext()->isEnabled())) << "\n";
//debug() << "index " << p->mCopyIndex << ", " << mEnabledCopies << " enabled.\n";
	
	return mEnabled && (p->mCopyIndex <= mEnabledCopies);
}

void MLMultiContainer::setup()
{
//	float fr;
//	fr = getParam("ratio");
//	setResampleRatio(theCommonRatios.getClosest(fr));
//debug() << "***setup multicontainer " << getName() << ": ratio " << getResampleRatio() << " (" << fr << ") \n";

	const int copies = (int)mCopies.size();	
	for (int i=0; i < copies; ++i)
	{
		getCopyAsContainer(i)->setup();
	}
	
	MLProcContainer::setup();

}

// recurse into containers, setting stats ptr and collecting number of procs.
void MLMultiContainer::collectStats(MLSignalStats* pStats)
{
	// for each copy, collect stats.  
	for (int i=0; i < mEnabledCopies; ++i)
	{
		getCopyAsContainer(i)->collectStats(pStats);
	}
}

void MLMultiContainer::process(const int n)
{
	const int outs = getNumOutputs();
	
	// for each copy, process.  
	// TODO this can be dispatched to multiple threads.
	for (int i=0; i < mEnabledCopies; ++i)
	{
		getCopyAsContainer(i)->process(n);
	}
    
	// for each of our outputs,
	for (int i=1; i <= outs; ++i)
	{
		// sum outputs of copies to our output.
		MLSignal& y = getOutput(i);
		y.clear();
                
		for(int j=0; j < mEnabledCopies; ++j)
		{
            // TODO rewrite to handle multiple outputs better.
            // right now if the last copy is enabled, it shares a buffer with the
            // parent and this add doubles the last value. the hack in place adds one more copy to prevent this. 
            MLProcContainer* pCopy = getCopyAsContainer(j);
            if(pCopy)
            {
                y.add(pCopy->getOutput(i));
            }
            else
            {
                debug() << "MLMultiContainer: null copy in process()!\n";
            }
 		}
    }
}

// Setup internal buffers and data to prepare for processing any attached input signals.
//
MLProc::err MLMultiContainer::prepareToProcess()
{
	err e = OK;
	
	// prepareToProcess must set up MLProcContainer context before it is called
	// on copies.  The copies refer to MLProcContainer context rate, etc.
	e = MLProcContainer::prepareToProcess();

	const int copies = (int)mCopies.size();	
	if (e == OK)
	{
		for(int i=0; i<copies; i++)
		{
			e = getCopyAsContainer(i)->prepareToProcess();
			if (e != OK) break;
		}	
	}
	return e;
}

void MLMultiContainer::clear()
{
	const int copies = (int)mCopies.size();
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->clearProc();
	}
}

MLProcInfoBase& MLMultiContainer::procInfo()
{ 
	if (mTemplate)
	{
		return mTemplate->procInfo(); 
	}
	else
	{
		debug() << "MLMultiContainer::procInfo(): no template!\n";
		return mInfo; 
	}
}

MLProc::err MLMultiContainer::setInput(const int idx, const MLSignal& srcSig)
{
	err e = OK;

	const int copies = (int)mCopies.size();	
	
	// set MLProc input to sig for prepareToProcess()
	e = MLProc::setInput(idx, srcSig);
	
	if (e == OK)
	{
		for(int i=0; i<copies; i++)
		{
			e = getCopyAsContainer(i)->setInput(idx, srcSig);
			if (e != OK) break;
		}
	}
	
	return e;
}

// we override setParam but not getParam.  Since all copies share parameters,
// we just store them in our MLProc and return those for getParam().
// this is called for "ratio" and any other params added for MLProcContainer itself.
void MLMultiContainer::setParam(const MLSymbol p, MLParamValue v)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->setParam(p, v);
	}
	MLProc::setParam(p, v);
}	

int MLMultiContainer::getInputIndex(const MLSymbol name)
{
	return getCopyAsContainer(0)->getInputIndex(name);
}


int MLMultiContainer::getOutputIndex(const MLSymbol name)		
{
	return getCopyAsContainer(0)->getOutputIndex(name);
}


void MLMultiContainer::resizeInputs(const int n)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->resizeInputs(n);
	}
	MLProc::resizeInputs(n);
}

void MLMultiContainer::resizeOutputs(const int n)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		mCopies[i]->resizeOutputs(n);
	}
	MLProc::resizeOutputs(n);
}

// ----------------------------------------------------------------
#pragma mark MLContainerBase -- graph creation	

// make a new instance of a named subclass of MLProc. 
//
MLProcPtr MLMultiContainer::newProc(const MLSymbol className, const MLSymbol procName) 
{
	MLProcPtr pNew;
	
	// call factory to get instance of processor class
	// sets context of new proc to this.
	pNew = theProcFactory.create(className, this);	
	if (!pNew)
	{
		debug() << "MLMultiContainer: newProc: couldn't create!\n";
	}
	else
	{
		pNew->setName(procName);
		pNew->setContext(this);
	}
	return pNew;
}

MLProc::err MLMultiContainer::buildProc(juce::XmlElement* parent)
{
	err e = OK;
	const int copies = (int)mCopies.size();	
	const MLSymbol className(parent->getStringAttribute("class").toUTF8());
	const MLSymbol procName(parent->getStringAttribute("name").toUTF8());
	// debug() << "MLMultiContainer::buildProc (class=" << className << ", name=" << procName << ")\n";

	for(int i=0; i<copies; i++)
	{
		// add the specified proc to all subcontainers. 
		MLProcContainer* pCopy = getCopyAsContainer(i);
        if(pCopy != nullptr)
        {
            e = pCopy->addProc(className, procName);
            if (e == MLProc::OK)
            {
                MLPath procPath(procName);
                pCopy->setProcParams(procPath, parent);
                pCopy->setCopyIndex(i + 1);
                MLProcPtr p = pCopy->getProc(procPath);	
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
                    debug() << "MLMultiContainer::buildProc: getProc failed for new proc!\n";
                }
            }
        }
        else
        {
            debug() << "MLMultiContainer: null copy in buildProc ()!\n";
        }
	}
	return e;
}

// calling this is a bad idea because there are no procs in this container, 
// only in our template and copies. 
MLProcPtr MLMultiContainer::getProc(const MLPath & pathName)
{
	MLProcPtr r;
debug() << "*************** ACK:	MLMultiContainer::getProc called!\n";
debug() << "path = " << pathName << "\n"; 
	return r;
}

// ----------------------------------------------------------------
// 
// really we should only need to add one pipe per proxy container
// but let's keep things simple for now.
//
void MLMultiContainer::addPipe(const MLPath& src, const MLSymbol out, const MLPath& dest, const MLSymbol in)
{
	//debug() << "MLMultiContainer::addPipe " << src << " " << out << " -> " << dest << " " << in << "\n";
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->addPipe(src, out, dest, in);
	}
}	

// check that Pipe is doing something reasonable and setup connection
// between procs.
//
MLProc::err MLMultiContainer::connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi)
{
	err e = OK;
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		e = getCopyAsContainer(i)->connectProcs(a, ai, b, bi);
		if (e != OK) break;
	}
	return e;
}

// ----------------------------------------------------------------
#pragma mark I/O

void MLMultiContainer::publishInput(const MLPath & procName, const MLSymbol inputName, const MLSymbol alias)
{
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->publishInput(procName, inputName, alias);
	}
	
	// make dummy input for prepareToProcess()
	// if not allocated, make space for input pointer and zero it
	const int index = getCopyAsContainer(0)->getNumInputs();
	createInput(index);
}

void MLMultiContainer::publishOutput(const MLPath & procName, const MLSymbol outputName, const MLSymbol alias)
{
 	const int copies = (int)mCopies.size();
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->publishOutput(procName, outputName, alias);
	}
		
	// this container does not have published outputs, because it has no procs to point to. 
	// get number of outputs from template, which will already be set to number of published
	// outputs, then resize our outputs. 
	
	resizeOutputs(getCopyAsContainer(0)->getNumOutputs());			

}


MLSymbol MLMultiContainer::getOutputName(int index)
{	
	return (getCopyAsContainer(0)->getOutputName(index));	
}

// ----------------------------------------------------------------
#pragma mark signals

// add ring buffers to each copy so that the published signal can be read by clients in different threads.
MLProc::err MLMultiContainer::addSignalBuffers(const MLPath & procAddress, const MLSymbol outputName, 
	const MLSymbol alias, int trigMode, int bufLength)
{
	err e = OK;
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		e = getCopyAsContainer(i)->addSignalBuffers(procAddress, outputName, alias, trigMode, bufLength);
		if (e != OK) break;
	}
	return e;
}

void MLMultiContainer::gatherSignalBuffers(const MLPath & procAddress, const MLSymbol alias, MLProcList& signalBuffers)
{
    const int hack = 1; // for multiple outputs extra voice
	const int copies = (int)mCopies.size() - hack;
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->gatherSignalBuffers(procAddress, alias, signalBuffers);
	}
}

// ----------------------------------------------------------------
#pragma mark parameters
// 

void MLMultiContainer::dumpGraph(int indent)
{
	const int copies = (int)mCopies.size();	
	
	dumpProc(indent);

	debug() << ml::stringUtils::spaceStr(indent) << getName() << " (multicontainer " << (void *)&(*this) << ")\n";

	for(int i=0; i<copies; i++)
	{
		debug() << ml::stringUtils::spaceStr(indent) <<  " copy " << i + 1 << ": \n";	
		getCopyAsContainer(i)->dumpGraph(indent + 1);
	}
}

void MLMultiContainer::setProcParams(const MLPath& procName, juce::XmlElement* parent)
{
	// build in containers
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->setProcParams(procName, parent);
	}
}

MLPublishedParamPtr MLMultiContainer::publishParam(const MLPath & procName, const MLSymbol param, const MLSymbol alias, const MLSymbol type)
{
	MLPublishedParamPtr p;

	// publish param in containers
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->publishParam(procName, param, alias, type);
	}

	// publish param in this
	p = MLProcContainer::publishParam(procName, param, alias, type);
	return p;
}

void MLMultiContainer::addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const MLSymbol param)
{
	// add setter in containers
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->addSetterToParam(p, procName, param);
	}

	// add setter in this
	MLProcContainer::addSetterToParam(p, procName, param);
}

void MLMultiContainer::setPublishedParam(int index, const MLProperty& val)
{
	// set param in containers
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->setPublishedParam(index, val);
	}
	// set param in this
	// is this needed??
	//MLProcContainer::setPublishedParam(index, val);
}

void MLMultiContainer::routeParam(const MLPath & procAddress, const MLSymbol paramName, const MLProperty& val)
{
	// set param in containers
	const int copies = (int)mCopies.size();	
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->routeParam(procAddress, paramName, val);
	}
	// set param in this
	// MLProcContainer::setPublishedParam(index, val);
}

void MLMultiContainer::compile()
{
	const int copies = (int)mCopies.size();
    
	for(int i=0; i<copies; i++)
	{
		getCopyAsContainer(i)->compile();
	}
    
    // MLProcContainer's outputs are allocated in compile().  We do a minimal verison here.
	int outs = getNumOutputs();
	for(int i=0; i<outs; ++i)
	{
        MLSignal& newSig = *allocBuffer();
		setOutput(i + 1, newSig);
	}
}
