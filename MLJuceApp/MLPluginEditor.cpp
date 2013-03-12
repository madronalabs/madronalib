
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginEditor.h"

MLPluginEditor::MLPluginEditor (juce::AudioProcessor* ownerProcessor) : 
	AudioProcessorEditor(ownerProcessor)
{
}

MLPluginEditor::~MLPluginEditor()
{
}

void MLPluginEditor::paint(Graphics& )
{
}

void MLPluginEditor::setWrapperFormat(int format)
{ 
	mWrapperFormat = format; 
	juce::String regStr, bitsStr, pluginType;
	switch(mWrapperFormat)
	{
		case MLPluginFormats::eVSTPlugin:
			pluginType = "VST";
		break;
		case MLPluginFormats::eAUPlugin:
			pluginType = "AU";
		break;
		case MLPluginFormats::eStandalone:
			pluginType = "App";
		break;
		default:
			pluginType = "?";
		break;
	}
		
	#if (__LP64__) || (_WIN64)
		bitsStr = ".64";
	#else
		bitsStr = ".32";
	#endif

	mVersionString = ("version ");
	mVersionString += (JucePlugin_VersionString);
	mVersionString += " (" + pluginType + bitsStr + ")";

	/*
	regStr = mVersionString;
	if (mCanBeLicensed)
	{
	#if DEMO
		regStr += " DEMO";
	#else
		regStr += ", licensed to:";
	#endif
	}
		
	if (mpHeaderUserLabel)
	{
		mpHeaderUserLabel->setText(regStr, false);
	}	
	*/
}

MLRect MLPluginEditor::getWindowBounds()
{
	ComponentPeer *peer = getPeer();
	if(peer)
	{
		MLPoint p(juceToMLPoint(peer->getScreenPosition()));
		MLRect r(juceToMLRect(peer->getBounds()));
		return r + p;
	}
	return MLRect();
}


