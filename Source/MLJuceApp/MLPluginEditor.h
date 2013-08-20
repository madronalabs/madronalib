
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINEDITOR_H__
#define __ML_PLUGINEDITOR_H__

#include "MLProjectInfo.h"
#include "MLUI.h"

const int kMLPluginMIDIPrograms = 127;

class MLPluginProcessor;

class MLPluginEditor : 
	public juce::AudioProcessorEditor
{
public:
	MLPluginEditor (juce::AudioProcessor* ownerProcessor);
    ~MLPluginEditor();

	void paint(Graphics& g);
	
	MLRect getWindowBounds();
};

#endif  // __ML_PLUGINEDITOR_H__
