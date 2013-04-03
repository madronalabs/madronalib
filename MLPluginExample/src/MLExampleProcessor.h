
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_EXAMPLE_PROCESSOR__
#define __ML_EXAMPLE_PROCESSOR__

#include "MLPluginEditor.h"
#include "MLPluginProcessor.h"
#include "MLExampleProcessor.h"
#include "MLExampleBinaryData.h"

// Set this to 1 for development, to read the XML plugin description direcly from disk.
// This allows changing the plugin graph without recompiling. Set to 0 and run
// the make_data script in the PluginData folder to embed the XML into the plugin. 
#define READ_PLUGIN_FROM_FILE 0

class MLExampleProcessor : public MLPluginProcessor
{
public:
	MLExampleProcessor(){}
	~MLExampleProcessor(){}
	
	void loadDefaultPreset();
	void initializeProcessor(); 
	bool wantsMIDI() {return true;}
	bool silenceInProducesSilenceOut() const { return false; }
};

#endif // __ML_EXAMPLE_PROCESSOR__