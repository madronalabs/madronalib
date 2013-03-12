
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINEDITOR_H__
#define __ML_PLUGINEDITOR_H__

#include "MLUI.h"
#include "MLPluginFormats.h"

class MLPluginProcessor;

class MLPluginEditor : 
	public juce::AudioProcessorEditor
{
public:
	MLPluginEditor (juce::AudioProcessor* ownerProcessor);
    ~MLPluginEditor();

	void paint(Graphics& g);

	virtual void setWrapperFormat(int format);
	
	MLRect getWindowBounds();

protected:
	int mWrapperFormat;
	juce::String mVersionString;
};

#endif  // __ML_PLUGINEDITOR_H__
