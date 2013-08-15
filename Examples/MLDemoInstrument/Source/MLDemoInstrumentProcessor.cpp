
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "AppConfig.h"
#include "MLDemoInstrumentProcessor.h"

AudioProcessor* JUCE_CALLTYPE createPluginFilter();
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	MLPluginProcessor* pFilter = new MLDemoInstrumentProcessor();
	
	if (!READ_PLUGIN_FROM_FILE)
	{
		// initialize filter with Example description
		pFilter->loadPluginDescription((const char*) MLDemoInstrumentBinaryData::mlexample_xml);
	}
	else 
	{
		MLError() << "NOTE: loading Processor from disk file!\n";
		File xmlFile("~/Dev/madronalib/MLPluginExample/PluginData/BinarySrc/MLExample.xml");

		if (xmlFile.exists())
		{
			String xmlStr = xmlFile.loadFileAsString();
			pFilter->loadPluginDescription((const char*)xmlStr.toUTF8());
		}
		else
		{
			debug() << "couldn't read plugin description file!\n";
		}
	}
	
    return pFilter;	
}

// set the default preset.
void MLDemoInstrumentProcessor::loadDefaultPreset()
{
	setDefaultParameters();
	//debug() << "Example: loading default preset\n";
	//const String d((const char*) MLDemoInstrumentBinaryData::default_xml);
	//setStateFromText(d);
}

void MLDemoInstrumentProcessor::initializeProcessor()
{

}

// Send musical scale to any of our processors that need it.
//
void MLDemoInstrumentProcessor::broadcastScale(const MLScale* pScale)
{

}



