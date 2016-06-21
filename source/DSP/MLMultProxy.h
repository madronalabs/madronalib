
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_MULT_PROXY_H
#define ML_MULT_PROXY_H

#include "MLProcContainer.h"

class MLMultProxy
{
friend class MLProcMultiple;
public:
	MLMultProxy();
	
	// set the template proc to make more of. 
	void setTemplate(MLProcPtr pTemplate);
	
	// set number of multiples of the class that will be made.  
	// there is one template and (m-1) copies.
	void setCopies(const int m);

	// set number of multiples of the class that will be run in process(). 
	void setEnabledCopies(const int c);

protected:
	~MLMultProxy();
	
	MLProcPtr getCopy(int c);
	MLProcContainer* getCopyAsContainer(int c);
	
    MLProcPtr mTemplate;
	std::vector<MLProcPtr> mCopies;
	int mEnabledCopies;
};

class MLMultiProc : public MLProc, public MLMultProxy
{
public:

	MLMultiProc();
	~MLMultiProc();
	
	// masquerade as instance of template class
	MLProcInfoBase& procInfo(); 
	
	void process(const int n);		
	err prepareToProcess();	
	void clear();
	void clearProc();
	void clearInputs();
	void clearInput(int i);	 
	MLProc::err setInput(const int idx, const MLSignal& srcSig);	
	
	void setParam(const ml::Symbol p, MLParamValue v);	
	int getInputIndex(const ml::Symbol name);
	int getOutputIndex(const ml::Symbol name);		
	void createInput(const int idx);		
	void resizeInputs(const int n);
	void resizeOutputs(const int n);
	void dumpProc(int indent);
	
private:
	MLProcInfo<MLMultiProc> mInfo; //  unused except for errors
};


class MLMultiContainer : public MLProcContainer, public MLMultProxy
{
friend class MLDSPEngine;
public:
	MLMultiContainer();
	~MLMultiContainer();
	
	// ----------------------------------------------------------------
	#pragma mark MLDSPContext methods

	void setEnabled(bool t);
	bool isEnabled() const;
	bool isProcEnabled(const MLProc* p) const;
	
	// ----------------------------------------------------------------
	#pragma mark -

	void setup();		

	void collectStats(MLSignalStats* pStats);
	void process(const int n);		
	err prepareToProcess();	
	void clear();

	// not in ContainerBase because this is a virtual method of MLProc.
	bool isContainer(void) { return true; }

	// masquerade as instance of template class
	MLProcInfoBase& procInfo(); 
	
	// ----------------------------------------------------------------
	#pragma mark graph creation
	//
	bool isMultiple(void) { return true; }

	//
	MLProc::err setInput(const int idx, const MLSignal& srcSig);	
	void setParam(const ml::Symbol p, MLParamValue v);	
	//
	int getInputIndex(const ml::Symbol name);
	int getOutputIndex(const ml::Symbol name);		
	//
	void resizeInputs(const int n);
	void resizeOutputs(const int n);

	MLProcPtr newProc(const ml::Symbol className, const ml::Symbol procName);
	MLProcPtr getProc(const MLPath & pathName); 
	void addPipe(const MLPath& src, const ml::Symbol output, const MLPath& dest, const ml::Symbol input);
	MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi);
	// ----------------------------------------------------------------
	#pragma mark I/O
	//
	void publishInput(const MLPath & procName, const ml::Symbol inputName, const ml::Symbol alias);
	void publishOutput(const MLPath & procName, const ml::Symbol outputName, const ml::Symbol alias);	
	ml::Symbol getOutputName(int index);
	//
	// ----------------------------------------------------------------
	#pragma mark signals
	//
	// methods of MLContainerBase
	MLProc::err addSignalBuffers(const MLPath & procAddress, const ml::Symbol outputName, 
		const ml::Symbol alias, int trigMode, int bufLength, int frameSize = 1);
	void gatherSignalBuffers(const MLPath & procAddress, const ml::Symbol alias, MLProcList& signalBuffers);
	
	//
	MLProc::err buildProc(juce::XmlElement* parent);
	void dumpGraph(int indent);	
	void setProcParams(const MLPath& procName, juce::XmlElement* pelem);
	MLPublishedParamPtr publishParam(const MLPath & procName, const ml::Symbol paramName, const ml::Symbol alias, const ml::Symbol type);
	void addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const ml::Symbol param);
	void setPublishedParam(int index, const MLProperty& val);
	void routeParam(const MLPath & procAddress, const ml::Symbol paramName, const MLProperty& val);
	//
	void compile();

private:
	MLProcInfo<MLMultiContainer> mInfo; //  unused except for errors


};


#endif // ML_MULT_PROXY_H