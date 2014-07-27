
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppState.h"

MLAppState::MLAppState(MLModel* pM, MLAppView* pV, const char* makerName, const char* appName, int version) :
    MLPropertyListener(pM),
    MLPropertyModifier(pM),
	mpAppView(pV),
	mpMakerName(makerName),
	mpAppName(appName),
	mVersion(version)
{
	updateAllProperties();
	startTimer(1000);
}

MLAppState::~MLAppState()
{

}

void MLAppState::timerCallback()
{
	updateChangedProperties();
}

// --------------------------------------------------------------------------------
// MLPropertyListener implementation
// an updateChangedProperties() is needed to get these actions sent by the Model.
//
void MLAppState::doPropertyChangeAction(MLSymbol p, const MLProperty & val)
{
    // nothing to do here, but we do need to be an MLPropertyListener in order to
    // states of all the Properties.
	//debug() << "MLAppState::doPropertyChangeAction: " << p << " to " << val << "\n";
}

// --------------------------------------------------------------------------------

void MLAppState::saveState()
{
	// get directory
	File dir(getStateDir());
	if (dir.exists())
	{
		if(!dir.isDirectory())
		{
			MLError() << "MLAppState:: file present instead of directory " << 
				dir.getFileName() << "!  Aborting.\n";
		}
	}
	else 
	{
		// make directory		
		Result r = dir.createDirectory();
		if(r.failed())
		{
			MLError() << "Error creating state directory: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// make state file
	File stateFile(getStateFile());
	if (!stateFile.exists())
	{
		// make state file		
		Result r = stateFile.create();
		if(r.failed())
		{
			MLError() << "Error creating state file: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// get app state as JSON container
	cJSON* root = cJSON_CreateObject();
	if(root)
	{
		// get Model parameters
		std::map<MLSymbol, PropertyState>::iterator it;
		for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
		{
			MLSymbol key = it->first;
			const char* keyStr = key.getString().c_str();
			PropertyState& state = it->second;
			switch(state.mValue.getType())
			{
				case MLProperty::kFloatProperty:
					cJSON_AddNumberToObject(root, keyStr, state.mValue.getFloatValue());
					break;
				case MLProperty::kStringProperty:
					cJSON_AddStringToObject(root, keyStr, state.mValue.getStringValue()->c_str());
					break;
				case MLProperty::kSignalProperty:
				{
					// make and populate JSON object representing signal
					cJSON* signalObj = cJSON_CreateObject();
					const MLSignal* sig = state.mValue.getSignalValue();
					cJSON_AddStringToObject(signalObj, "type", "signal");
					cJSON_AddNumberToObject(signalObj, "width", sig->getWidth());
					cJSON_AddNumberToObject(signalObj, "height", sig->getHeight());
					cJSON_AddNumberToObject(signalObj, "depth", sig->getDepth());
					int size = sig->getSize();
					float* pSignalData = sig->getBuffer();
					cJSON* data = cJSON_CreateFloatArray(pSignalData, size);
					cJSON_AddItemToObject(signalObj, "data", data);
					
					// add signal object to state JSON
					cJSON_AddItemToObject(root, keyStr, signalObj);
				}
				break;
				default:
					MLError() << "MLAppState::saveState(): undefined param type! \n";
					break;
			}
		}
		
		// get View info
		//int w = mpAppView->getWidth();
		//int h = mpAppView->getHeight();
		ComponentPeer* p = mpAppView->getPeer();
		if(p)
		{
			Rectangle<int> b = p->getBounds();
			int boundsArray[4] = {b.getX(), b.getY(), b.getWidth(), b.getHeight()};
			cJSON* cj = cJSON_CreateIntArray(boundsArray, 4);
			cJSON_AddItemToObject(root, "window_bounds", cj);
		}
		
		String stateStr(cJSON_Print(root));
		stateFile.replaceWithText(stateStr);
		cJSON_Delete(root);
	}
	else		
	{
		debug() << "MLAppState::saveState: couldn't create JSON object!\n";
	}
}

// --------------------------------------------------------------------------------
// JSON parser (to move)
//

void MLAppState::loadStateFromJSON(cJSON* pNode, int depth)
{
	while(pNode)
	{
		if(pNode->string)
		{
			switch(pNode->type)
			{
			case cJSON_Number:
//debug() << " depth " << depth << " loading float param " << pNode->string << " : " << pNode->valuedouble << "\n";
				requestPropertyChange(MLSymbol(pNode->string), (float)pNode->valuedouble);
				break;
			case cJSON_String:
//debug() << " depth " << depth << " loading string param " << pNode->string << " : " << pNode->valuestring << "\n";
				requestPropertyChange(MLSymbol(pNode->string), pNode->valuestring);
				break;
			case cJSON_Array: 
				if(!strcmp(pNode->string, "window_bounds"))
				{
					assert(cJSON_GetArraySize(pNode) == 4);
					int x = cJSON_GetArrayItem(pNode, 0)->valueint;
					int y = cJSON_GetArrayItem(pNode, 1)->valueint;
					int w = cJSON_GetArrayItem(pNode, 2)->valueint;
					int h = cJSON_GetArrayItem(pNode, 3)->valueint;
					mpAppView->setPeerBounds(x, y, w, h);
				}
				break;
			case cJSON_Object:
// 	debug() << "looking at object: \n";		
				// see if object is a stored signal
				cJSON* pObjType = cJSON_GetObjectItem(pNode, "type");
				if(pObjType && !strcmp(pObjType->valuestring, "signal") )
				{
//debug() << " depth " << depth << " loading signal param " << pNode->string << "\n";
					MLSignal* pSig;
					int width = cJSON_GetObjectItem(pNode, "width")->valueint;
					int height = cJSON_GetObjectItem(pNode, "height")->valueint;
					int sigDepth = cJSON_GetObjectItem(pNode, "depth")->valueint;
					pSig = new MLSignal(width, height, sigDepth);
					if(pSig)
					{
						// read data into signal and set model param
						float* pSigData = pSig->getBuffer();
						int widthBits = bitsToContain(width);
						int heightBits = bitsToContain(height);
						int depthBits = bitsToContain(sigDepth);
						int size = 1 << widthBits << heightBits << depthBits;
						cJSON* pData = cJSON_GetObjectItem(pNode, "data");
						int dataSize = cJSON_GetArraySize(pData);
						if(dataSize == size)
						{
							// read array							
							cJSON *c=pData->child; 
							int i = 0; 
							while (c)
							{
								pSigData[i++] = c->valuedouble;
								c=c->next; 
							}
						}
						else
						{
							MLError() << "MLAppState::loadStateFromJSON: wrong array size!\n";
						}				
						requestPropertyChange(MLSymbol(pNode->string), *pSig);
					}
				}
			
				break;
			}
		}
	
		if(pNode->child && depth < 1)
		{
			loadStateFromJSON(pNode->child, depth + 1);
		}
		pNode = pNode->next;
	}
}

bool MLAppState::loadSavedState()
{
	bool r = true;
	File stateFile = getStateFile();
	if(stateFile.exists())
	{
		String stateStr(stateFile.loadFileAsString());
		cJSON* root = cJSON_Parse(stateStr.toUTF8());
		if(root)
		{
			loadStateFromJSON(root);		
			cJSON_Delete(root);
		}
		else
		{
			debug() << "MLAppState::loadSavedState: couldn't create JSON object!\n";
#ifdef ML_DEBUG
			debug() << "STATE:\n" << 	stateStr << "\n";
#endif				
		}
	}
	else
	{
		r = false;
	}
	updateAllProperties();
	return r;
}

void MLAppState::loadDefaultState()
{
//	defaultappstate_txt 
}

/*
const std::string& MLAppState::getStateAsText()
{
//	return mStateAsText;
}
*/

void MLAppState::setStateFromText(const std::string& )
{

}

File MLAppState::getStateDir() const
{
 	String makerName(File::createLegalFileName (String(mpMakerName)));
 	String applicationName(File::createLegalFileName (String(mpAppName)));

   #if JUCE_MAC || JUCE_IOS
    File dir ("~/Library/Application Support"); // user Library
    dir = dir.getChildFile (makerName);
    dir = dir.getChildFile (applicationName);

   #elif JUCE_LINUX || JUCE_ANDROID
	File dir ("~/" + "." + makerName + "/" + applicationName);

   #elif JUCE_WINDOWS
    File dir (File::getSpecialLocation (File::userApplicationDataDirectory));

    if (dir == File::nonexistent)
        return File::nonexistent;

    dir = dir.getChildFile(makerName);
    dir = dir.getChildFile(applicationName);
   #endif

	return dir;
}

File MLAppState::getStateFile() const
{
    String applicationName(mpAppName);
	String extension("txt");
	File dir(getStateDir());
    return dir.getChildFile(applicationName + "AppState").withFileExtension (extension);
}



