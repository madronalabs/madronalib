
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

MLPluginController::MLPluginController(MLPluginProcessor* const pProcessor) :
	mpProcessor(pProcessor),
	MLResponder(),
	MLReporter(pProcessor),
	MLSignalReporter(pProcessor),
	mWrapperFormat(MLPluginFormats::eUndefined),
	mFileLocationsOK(false),
	mpView(nullptr),
	mCurrentPresetIndex(0)
{
	// get parameters and initial values from our processor and create corresponding model params. 
	MLPluginProcessor* const pProc = getProcessor();
	MLModel* pModel = getModel();
	int params = pProc->getNumParameters();
	for(int i=0; i<params; ++i)
	{
		MLPublishedParamPtr p = pProc->getParameterPtr(i);
		MLPublishedParam* param = &(*p);
		if(param)
		{
			MLSymbol paramName = param->getAlias();
			MLParamValue val = param->getValue();
			pModel->setModelParam(paramName, val);
		}
	}

	// get data folder locations
	mFactoryPresetsFolder = getDefaultFileLocation(kFactoryPresetFiles);
	mUserPresetsFolder = getDefaultFileLocation(kUserPresetFiles);
	mScalesFolder = getDefaultFileLocation(kScaleFiles);
	if ((mFactoryPresetsFolder == File::nonexistent) ||
		(mUserPresetsFolder == File::nonexistent) ||
		(mScalesFolder == File::nonexistent))
	{
		MLError() << "MLPluginEditor: couldn't get data files!\n";
	}
	else
	{
		mFileLocationsOK = true;
		mCurrentPresetFolder = mUserPresetsFolder;
	}	
	
	mMIDIProgramFiles.resize(kMLPluginMIDIPrograms);
}

MLPluginController::~MLPluginController()
{
}

MLAppView* MLPluginController::getView() 
{ 
	return mpView; 
}

void MLPluginController::setView(MLAppView* v) 
{ 
	mpView = v; 
}

void MLPluginController::setPluginWrapperFormat(int format)
{ 
	mWrapperFormat = static_cast<MLPluginFormats::pluginFormat>(format); 
	std::string regStr, bitsStr, pluginType;
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

	mVersionString = std::string("version ");
	mVersionString += (std::string(JucePlugin_VersionString));
	mVersionString += " (" + pluginType + bitsStr + ")";

	
	regStr = mVersionString;
	#if DEMO
		regStr += " DEMO\n";
	#else
		regStr += ", licensed to:\n";
	#endif
		
	
	// TODO typed widgets?
	MLAppView* myView = getView();
	MLLabel* regLabel = static_cast<MLLabel*>(myView->getWidget("reg"));
	if(regLabel)
	{
		regLabel->setStringAttribute(MLSymbol("text"), regStr);
	}	
}

// --------------------------------------------------------------------------------
#pragma mark MLButton::Listener

void MLPluginController::buttonClicked (MLButton* button)
{	
	const MLSymbol paramName = button->getParamName();
	MLPluginProcessor* const filter = getProcessor();
	const bool state = button->getToggleState();
	if (filter)
	{
		int idx = filter->getParameterIndex(paramName);
		if (idx >= 0)
		{
			float bVal = state ? button->getOnValue() : button->getOffValue();
			filter->MLSetParameterNotifyingHost(idx, bVal);
		}
	}
}

// --------------------------------------------------------------------------------
#pragma mark MLDial::Listener	
	
void MLPluginController::dialDragStarted (MLDial* pSlider)
{
	const MLSymbol paramName = pSlider->getParamName();
	MLPluginProcessor* const filter = getProcessor();
	if (filter)
	{
		int idx = filter->getParameterIndex(paramName);
		if (idx > 0)
			filter->beginParameterChangeGesture (idx);
	}
}

void MLPluginController::dialDragEnded (MLDial* pSlider)
{
	const MLSymbol paramName = pSlider->getParamName();
	MLPluginProcessor* const filter = getProcessor();
	if (filter)
	{
		int idx = filter->getParameterIndex(paramName);
		if (idx > 0)
			filter->endParameterChangeGesture (idx);
	}
}

// send Slider changes to filter. 
void MLPluginController::dialValueChanged (MLDial* pSlider)
{
    MLPluginProcessor* const filter = getProcessor();
	float val = 0, minVal = 0, maxVal = 0;
	int paramIdx = -1;
	
	if (pSlider)
	{
		const MLSymbol paramName = pSlider->getParamName();

		if (pSlider->isMultiValued())
		{
//			minVal = pSlider->getMinValue();
		}
		else
		{
			if (!pSlider->isTwoValued())
			{
				val = pSlider->getValue();				
				paramIdx = filter->getParameterIndex(paramName);
				if (paramIdx >= 0)
				{
					float paramVal = filter->getParameter(paramIdx);
					if (val != paramVal)
					{
						filter->MLSetParameterNotifyingHost(paramIdx, val);
					}
				}
			}
			
			// NOT TESTED
			if (pSlider->isTwoOrThreeValued())
			{
				const std::string paramStr = paramName.getString();
				minVal = pSlider->getMinValue();
				paramIdx = filter->getParameterIndex(MLSymbol(paramStr + "_min"));
	//debug() << "index of " << minName << " is " << index << ".\n";
				if (paramIdx >= 0)
				{
					filter->MLSetParameterNotifyingHost(paramIdx, minVal);
				}
				maxVal = pSlider->getMaxValue();
				paramIdx = filter->getParameterIndex(MLSymbol(paramStr + "_max"));
	//debug() << "index of " << maxName << " is " << index << ".\n";
				if (paramIdx >= 0)
				{
					filter->MLSetParameterNotifyingHost(paramIdx, maxVal);
				}
			}		
		}

//		debug() << "dial: " << static_cast<void *>(pSlider) << ", index " << paramIdx << 
//			" [" << minVal << " " << val << " " << maxVal << "]\n";

	}
}

// --------------------------------------------------------------------------------
#pragma mark MLMultiSlider::Listener	
	
void MLPluginController::multiSliderDragStarted (MLMultiSlider* pSlider, int idx)
{
	MLPluginProcessor* const filter = getProcessor();
	if (!filter) return;
	const MLSymbol paramName = pSlider->getParamName();
	const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
	int paramIdx = filter->getParameterIndex(paramName);
	if (paramIdx >= 0)
	{
		filter->beginParameterChangeGesture (paramIdx);
	}
}

void MLPluginController::multiSliderDragEnded (MLMultiSlider* pSlider, int idx)
{
	MLPluginProcessor* const filter = getProcessor();
	if (!filter) return;
	const MLSymbol paramName = pSlider->getParamName();
	const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
	int paramIdx = filter->getParameterIndex(paramName);
	if (paramIdx >= 0)
	{
		filter->endParameterChangeGesture (paramIdx);
	}
}

void MLPluginController::multiSliderValueChanged (MLMultiSlider* pSlider, int idx)
{
    MLPluginProcessor* const filter = getProcessor();
	if (!filter) return;
	float val = 0.;
	int paramIdx = -1;
	
	if (pSlider)
	{
		MLSymbol paramName = pSlider->getParamName();
		const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
		paramIdx = filter->getParameterIndex(nameWithNumber);
		val = pSlider->getValue(idx);		

//debug() << "    name: " << nameWithNumber << " index " << paramIdx << " ...\n";
		
		if (paramIdx >= 0)
		{
			float paramVal = filter->getParameter(paramIdx);
			if(val != paramVal)
			{
				filter->MLSetParameterNotifyingHost(paramIdx, val);
			}
		}
		else
		{
			debug() << "MLPluginController::multiSliderValueChanged: couldn't get param index for " << nameWithNumber << "\n";
		}
	}
}

void MLPluginController::multiButtonValueChanged (MLMultiButton* pButton, int idx)
{
    MLPluginProcessor* const filter = getProcessor();
	if (!filter) return;
	float val = 0.;
	int paramIdx = -1;
	
	if (pButton)
	{
		MLSymbol paramName = pButton->getParamName();
		const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
		paramIdx = filter->getParameterIndex(nameWithNumber);
		val = pButton->getValue(idx);		

//debug() << "    paramName name: " << nameWithNumber << " index " << paramIdx << " ...\n";
		
		if (paramIdx >= 0)
		{
			float paramVal = filter->getParameter(paramIdx);
			if(val != paramVal)
			{
				filter->MLSetParameterNotifyingHost(paramIdx, val);
			}
		}
		else
		{
			debug() << "MLPluginController::multiButtonValueChanged: couldn't get param index for " << nameWithNumber << "\n";
		}
	}
}

// --------------------------------------------------------------------------------
#pragma mark presets

void MLPluginController::prevPreset()
{
	int idx = mCurrentPresetIndex - 1;
	if (idx < 0) idx = mMenuPresetFiles.size() - 1;	
	loadPresetByIndex(idx);
}

void MLPluginController::nextPreset()
{
	int idx = mCurrentPresetIndex + 1;
	if (idx >= mMenuPresetFiles.size()) idx = 0;
	loadPresetByIndex(idx);
}

String MLPluginController::getPresetString(int n)
{ 
	int idx = n - 1;
	if (idx >= 0)
	{
		MLMenuMapT::iterator menuIter(mMenuMap.find("preset"));
		if (menuIter != mMenuMap.end())
		{
			MLMenuPtr menu = menuIter->second;
			return String(menu->getItemString(idx).c_str()); 
		}
	}
	return String();
}

void MLPluginController::loadPresetByIndex (int idx)
{
	MLPluginProcessor* const filter = getProcessor();
	
	// pause timer
	//stopTimer();
	
debug() << mMenuPresetFiles.size() << " presets\n";

	if(idx < mMenuPresetFiles.size())
	{
		debug() << "loading preset " << idx << ": " << mMenuPresetFiles[idx].getFileNameWithoutExtension() << "\n";		
		filter->loadStateFromFile(mMenuPresetFiles[idx]);	
		mCurrentPresetFolder = mMenuPresetFiles[idx].getParentDirectory();
	}
	mCurrentPresetIndex = idx;
	
	// resume
	//startTimer(kTimerInterval);
}

int MLPluginController::getIndexOfPreset(const std::string& dir, const std::string& name)
{
	int index = -1;
	const int kDefaultIdx = 0;
	int size = mMenuPresetFiles.size();
	String searchName (name.c_str());
	String searchDir (dir.c_str());
	String presetName, presetDir;
	std::string defaultStr("default");
	
	if ((dir == defaultStr) && (name == defaultStr))
	{
		index = kDefaultIdx;
	}
		
	// first try to match both folder and preset name
	if (index == -1)
	{
		for(int i=0; i < size; ++i)
		{
			const File& f = mMenuPresetFiles[i];
			presetName = f.getFileNameWithoutExtension();
			presetDir = f.getParentDirectory().getFileNameWithoutExtension();
			if((searchName == presetName) && (searchDir == presetDir))
			{
				index = i;
				break;
			}
		}
	}
	
	// settle for preset name only
	if (index == -1)
	{
		for(int i=0; i < size; ++i)
		{
			const File& f = mMenuPresetFiles[i];
			presetName = f.getFileNameWithoutExtension();			
			if(searchName == presetName)
			{
				index = i;
				break;
			}
		}
	}
	
	// settle for preset name only
	if (index == -1)
	{
		index = kDefaultIdx;
	}
	
	return index;
}

// --------------------------------------------------------------------------------
#pragma mark menus

static void menuItemChosenCallback (int result, MLPluginController* pC, MLMenuPtr menu);

void MLPluginController::setupMenus()
{
	mMenuMap["preset"] = MLMenuPtr(new MLMenu("preset"));
	populatePresetMenu();
	
	mMenuMap["key_scale"] = MLMenuPtr(new MLMenu("key_scale"));
	populateScaleMenu();

}

void MLPluginController::showMenu (MLSymbol menuName, MLSymbol instigatorName)
{	
	if(!mpView) return;
	
	MLMenuMapT::iterator menuIter(mMenuMap.find(menuName));
	if (menuIter != mMenuMap.end())
	{
		MLMenuPtr menu = menuIter->second;
		menu->setInstigator(instigatorName);

		// find instigator widget and set value to 1 - this depresses menu buttons for example
		MLWidget* pInstigator = mpView->getWidget(instigatorName);
		if(pInstigator != nullptr)
		{
			pInstigator->setAttribute("value", 1);
		}
		
		const int u = pInstigator->getWidgetGridUnitSize();
		int height = ((float)u)*0.35f;
		height = clamp(height, 12, 128);
		
		// update menus that might change each time
		if (menuName == "preset")
		{
			populatePresetMenu();
		}
		else if (menuName == "key_scale")
		{
			populateScaleMenu();
		}
		
		if(menu != MLMenuPtr())
		{
			if(pInstigator != nullptr)
			{
				Component* pInstComp = pInstigator->getComponent();
				if(pInstComp)
				{
					PopupMenu& juceMenu = menu->getJuceMenu();
					juceMenu.showMenuAsync (PopupMenu::Options().withTargetComponent(pInstComp).withStandardItemHeight(height),
						ModalCallbackFunction::withParam(menuItemChosenCallback, this, menu));
				}
			}
		}
	}
}

void MLPluginController::doPresetMenu(int result)
{
	String presetStr;
	if (result > mPresetMenuStartItems)
	{
		// load a preset
		String resultStr = getPresetString(result);
		// sets Model patch param, which updates menu display
		loadPresetByIndex(result - mPresetMenuStartItems - 1);
	}
	else switch(result)
	{
		// do another menu command
		case (0):	// dismiss
		default:
		break;
		case (1):	// save over previous
			if(getProcessor()->saveStateAsVersion(mCurrentPresetFolder) != MLProc::OK)  // TODO this should be MLErr
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					getProcessor()->getErrorMessage(),
					"OK");
			}
			// rescan all is sort of lazy, but doesn't seem too slow for now
			populatePresetMenu();
		break;
		case (2):	// save as version in current dir
			if(getProcessor()->saveStateOverPrevious(mCurrentPresetFolder) != MLProc::OK)  // TODO this should be MLErr
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					getProcessor()->getErrorMessage(),
					"OK");
			}
			// rescan all is sort of lazy, but doesn't seem too slow for now
			populatePresetMenu();
		break;
		case (3):	// save as ...
		{
			FileChooser fc ("Save preset as...", mUserPresetsFolder, String::empty, true); // true = native file chooser
			if (fc.browseForFileToSave (true))
			{
				File saveFile = fc.getResult();	
				getProcessor()->saveStateToFile(saveFile, mWrapperFormat);
				populatePresetMenu();
			}
		}
		break;
		case (4):	// revert
			getProcessor()->returnToLatestStateLoaded();
		break;

		case (5):	// copy
			getProcessor()->getStateAsText (presetStr);
			SystemClipboard::copyTextToClipboard (presetStr);
		break;
		case (6):	// paste
			presetStr = SystemClipboard::getTextFromClipboard();
			getProcessor()->setStateFromText (presetStr);
		break;

#if ML_MAC
		case (7):	// show convert alert box
			convertPresets();
			populatePresetMenu();
		break;
#endif

	}
}

void MLPluginController::doScaleMenu(int result)
{
	MLPluginProcessor* const filter = getProcessor();
	String scaleName, scaleDir;
	
	if (result > 1)
	{
		int scaleIdx = result - 2;
		scaleName = mScaleMenuFiles[scaleIdx].getFileNameWithoutExtension();
		scaleDir = mScaleMenuFiles[scaleIdx].getParentDirectory().getFileNameWithoutExtension();
		filter->loadScale (mScaleMenuFiles[scaleIdx]);
		mCurrentScaleDir = scaleDir;
	}
	
	else switch(result)
	{
		case (0):	// do nothing
		default:
		break;
		case (1):	// 12-equal
		{
			filter->loadDefaultScale();
			scaleName = "12-equal";
			mCurrentScaleDir = scaleDir;
		}
		break;
	}
	
	// notify Model of change
	int menuIdx = result - 1;
	MLMenuPtr menu = mMenuMap["key_scale"];
	if (menu != MLMenuPtr())
	{
		mpProcessor->setModelParam("key_scale", menu->getItemString(menuIdx));
	}
}

static void menuItemChosenCallback (int result, MLPluginController* pC, MLMenuPtr menu)
{
	MLWidgetContainer* pView = pC->getView();
	if(pView)
	{	
		MLWidget* pInstigator = pView->getWidget(menu->getInstigator());
		if(pInstigator != nullptr)
		{
			// turn instigator Widget off
			pInstigator->setAttribute("value", 0);
		}
	}
	pC->menuItemChosen(menu->getName(), result);
}

void MLPluginController::menuItemChosen(MLSymbol menuName, int result)
{
	if (result > 0)
	{
		// do action
		MLAppView* pV = getView();
		if(pV)
		{
			if (menuName == "preset")
			{
				doPresetMenu(result);
			}	
			else if(menuName == "key_scale")
			{
				doScaleMenu(result);
			}			
		}		
	}
}

// get all files in the given directory and its immediate subdirectories that have the 
// given extension. append the Files to results. if the three optional menu params 
// are specified, add menu items to the menus.  
//
//
void MLPluginController::findFilesOneLevelDeep(File& startDir, String extension, Array<File>& results, MLMenu* pMenu)
{
	if (!startDir.isDirectory()) return;
	bool doMenus = (pMenu != nullptr);
	Array<File> startDirArray;
	const int level0FilesToFind = File::findFilesAndDirectories | File::ignoreHiddenFiles;
	const int level1FilesToFind = File::findFiles | File::ignoreHiddenFiles;
	int userFiles = startDir.findChildFiles(startDirArray, level0FilesToFind, false);
	int midiPgmCount = 0; 
	for(int i=0; i<userFiles; ++i)
	{
		File f = startDirArray[i];
		if (f.isDirectory())
		{
			// only recurse one level deep.
			String category = f.getFileNameWithoutExtension();
			File subdir = startDir.getChildFile(category);
			bool doMIDI = (category == "MIDI Programs"); 
			
			if(subdir.exists())
			{
				Array<File> subdirArray;
				MLMenuPtr subPop(new MLMenu(category.toUTF8()));
				if(doMenus)
				{
					subPop->setItemOffset(pMenu->getNumItems());
				}
				int filesInCategory = subdir.findChildFiles(subdirArray, level1FilesToFind, false);
				for(int j=0; j<filesInCategory; ++j)
				{
					File f2 = subdirArray[j];
					if (f2.hasFileExtension(extension))
					{
						String subPreset = f2.getFileNameWithoutExtension();
						if(doMIDI && (midiPgmCount < kMLPluginMIDIPrograms))
						{
							String tagStr;
							tagStr << " (#" << midiPgmCount << ")";
							
							// save index of MIDI program
							mMIDIProgramFiles[midiPgmCount] = f2;
							midiPgmCount++;
							subPreset += tagStr;
						}
						results.add(f2);
						if(doMenus)
						{
							subPop->addItem(subPreset.toUTF8());
						}	
					}
				}
				if(doMenus)
				{
					pMenu->addSubMenu(subPop);
				}
			}					
		}
		else if (f.hasFileExtension(extension))
		{
			// add preset to top level if not in subdirectory
			String preset = f.getFileNameWithoutExtension();
			results.add(f);
			if(doMenus)
			{
				pMenu->addItem(preset.toUTF8());
			}
		}
	}
}

void MLPluginController::populatePresetMenu() 
{
	MLMenuMapT::iterator menuIter(mMenuMap.find("preset"));
	if (menuIter == mMenuMap.end())
	{
		MLError() << "MLPluginController::populatePresetMenu(): menu not found!\n";
	}			
	MLMenuPtr menu = menuIter->second;
	menu->clear();
	mMenuPresetFiles.clear();
	mPresetMenuStartItems = 0;
	
#if DEMO	
	menu->addItem("Save as version", false);
#else
	menu->addItem("Save as version");
#endif	
	
#if DEMO
	menu->addItem("Save", false);
#else

	menu->addItem("Save");
#endif	
	
#if DEMO
	menu->addItem("Save as...", false); 
#else
	menu->addItem("Save as...");
#endif	

	menu->addItem("Revert to saved"); 

	menu->addSeparator();		

	menu->addItem("Copy to clipboard");
	menu->addItem("Paste from clipboard");
	
#if ML_MAC
	menu->addSeparator();		
	menu->addItem("Convert presets..."); 
#endif

	// get plugin type and set extension to look for
	String pluginType;
	String presetFileType;
	switch(mWrapperFormat)
	{
		case MLPluginFormats::eVSTPlugin:
			pluginType = "VST";
			presetFileType = ".mlpreset";
		break;
		case MLPluginFormats::eStandalone:
			pluginType = "App";
			presetFileType = ".mlpreset";
		break;
		case MLPluginFormats::eAUPlugin:
			pluginType = "AU";
			presetFileType = ".aupreset";
		break;
		default:
			pluginType = "undefined!";
		break;
	}
	
	// find and add patch files to menus
	mPresetMenuStartItems = menu->getNumItems();
	if (mFileLocationsOK)
	{
		menu->addSeparator();		
		findFilesOneLevelDeep(mUserPresetsFolder, presetFileType, mMenuPresetFiles, &(*menu));
		menu->addSeparator(); 				
		findFilesOneLevelDeep(mFactoryPresetsFolder, presetFileType, mMenuPresetFiles, &(*menu));	
			
//		debug() << "MLPluginController: " << mMenuPresetFiles.size() << " preset files.\n";
		
		// send MIDI program info to processor
		MLPluginProcessor* const pProc = getProcessor();
		if(pProc)
		{
			pProc->clearMIDIProgramFiles();
			for(int i=0; i<kMLPluginMIDIPrograms; ++i)
			{
				if(mMIDIProgramFiles[i].exists())
				{
			debug() << "MIDI pgm " << i << " " << mMIDIProgramFiles[i].getFileName() << "\n";
					pProc->setMIDIProgramFile(i, mMIDIProgramFiles[i]);
				}
			}
		}
	}
	// sync current preset index to new list
	MLPluginProcessor* const pProc = getProcessor();
	mCurrentPresetIndex = getIndexOfPreset(pProc->getModelStringParam("preset_dir"), pProc->getModelStringParam("preset_name"));
}

// create a menu of the factory scale presets.
//
void MLPluginController::populateScaleMenu()
{
	String scaleName, scaleDir;
	MLMenuMapT::iterator menuIter(mMenuMap.find("key_scale"));
	if (menuIter == mMenuMap.end())
	{
		MLError() << "MLPluginController::populateScaleMenu(): menu not found!\n";
	}			
	MLMenuPtr menu = menuIter->second;
	menu->clear();
	menu->addItem("12-equal");
	if(mFileLocationsOK) 
	{
		findFilesOneLevelDeep(mScalesFolder, ".scl", mScaleMenuFiles, &(*menu));
	}
}

/* 

// TEST implementation TODO in v.2
// settings menu component. just contains number and animate buttons now.
// in the future this should contain anything that affects the plugin but
// not its sound.  Examples would be other look and feel changes or
// localization.
//
class SettingsComponent : public Component
{
public:
    SettingsComponent(const String& componentName = String::empty) : Component(componentName)
    {
		// num display toggle
		mNumbersButton = new MLButton("numbers");
		mNumbersButton->setDrawName(true);
		mNumbersButton->setBounds(10, 10, 32, 26);
		addAndMakeVisible(mNumbersButton);
		mComponents.add(mNumbersButton);		
		setSize(192, 64);
    }

    ~SettingsComponent()
    {
    }
	
	void buttonClicked (Button* button)
	{
	debug() << "YA";
	
		if (button == mNumbersButton)
		{
			debug() << "numbers!\n";
		}
	}
	
private:
	OwnedArray<Component> mComponents;
	MLButton* mNumbersButton;
};


void MLPluginEditor::doSettingsMenu()
{
	SettingsComponent settings("SETTINGS");
	MLCallOutBox callOut (settings, *mHeaderSettingsButton, this);
	callOut.runModalLoopAsync();
}
*/


#if ML_MAC
// 
void MLPluginController::getPresetsToConvert(Array<File>* pResults)
{
	Array<File> fromFiles;

	String pluginType;
	String fromFileType, toFileType;
	switch(mWrapperFormat)
	{
		case MLPluginFormats::eRTASPlugin:
			pluginType = "RTAS";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case MLPluginFormats::eVSTPlugin:
			pluginType = "VST";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case MLPluginFormats::eAUPlugin:
			pluginType = "AU";
			fromFileType = ".mlpreset";
			toFileType = ".aupreset";
		break;
		default:
			pluginType = "undefined!";
			fromFileType = ".undefined";
			toFileType = ".undefined";
		break;
	}
	// debug() << "getPresetsToConvert: my plugin type is " << pluginType << "\n";
	
	if (mFileLocationsOK)
	{	
		// get lists of files but don't add to menus
		findFilesOneLevelDeep(mUserPresetsFolder, fromFileType, fromFiles, 0);
		findFilesOneLevelDeep(mFactoryPresetsFolder, fromFileType, fromFiles, 0);
	}
	
	//debug() << "convertPresets: got " << fromFiles.size() << " preset files of other type.\n";

	// for each fromType file, look to see if it has a toType counterpart. 
	// if not, add it to list. 
	for(int i=0; i<fromFiles.size(); i++)
	{
		// debug() << fromFiles[i].getFileNameWithoutExtension() << " : ";
		
		if (!(fromFiles[i].withFileExtension(toFileType).exists()))
		{
		//debug() << " NOT in our format\n";		
			pResults->add(fromFiles[i]);
		}
		else
		{
		//debug() << " IS in our format\n";
		}
	}

	// debug() << "got " << pResults->size() << " preset files to convert.\n";
} 


// Convert all the preset files in the array parameter to the format of the current
// plugin version.  Just loads each preset into the filter and saves it out again. 
//
class PresetConverterThread  : public ThreadWithProgressWindow
{
public:
    PresetConverterThread(Array<File> &pFilesToConvert, MLPluginProcessor* filter, MLPluginFormats::pluginFormat format)
        : ThreadWithProgressWindow (String::empty, true, true),
		mpFiles(pFilesToConvert),
		mpFilter(filter),
		mFormat(format)
    {
        setStatusMessage ("Getting ready...");
		switch(mFormat)
		{
			case MLPluginFormats::eVSTPlugin:
				mExtension = ".mlpreset";
			break;
			case MLPluginFormats::eAUPlugin:
				mExtension = ".aupreset";
			break;
			default:
				mExtension = ".undefined";
			break;
		}			
    }

    ~PresetConverterThread()
    {
    }

    void run()
    {
        setProgress (-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar..
        setStatusMessage ("Preparing to convert...");
        wait (1000);
		
		const int numFiles = mpFiles.size();
 
		setStatusMessage ("Converting presets...");
		for (int i = 0; i < numFiles; ++i)
        {
            // must check this often, because this is
            // how we know if the user has pressed 'cancel'
            if (threadShouldExit())
                return;
				
			// do the conversion.
			File fromFile, toFile;
			fromFile = mpFiles[i];		
			toFile = fromFile.withFileExtension(mExtension);					
			if (!toFile.exists())
			{				
				if (mExtension == ".mlpreset")
				{
					ScopedPointer<XmlElement> xml(loadPropertyFileToXML(fromFile));
					xml->writeToFile(toFile, String::empty);
					wait(10); 
				}
				else if (mExtension == ".aupreset")
				{
					mpFilter->loadStateFromFile(fromFile);
					mpFilter->saveStateToFile(toFile, mFormat);
					wait(10); 
				}
			}
					
            setProgress (i / (double) numFiles);
        }
    }
	
	private:
		Array<File> &mpFiles;
		MLPluginProcessor* mpFilter;
		MLPluginFormats::pluginFormat mFormat;
		String mExtension;
};


//
// TODO BUG! plugin will crash if Live is quit while modal window is up.
//
void MLPluginController::convertPresets() 
{
//debug() << "converting presets...\n";

	Array<File> filesToConvert;
	getPresetsToConvert(&filesToConvert);
	int numFiles = filesToConvert.size();
	String fromFileType, toFileType, fromPluginType, toPluginType;
	switch(mWrapperFormat)
	{
		case MLPluginFormats::eVSTPlugin:
			fromPluginType = "AU";
			toPluginType = "VST";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case MLPluginFormats::eAUPlugin:
			fromPluginType = "VST";
			toPluginType = "AU";
			fromFileType = ".mlpreset";
			toFileType = ".aupreset";
		break;
		default:
			fromPluginType = "undefined!";
			toPluginType = "undefined!";
			fromFileType = ".undefined";
			toFileType = ".undefined";
		break;
	}			

	if (numFiles > 0)
	{
		// prompt to convert files
		String noticeStr;
		String numberStr(numFiles);
		String filesStr;
		filesStr = (numFiles > 1) ? " preset files were " : " preset file was ";		
		noticeStr = String(JucePlugin_Name) + " " + toPluginType + ": " + String(filesToConvert.size()) + filesStr + 
			"found in other formats.";
		noticeStr += " Convert to " + toFileType + " format for " + toPluginType + " ?";
				
		bool userPickedOk
			= AlertWindow::showOkCancelBox (AlertWindow::NoIcon,
			String::empty,
			noticeStr,
			"OK",
			"Cancel");
			
		if (userPickedOk)
		{
			PresetConverterThread demoThread(filesToConvert, getProcessor(), mWrapperFormat);

			if (demoThread.runThread())
			{
				// thread finished normally..
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty, "Presets converted ok.", "OK");
			}
			else
			{
				// user pressed the cancel button..
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty, "Convert cancelled.  Some presets were not converted.",
					"OK");
			}
		}
	}
	else
	{
		AlertWindow::showMessageBox (AlertWindow::NoIcon,
			String::empty,
			"No presets found to convert to " + toPluginType + " format.",
			"OK");
	}
}


#endif // ML_MAC

