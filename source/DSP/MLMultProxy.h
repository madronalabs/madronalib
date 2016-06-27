
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
	MLProcInfoBase& procInfo() override;
	
	void process(const int frames) override;		
	err prepareToProcess() override;	
	
	void clear() override;
	void clearInputs() override;
	void clearInput(int i) override;	 
	void clearProc();

	MLProc::err setInput(const int idx, const MLSignal& srcSig) override;	
	
	void setParam(const ml::Symbol p, const MLProperty& val) override;	
	
	int getInputIndex(const  ml::Symbol name) override;
	int getOutputIndex(const  ml::Symbol name) override;		
	void createInput(const int idx) override;		
	void resizeInputs(const int n) override;
	void resizeOutputs(const int n) override;
	void dumpProc(int indent) override;
	
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

	void setEnabled(bool t) override;
	bool isEnabled() const override;
	bool isProcEnabled(const MLProc* p) const override;
	
	// ----------------------------------------------------------------
	#pragma mark -

	void setup() override;		

	void collectStats(MLSignalStats* pStats) override;
	void process(const int frames) override;		
	err prepareToProcess() override;	
	void clear() override;

	// not in ContainerBase because this is a virtual method of MLProc.
	bool isContainer(void) override { return true; }

	// masquerade as instance of template class
	MLProcInfoBase& procInfo() override; 
	
	// ----------------------------------------------------------------
	#pragma mark graph creation
	
	MLProc::err setInput(const int idx, const MLSignal& srcSig) override;		
	void setParam(const  ml::Symbol p, const MLProperty& val) override;		
	//
	int getInputIndex(const  ml::Symbol name) override;
	int getOutputIndex(const  ml::Symbol name) override;		
	//
	void resizeInputs(const int n) override;
	void resizeOutputs(const int n) override;

	MLProcPtr newProc(const ml::Symbol className, const ml::Symbol procName) override;
	MLProcPtr getProc(const ml::Path & pathName) override; 
	void addPipe(const ml::Path& src, const  ml::Symbol output, const ml::Path& dest, const  ml::Symbol input) override;
	MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi) override;
	// ----------------------------------------------------------------
	#pragma mark I/O
	//
	void publishInput(const ml::Path & procName, const ml::Symbol inputName, const ml::Symbol alias) override;
	void publishOutput(const ml::Path & procName, const ml::Symbol outputName, const ml::Symbol alias) override;	
	 ml::Symbol getOutputName(int index) override;

	//
	// ----------------------------------------------------------------
	#pragma mark signals
	//
	// methods of MLContainerBase
	MLProc::err addSignalBuffers(const ml::Path & procAddress, const ml::Symbol outputName, 
		const  ml::Symbol alias, int trigMode, int bufLength, int frameSize = 1) override;
	void gatherSignalBuffers(const ml::Path & procAddress, const  ml::Symbol alias, MLProcList& signalBuffers) override;
	
	//
	MLProc::err buildProc(juce::XmlElement* parent) override;
	void dumpGraph(int indent) override;	
	void setProcParams(const ml::Path& procName, juce::XmlElement* pelem) override;
	MLPublishedParamPtr publishParam(const ml::Path & procName, const  ml::Symbol paramName, const ml::Symbol alias, const ml::Symbol type) override;
	void addSetterToParam(MLPublishedParamPtr p, const ml::Path & procName, const ml::Symbol param) override;
	void setPublishedParam(int index, const MLProperty& val) override;
	void routeParam(const ml::Path & procAddress, const ml::Symbol paramName, const MLProperty& val) override;
	//
	void compile() override;

private:
	MLProcInfo<MLMultiContainer> mInfo; //  unused except for errors


};


#endif // ML_MULT_PROXY_H