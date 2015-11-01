
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppState.h"

MLAppState::MLAppState(MLPropertySet* pM, const std::string& name, const std::string& makerName, const std::string& appName, int version) :
    MLPropertyListener(pM),
	mExtraName(name),
	mMakerName(makerName),
	mAppName(appName),
	mAppVersion(version),
	mpTarget(pM)
{
	// default extra name
	if(mExtraName.length() == 0)
	{
		mExtraName = "App";
	}
	
	updateAllProperties();
	startTimer(1000);
}

MLAppState::~MLAppState()
{
	stopTimer();
}

void MLAppState::timerCallback()
{
	updateChangedProperties();
}

void MLAppState::ignoreProperty(MLSymbol property)
{
	mIgnoredProperties.insert(property);
}

// MLPropertyListener implementation
// an updateChangedProperties() is needed to get these actions sent by the Model.
// 
void MLAppState::doPropertyChangeAction(MLSymbol p, const MLProperty & val)
{
    // nothing to do here, but we do need to be an MLPropertyListener in order to
    // know the update states of all the Properties.
	
	// debug() << "MLAppState " << mName << ": doPropertyChangeAction: " << p << " to " << val << "\n";
}

#pragma mark get and save state

MemoryBlock MLAppState::getStateAsBinary()
{
	MemoryBlock bIn;
	String stateStr = getStateAsText();
	if(stateStr.length() > 0)
	{
		int cLen = CharPointer_UTF8::getBytesRequiredFor (stateStr.toUTF8());
		bIn.replaceWith(stateStr.toUTF8(), cLen);
		// TODO compress here
	}
	return bIn;
}

String MLAppState::getStateAsText()
{
	String r;
	cJSON* root = getStateAsJSON();
	if(root)
	{
		char* stateText = cJSON_Print(root);
		r = CharPointer_UTF8(stateText);
		free(stateText);
		cJSON_Delete(root);
	}
	else
	{
		debug() << "MLAppState::getStateAsText: couldn't create JSON object!\n";
	}
	return r;
}

void MLAppState::saveStateToStateFile()
{
	// get directory
	File dir(getAppStateDir());
	if (dir.exists())
	{
		if(!dir.isDirectory())
		{
			debug() << "MLAppState:: file present instead of directory " << 
				dir.getFileName() << "!  Aborting.\n";
		}
	}
	else 
	{
		// make directory		
		Result r = dir.createDirectory();
		if(r.failed())
		{
			debug() << "Error creating state directory: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// make state file
	File stateFile(getAppStateFile());
	if (!stateFile.exists())
	{
		Result r = stateFile.create();
		if(r.failed())
		{
			debug() << "Error creating state file: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// get app state as JSON container
	cJSON* root = getStateAsJSON();
	if(root)
	{
		char* stateText = cJSON_Print(root);
		String stateStr(stateText);
		stateFile.replaceWithText(stateStr);
		cJSON_Delete(root);
		free(stateText);
	}
	else		
	{
		debug() << "MLAppState::saveStateToStateFile: couldn't create JSON object!\n";
	}
}

cJSON* MLAppState::getStateAsJSON()
{
	updateAllProperties();
	
	cJSON* root = cJSON_CreateObject();

	// get Model parameters
	std::map<MLSymbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		MLSymbol key = it->first;
		if(mIgnoredProperties.find(key) == mIgnoredProperties.end())
		{			
			const char* keyStr = key.getString().c_str();
			PropertyState& state = it->second;
			switch(state.mValue.getType())
			{
				case MLProperty::kFloatProperty:
					cJSON_AddNumberToObject(root, keyStr, state.mValue.getFloatValue());
					break;
				case MLProperty::kStringProperty:
					cJSON_AddStringToObject(root, keyStr, state.mValue.getStringValue().c_str());
					break;
				case MLProperty::kSignalProperty:
					{
						// make and populate JSON object representing signal
						cJSON* signalObj = cJSON_CreateObject();
						const MLSignal& sig = state.mValue.getSignalValue();
						cJSON_AddStringToObject(signalObj, "type", "signal");
						cJSON_AddNumberToObject(signalObj, "width", sig.getWidth());
						cJSON_AddNumberToObject(signalObj, "height", sig.getHeight());
						cJSON_AddNumberToObject(signalObj, "depth", sig.getDepth());
						int size = sig.getSize();
						float* pSignalData = sig.getBuffer();
						cJSON* data = cJSON_CreateFloatArray(pSignalData, size);
						cJSON_AddItemToObject(signalObj, "data", data);
						
						// add signal object to state JSON
						cJSON_AddItemToObject(root, keyStr, signalObj);
					}
					break;
				default:
					debug() << "MLAppState::saveStateToStateFile(): undefined param type! \n";
					break;
			}
		}
	}
	
	// add or replace environment info
	cJSON * makerName = cJSON_CreateString(mMakerName.c_str());
	cJSON_ReplaceOrAddItemToObject(root, "maker_name", makerName);
	
	cJSON * appName = cJSON_CreateString(mAppName.c_str());
	cJSON_ReplaceOrAddItemToObject(root, "app_name", appName);

	cJSON * appVersion = cJSON_CreateNumber(mAppVersion);
	cJSON_ReplaceOrAddItemToObject(root, "app_version", appVersion);
	
	return root;
}

#pragma mark load and set state

void MLAppState::setStateFromBinary(const MemoryBlock& bIn)
{
	const void* inData = bIn.getData();
	unsigned int inSize = bIn.getSize();
	// TODO uncompress here
	String stateStr = juce::String::fromUTF8(static_cast<const char *>(inData), inSize);
	setStateFromText(stateStr);
}

bool MLAppState::loadStateFromAppStateFile()
{
	bool r = false;
	File stateFile = getAppStateFile();
	if(stateFile.exists())
	{
		r = setStateFromText(stateFile.loadFileAsString());
	}
	else
	{
		debug() << "MLAppState::loadStateFromAppStateFile: couldn't open file!\n";
	}
	return r;
}

bool MLAppState::setStateFromText(String stateStr)
{
	bool r = false;
	cJSON* root = cJSON_Parse(stateStr.toUTF8());
	if(root)
	{
		setStateFromJSON(root);
		cJSON_Delete(root);
		r = true;
	}
	else
	{
		debug() << "MLAppState::setStateFromText: couldn't create JSON object!\n";
	}
	return r;
}

void MLAppState::setStateFromJSON(cJSON* pNode, int depth)
{
	if(!pNode) return;
	cJSON *child = pNode->child;
	while(child)
	{
		MLSymbol key(child->string);
		if(mIgnoredProperties.find(key) == mIgnoredProperties.end())
		{							
			switch(child->type & 255)
			{
				case cJSON_Number:
					//debug() << " depth " << depth << " loading float param " << child->string << " : " << child->valuedouble << "\n";
					mpTarget->setProperty(key, (float)child->valuedouble);
					break;
				case cJSON_String:
					//debug() << " depth " << depth << " loading string param " << child->string << " : " << child->valuestring << "\n";
					mpTarget->setProperty(key, child->valuestring);
					break;
				case cJSON_Object:
					//debug() << "looking at object: " << child->string << "\n";
					// see if object is a stored signal
					if(cJSON* pObjType = cJSON_GetObjectItem(child, "type"))
					{
						if(!strcmp(pObjType->valuestring, "signal") )
						{
							//debug() << " depth " << depth << " loading signal param " << child->string << "\n";
							int width = cJSON_GetObjectItem(child, "width")->valueint;
							int height = cJSON_GetObjectItem(child, "height")->valueint;
							int sigDepth = cJSON_GetObjectItem(child, "depth")->valueint;
							
							// read data into signal and set model param
							MLSignal signalValue(width, height, sigDepth);
							float* pSigData = signalValue.getBuffer();
							if(pSigData)
							{
								int widthBits = bitsToContain(width);
								int heightBits = bitsToContain(height);
								int depthBits = bitsToContain(sigDepth);
								int size = 1 << widthBits << heightBits << depthBits;
								
								cJSON* pData = cJSON_GetObjectItem(child, "data");
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
									debug() << "MLAppState::setStateFromJSON: wrong array size!\n";
								}
								mpTarget->setProperty(key, signalValue);
							}
						}
					}
					else
					{
						//debug() << " recursing into object " << child->string << ": \n";
						setStateFromJSON(child, depth + 1);
					}
					break;
				case cJSON_Array:
				default:
					break;
			}
		}
		child = child->next;
	}
}

void MLAppState::loadDefaultState()
{
//	load defaultappstate_txt ?
}

File MLAppState::getAppStateDir() const
{
 	String makerName(File::createLegalFileName (String(mMakerName)));
 	String applicationName(File::createLegalFileName (String(mAppName)));

   #if JUCE_MAC || JUCE_IOS
    File dir ("~/Library/Application Support"); // user Library
    dir = dir.getChildFile (makerName);
    dir = dir.getChildFile (applicationName);

   #elif JUCE_LINUX || JUCE_ANDROID
	File dir ("~/" "." + makerName + "/" + applicationName);

   #elif JUCE_WINDOWS
    File dir (File::getSpecialLocation (File::userApplicationDataDirectory));

    if (dir == File::nonexistent)
        return File::nonexistent;

    dir = dir.getChildFile(makerName);
    dir = dir.getChildFile(applicationName);
   #endif

	return dir;
}

File MLAppState::getAppStateFile() const
{
    String applicationName(mAppName);
	String extension("txt");
	File dir(getAppStateDir());
	std::string extra;
    return dir.getChildFile(applicationName + mExtraName + "State").withFileExtension (extension);
}

void MLAppState::clearStateStack()
{
	mStateStack.clear();
}

void MLAppState::pushStateToStack()
{
	mStateStack.push_back(getStateAsBinary());
}

void MLAppState::popStateFromStack()
{
	if(mStateStack.size() > 0)
	{
		MemoryBlock p = mStateStack.back();
		mStateStack.pop_back();
		setStateFromBinary(p);
	}
}

void MLAppState::returnToFirstSavedState()
{
	MemoryBlock p;
	if(mStateStack.size() > 0)
	{
		p = mStateStack[0];
		mStateStack.clear();
		mStateStack.push_back(p);
		setStateFromBinary(p);
	}
}
