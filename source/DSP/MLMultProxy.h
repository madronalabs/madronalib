
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
	
	void setParam(const MLSymbol p, MLParamValue v);	
	int getInputIndex(const MLSymbol name);
	int getOutputIndex(const MLSymbol name);		
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
	void setParam(const MLSymbol p, MLParamValue v);	
	//
	int getInputIndex(const MLSymbol name);
	int getOutputIndex(const MLSymbol name);		
	//
	void resizeInputs(const int n);
	void resizeOutputs(const int n);

	MLProcPtr newProc(const MLSymbol className, const MLSymbol procName);
	MLProcPtr getProc(const MLPath & pathName); 
	void addPipe(const MLPath& src, const MLSymbol output, const MLPath& dest, const MLSymbol input);
	MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi);
	// ----------------------------------------------------------------
	#pragma mark I/O
	//
	void publishInput(const MLPath & procName, const MLSymbol inputName, const MLSymbol alias);
	void publishOutput(const MLPath & procName, const MLSymbol outputName, const MLSymbol alias);	
	MLSymbol getOutputName(int index);
	//
	// ----------------------------------------------------------------
	#pragma mark signals
	//
	// methods of MLContainerBase
	MLProc::err addSignalBuffers(const MLPath & procAddress, const MLSymbol outputName, 
		const MLSymbol alias, int trigMode, int bufLength);
	void gatherSignalBuffers(const MLPath & procAddress, const MLSymbol alias, MLProcList& signalBuffers);
	
	//
	MLProc::err buildProc(juce::XmlElement* parent);
	void dumpGraph(int indent);	
	void setProcParams(const MLPath& procName, juce::XmlElement* pelem);
	MLPublishedParamPtr publishParam(const MLPath & procName, const MLSymbol paramName, const MLSymbol alias, const MLSymbol type);
	void addSetterToParam(MLPublishedParamPtr p, const MLPath & procName, const MLSymbol param);
	void setPublishedParam(int index, const MLProperty& val);
	void routeParam(const MLPath & procAddress, const MLSymbol paramName, const MLProperty& val);
	//
	void compile();

private:
	MLProcInfo<MLMultiContainer> mInfo; //  unused except for errors


};


#endif // ML_MULT_PROXY_H