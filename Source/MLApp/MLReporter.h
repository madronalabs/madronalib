
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_REPORTER_H
#define __ML_REPORTER_H

#include "MLModel.h"
#include "MLWidget.h"
#include <map>
#include "pa_ringbuffer.h"

#pragma mark property viewing

class MLPropertyView
{
	friend class MLReporter;
public:
	MLPropertyView(MLWidget* w, MLSymbol attr);
	~MLPropertyView();
	void view(const MLProperty& v) const;
	
private:
	MLWidget* mpWidget;
	MLSymbol mAttr;
};

typedef std::tr1::shared_ptr<MLPropertyView> MLPropertyViewPtr;
typedef std::list<MLPropertyViewPtr> MLPropertyViewList;
typedef std::map<MLSymbol, MLPropertyViewList> MLPropertyViewListMap;

#pragma mark MLReporter 

// Reporter listens to a Model and reports its changing properties by setting
// Attributes of Widgets. Properties may contain float, string or signal values.
//
class MLReporter :
	public MLPropertyListener
{
public:
	MLReporter(MLPropertySet* m);
    ~MLReporter();
	
	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	void addPropertyViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr);
	void viewProperties();

protected:
	MLPropertyViewListMap mPropertyViewsMap;
	std::vector<MLSymbol> mSymbolData;
	PaUtilRingBuffer mSymbolRing;
	
private:
	// TODO write a Timer class. juce::Timer is the only reason Juce is needed here. temporary.
	class ReporterTimer : public juce::Timer
	{
	public:
		ReporterTimer(MLReporter*);
		~ReporterTimer();
		void timerCallback();
	private:
		MLReporter* mpOwner;
	};
	std::tr1::shared_ptr<ReporterTimer> mpTimer;
};

#endif // __ML_REPORTER_H