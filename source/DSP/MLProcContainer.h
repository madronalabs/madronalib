
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_CONTAINER_H
#define ML_PROC_CONTAINER_H

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <sstream>

#include "MLDSP.h"
#include "MLProc.h"
#include "MLProcRingBuffer.h"
#include "MLParameter.h"
#include "MLRatio.h"

#include "JuceHeader.h" // used only for XML loading now. TODO move to creation by scripting and remove.

class MLProcRingBuffer;

class MLPublishedInput
{
public:
	MLPublishedInput(const MLProcPtr proc, const int inputIndex, const int index) :
		mIndex(index), 
		mProc(proc), 
		mProcInputIndex(inputIndex), 
		mDest(proc), 
		mDestInputIndex(inputIndex) 
		{}
	~MLPublishedInput() {}
	
	void setDest(const MLProcPtr proc, const int index)
	{
		mDest = proc;
		mDestInputIndex = index;
	}
	
	MLSymbol mName;
	int mIndex;
	
	// proc and input index that the input signal goes to.
	MLProcPtr mProc;
	int mProcInputIndex;
	
	// proc and input index that the input signal goes to after resampling, if any.
	// if container is not resampled that will be the same as mProc and mProcInputIndex.
	MLProcPtr mDest;
	int mDestInputIndex;
};

typedef std::shared_ptr<MLPublishedInput> MLPublishedInputPtr;

class MLPublishedOutput
{
public:
	MLPublishedOutput(const MLProcPtr proc, const int outputIndex, const int index) :
		mIndex(index), 
		mProc(proc), 
		mOutput(outputIndex), 
		mSrc(proc), 
		mSrcOutputIndex(outputIndex) 
		{}
	~MLPublishedOutput() {}

	void setSrc(const MLProcPtr proc, const int index)
	{
		mSrc = proc;
		mSrcOutputIndex = index;
	}
	
	MLSymbol mName;
	int mIndex;
	
	// proc and output index that the output signal comes from.
	// if we are resampling this will be the resampler.
	MLProcPtr mProc;
	int mOutput;
	
	// proc and output index that the output signal comes from before resampling, if any.
	// if container is not resampled that will be the same as mProc and mOutput.
	MLProcPtr mSrc;
	int mSrcOutputIndex;
};

typedef std::shared_ptr<MLPublishedOutput> MLPublishedOutputPtr;

// for gathering stats during process()
class MLSignalStats
{
public:
	
	MLSignalStats() :
		mProcs(0),
		mSignalBuffers(0),
		mSignals(0),
		mNanSignals(0),
		mConstantSignals(0)
		{};
	~MLSignalStats() {};
	
	void dump();
	
	int mProcs;
	int mSignalBuffers;
	int mSignals;
	int mNanSignals;
	int mConstantSignals;
};

class MLContainerBase
{
public:
	virtual ~MLContainerBase() {};
	
	// ----------------------------------------------------------------
	#pragma mark graph creation	
	//
	// create a proc.
	virtual MLProcPtr newProc(const MLSymbol className, const MLSymbol procName) = 0;
	
	// make a new MLProc and add it to this container so that it can be found by name.
	// name must be unique within this container, otherwise nothing is added 
	// and nameInUseErr is returned.  
	// new proc is added at end of ops list, or at position before proc matching positionName
	// if one is specified.
	virtual MLProc::err addProc(const MLSymbol className, const MLSymbol procName) = 0;
	
	// get a procPtr by slash delimited path name, recursively descending into subcontainers.
	// leaf names can be the same if procs have differently named paths.
	virtual MLProcPtr getProc(const MLPath & pathName) = 0;
	
	// connect one signal output to one signal input to make a graph edge. 
	// The src and dest procs must live in the same container. 
	virtual void addPipe(const MLPath& src, const MLSymbol output, const MLPath& dest, const MLSymbol input) = 0;

	// build the connection between Procs that a Pipe specifies, by copying
	// Signal ptrs, adjusting block sizes, and creating resamplers as necessary.
	virtual MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- I/O
	//
	virtual void publishInput(const MLPath & procName, const MLSymbol inputName, const MLSymbol alias) = 0;
	virtual void publishOutput(const MLPath & procName, const MLSymbol outputName, const MLSymbol alias) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- signals
	//	
	virtual MLProc::err addSignalBuffers(const MLPath & procAddress, const MLSymbol outputName, 
		const MLSymbol alias, int trigMode, int bufLength) = 0;
	virtual void gatherSignalBuffers(const MLPath & procAddress, const MLSymbol alias, MLProcList& buffers) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- parameters
	// 
	virtual MLPublishedParamPtr publishParam(const MLPath & procName, const MLSymbol paramName, const MLSymbol alias, const MLSymbol type) = 0;
	virtual void addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const MLSymbol param) = 0;
	virtual void setPublishedParam(int index, const MLProperty& val) = 0;
	virtual void routeParam(const MLPath & procAddress, const MLSymbol paramName, const MLProperty& val) = 0;
	//	
	virtual void makeRoot(const MLSymbol name) = 0;
	virtual bool isRoot() const = 0;
	virtual void compile() = 0;

	// ----------------------------------------------------------------
	#pragma mark -- building
	// 
	virtual void buildGraph(juce::XmlElement* pDoc) = 0;	
	virtual void dumpGraph(int indent) = 0;	
	virtual void setProcParams(const MLPath& procName, juce::XmlElement* pelem) = 0;

	typedef std::map<MLSymbol, MLProcPtr> MLSymbolProcMapT;	
	typedef std::map<MLSymbol, MLPublishedParamPtr> MLPublishedParamMapT;
	typedef std::map<MLSymbol, MLPublishedInputPtr> MLPublishedInputMapT;
	typedef std::map<MLSymbol, MLPublishedOutputPtr> MLPublishedOutputMapT;
};

// An MLProcContainer stores a connected graph of MLProc objects.
// Edges between MLProcs are represented by MLPipe objects.
// 
class MLProcContainer: public MLProc, public MLContainerBase, public MLDSPContext
{
friend class MLDSPEngine;

public:	

	class MLPipe
	{
	friend class MLDSPEngine;
	public:
		
		MLPipe(MLProcPtr a, int ai, MLProcPtr b, int bi) :
			mSrc(a), mSrcIndex(ai), mDest(b), mDestIndex(bi)
			{};
		~MLPipe()
			{};
		
		MLProcPtr mSrc;
		int mSrcIndex;
		MLProcPtr mDest;
		int mDestIndex;
	};

	typedef std::shared_ptr<MLPipe> MLPipePtr;

	MLProcContainer();
	virtual ~MLProcContainer();	
	
	// standard ProcInfo 
	virtual MLProcInfoBase& procInfo() { return mInfo; }
	
	// ----------------------------------------------------------------
	#pragma mark MLDSPContext methods
	//

	virtual void setEnabled(bool t);
	virtual bool isEnabled() const;
	virtual bool isProcEnabled(const MLProc* p) const;

	// ----------------------------------------------------------------
	#pragma mark MLProc methods
	//
	bool isContainer(void) { return true; }
	
	void setup();	
	virtual void collectStats(MLSignalStats* pStats);

	virtual void process(const int samples);
	virtual err prepareToProcess();

	void clear();	// clear buffers, DSP history
	void clearInput(const int idx);
	MLProc::err setInput(const int idx, const MLSignal& sig);
	
	virtual int getInputIndex(const MLSymbol name);
	virtual int getOutputIndex(const MLSymbol name);	
	int getNumProcs();
	MLParamValue getParam(const MLSymbol paramName);
	//
	virtual void makeRoot(const MLSymbol name); // mark as root context
	inline virtual bool isRoot() const { return (getContext() == this); }
	virtual void compile();
	
	// ----------------------------------------------------------------
	#pragma mark graph creation
	//
	MLProcPtr newProc(const MLSymbol className, const MLSymbol procName);
	virtual MLProc::err addProc(const MLSymbol className, const MLSymbol procName); 
	virtual void addPipe(const MLPath& src, const MLSymbol output, const MLPath& dest, const MLSymbol input);
	virtual MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi);
	//	
	virtual MLProcPtr getProc(const MLPath & pathName); 
	void getProcList(MLProcList& pList, const MLPath & pathName, int copies, bool enabledOnly = true);
	//
	virtual void publishInput(const MLPath & procName, const MLSymbol inputName, const MLSymbol alias);
	virtual void publishOutput(const MLPath & procName, const MLSymbol outputName, const MLSymbol alias);	
	MLSymbol getOutputName(int index);

	// ----------------------------------------------------------------
	#pragma mark signals
	//
	// methods of MLContainerBase
	virtual MLProc::err addSignalBuffers(const MLPath & procAddress, const MLSymbol outputName, 
		const MLSymbol alias, int trigMode, int bufLength);
	virtual void gatherSignalBuffers(const MLPath & procAddress, const MLSymbol alias, MLProcList& buffers);

private:
	err addBufferHere(const MLPath & procName, MLSymbol outputName, MLSymbol alias, 
		int trigMode, int bufLength);
	MLProc::err addProcAfter(MLSymbol className, MLSymbol alias, MLSymbol afterProc); 
public:

	//
	// ----------------------------------------------------------------
	#pragma mark parameters
	// 
	virtual MLPublishedParamPtr publishParam(const MLPath & procName, const MLSymbol paramName, const MLSymbol alias, const MLSymbol type);
	virtual void addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const MLSymbol param);
	virtual void setPublishedParam(int index, const MLProperty& val);
	virtual void routeParam(const MLPath & procAddress, const MLSymbol paramName, const MLProperty& val);
	
	MLPublishedParamPtr getParamPtr(int index) const;
	int getParamIndex(const MLSymbol name);
	const std::string& getParamGroupName(int index);	
	MLParamValue getParamByIndex(int index);
	int getPublishedParams();

	// ----------------------------------------------------------------
	#pragma mark xml loading / saving
	//
	void scanDoc(juce::XmlDocument* pDoc, int* numParameters);
    MLSymbol RequiredAttribute(juce::XmlElement* parent, const char * name);
    MLPath RequiredPathAttribute(juce::XmlElement* parent, const char * name);     
    void buildGraph(juce::XmlElement* pDoc);
	virtual void dumpGraph(int indent);			
	virtual void setProcParams(const MLPath& procName, juce::XmlElement* pelem);	

	// ----------------------------------------------------------------
	#pragma mark buffer pool
	//
	MLSignal* allocBuffer();
	void freeBuffer(MLSignal* pBuf);
	
protected:
	virtual err buildProc(juce::XmlElement* parent);
	
	// count number of param elements in document.
	// this is used to return param info to host before graph is built. 
	int countPublishedParamsInDoc(juce::XmlElement* pElem);	
	
	MLProcFactory& theProcFactory;

private:
	
	// append a Proc to the end of the proc list, to be run in order by process().
	// Currently the compiler is dumb: procs will be run in the order they are added.
	void addProcToOps (MLProcPtr proc);		

	// private doc building mathods
	void setPublishedParamAttrs(MLPublishedParamPtr p, juce::XmlElement* pelem);
	
	// dump all MLProc subclasses registered at static init time
	void printClassRegistry(void){ theProcFactory.printRegistry(); }

	void dumpMap();
	
// ----------------------------------------------------------------
#pragma mark data
	
private:		
	MLProcInfo<MLProcContainer> mInfo;
	
protected:	

	// TODO 
	// params inputs and outputs can all be managed by some new
	// class that indexes ptrs quickly by both names and integers
	// required: getIndexByName, getObjectByIndex, addObjectByName
	// 
	// map to published params by alias name
	MLPublishedParamMapT mPublishedParamMap;
	// vector of published params, for speedy access by integer	
	std::vector<MLPublishedParamPtr> mPublishedParams;	
			
	// map to published inputs by name
	MLPublishedInputMapT mPublishedInputMap;	
	// by index
	std::vector<MLPublishedInputPtr> mPublishedInputs;
	
	// map to published outputs by name
	MLPublishedOutputMapT mPublishedOutputMap;	
	// by index
	std::vector<MLPublishedOutputPtr> mPublishedOutputs;

	// vector of processors in order of processing operations.
	// This is what gets iterated on during process().
	std::vector<MLProc*> mOpsVec;
		
	// map to processors by name.   
	MLSymbolProcMapT mProcMap;
	
	// list of processors in order of creation. 
	std::list<MLProcPtr> mProcList;

	// List of Pipes created by addPipe().  Pipes are not named. 
	std::list<MLPipePtr> mPipeList; 

	// procs for resampling
	std::vector<MLProcPtr> mInputResamplers;
	std::vector<MLProcPtr> mOutputResamplers;

	// signal buffers for running procs.
	std::list<MLSignalPtr> mBufferPool;

	// parameter groups
	MLParamGroupMap mParamGroups;
	
	MLSignalStats* mStatsPtr;
	
private: // TODO more data should be private

};

// ----------------------------------------------------------------
#pragma mark compiler temps
// TODO move into class

// represents a signal and its lifetime in the DSP graph.
//
class compileSignal
{
public:
	enum
	{
		kNoLife = -1
	};

	compileSignal() : 
		mpSigBuffer(0), 
		mLifeStart(kNoLife), 
		mLifeEnd(kNoLife), 
		mPublishedInput(0), 
		mPublishedOutput(0) 
		{};
	~compileSignal(){};
	
	void setLifespan(int start, int end)
	{
		mLifeStart = start;
		mLifeEnd = end;
	}
	
	// get union of current lifespan with the one specified by [start, end].
	void addLifespan(int start, int end)
	{
		if (mLifeStart == kNoLife)
		{
			mLifeStart = start;
			mLifeEnd = end;
		}
		else
		{
			mLifeStart = min(mLifeStart, start);
			mLifeEnd = max(mLifeEnd, end);
		}
	}
	
	MLSignal* mpSigBuffer;
	int mLifeStart;
	int mLifeEnd;
	int mPublishedInput;
	int mPublishedOutput;
};

// a class representing a single processing node with inputs and outputs when compiling.
//
class compileOp
{
public:
	compileOp(MLProc* p) :
		procRef(p) {};
	~compileOp(){};

	int listIdx;
	MLProc* procRef;
	std::vector<MLSymbol> inputs;
	std::vector<MLSymbol> outputs;
};

// another temporary object for compile.
// TODO move inside class
// a buffer shared between multiple signals at different times. 
// the lifetime of each signal is stored in the signal itself.
class sharedBuffer
{
public:
	sharedBuffer(){};
	~sharedBuffer(){};
	bool canFit(compileSignal* sig);
	void insert(compileSignal* sig);

	// which signals are contained in this shared buffer?
	// sorted by signal lifetime. lifetimes cannot overlap.
	std::list<compileSignal*> mSignals;
};

// different functions to pack a signal into a list of shared buffers. 
// a new sharedBuffer is added to the list if it is needed.
// this is not quite a bin packing problem, because we are not allowed to 
// move the signals in time. 
//
void packUsingWastefulAlgorithm(compileSignal* sig, std::list<sharedBuffer>& bufs);
void packUsingFirstFitAlgorithm(compileSignal* sig, std::list<sharedBuffer>& bufs);

std::ostream& operator<< (std::ostream& out, const compileOp & r);
std::ostream& operator<< (std::ostream& out, const sharedBuffer & r);

#endif
