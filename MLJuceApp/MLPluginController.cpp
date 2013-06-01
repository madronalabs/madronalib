
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
	mCurrentPresetIndex(0),
	mCurrMenuInstigator(nullptr)
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
		debug() << "MLPluginEditor: couldn't get data files!\n";
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
		return mMenuItemStrings[idx]; 
	}
	return String();
}

void MLPluginController::loadPresetByIndex (int idx)
{
	MLPluginProcessor* const filter = getProcessor();
	mCurrentPresetIndex = idx;
	
	// pause timer
	//stopTimer();
	
	debug() << mMenuPresetFiles.size() << " presets\n";
	debug() << "loading preset: " << mCurrentPresetIndex << ", " << mMenuPresetFiles[mCurrentPresetIndex].getFileNameWithoutExtension() << "\n";		
	filter->loadStateFromFile(mMenuPresetFiles[mCurrentPresetIndex]);
	
	mCurrentPresetFolder = mMenuPresetFiles[mCurrentPresetIndex].getParentDirectory();
	
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

static void menuItemChosenCallback (int result, MLPluginController* pC);

void MLPluginController::showMenu (MLSymbol menuName, MLMenuButton* instigator)
{
	StringArray devices;
	
	// handle possible click on second menu while first is active
	if(getCurrMenuInstigator() != nullptr)
	{
		getCurrMenuInstigator()->setToggleState(false, false);
	}
	
	mCurrMenuName = menuName;
	assert(instigator);
	setCurrMenuInstigator(instigator);
	instigator->setToggleState(true, false);
	
	const int u = instigator->getWidgetGridUnitSize();
	int height = ((float)u)*0.35f;
	height = clamp(height, 12, 128);
	
	if (menuName == "preset")
	{
		mPresetMenu.showMenuAsync (PopupMenu::Options().withTargetComponent(instigator).withStandardItemHeight(height),
			ModalCallbackFunction::create(menuItemChosenCallback, this));	
	}
	else if (menuName == "key_scale")
	{
		mScaleMenu.showMenuAsync (PopupMenu::Options().withTargetComponent(instigator).withStandardItemHeight(height),
			ModalCallbackFunction::create(menuItemChosenCallback, this));
	}
	
	/*
	else if (menuName == "midi_device")
	{
		// refresh device list
		SoundplaneMIDIOutput& outs = getModel()->getMIDIOutput();
		outs.findMIDIDevices();
		
		int c = 0;
		mMIDIMenu.clear();
		
		int s = outs.getNumDevices();
		for(int i=0; i<s; ++i)
		{
			const std::string& dName = outs.getDeviceName(i);
			mMIDIMenu.addItem(++c, String(dName.c_str()));
		}
	
		mMIDIMenu.showMenuAsync (PopupMenu::Options().withTargetComponent(instigator).withStandardItemHeight(16),
			ModalCallbackFunction::withParam(menuItemChosenCallback, this, menuName));
	}	
	*/
}

void MLPluginController::doPresetMenu(int result)
{
	String presetStr;
	if (result > mPresetMenuStartItems)
	{
		String resultStr = getPresetString(result);
		loadPresetByIndex(result - mPresetMenuStartItems - 1);
		getCurrMenuInstigator()->setButtonText(resultStr);
	}
	else switch(result)
	{
		case (0):	// dismiss
		default:
		break;
		case (1):	// save over previous
			if(getProcessor()->saveStateAsVersion(mCurrentPresetFolder) != MLProc::OK)  // TODO this should be MLErr
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					getProcessor()->getErrorMessage(),
					"OK",
					getCurrMenuInstigator());
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
					"OK",
					getCurrMenuInstigator());
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
			filter->loadDefaultScale ();
			scaleName = "12-equal";
			mCurrentScaleDir = scaleDir;
		}
		break;
	}
	getCurrMenuInstigator()->setButtonText(scaleName);
}

static void menuItemChosenCallback (int result, MLPluginController* pC)
{
	MLMenuButton* instigator = pC->getCurrMenuInstigator();
	assert(instigator);
	
	// get result string	
	if (result)
	{
		String resultStr; // TODO std::string, ML menu class 
		MLSymbol menuName = pC->getCurrMenuName();

		MLAppView* pV = pC->getView();
		if(pV)
		{
			if (menuName == "preset")
			{
				pC->doPresetMenu(result);
			}	
			else if(menuName == "key_scale")
			{
				pC->doScaleMenu(result);
			}
			
			// notify Model / Processor of change
		}		
	}
	if(instigator)
	{
		instigator->setToggleState(false, false);
	}
}

/*
static void menuItemChosenCallback (int result, SoundplaneController* pC, MLSymbol menuName);
static void menuItemChosenCallback (int result, SoundplaneController* pC, MLSymbol menuName)
{
	MLMenuButton* instigator = pC->getCurrMenuInstigator();
	if(instigator)
	{
		instigator->setToggleState(false, false);
	}
	pC->menuItemChosen(menuName, result);
	
	
}
*/

// get all files in the given directory and its immediate subdirectories that have the 
// given extension. append the Files to results. if the three optional menu params 
// are specified, add menu items to the menus.  
//
//
void MLPluginController::findFilesOneLevelDeep(File& startDir, String extension, Array<File>& results, 
	Array<String>* menuStrings, PopupMenu* pMenu, OwnedArray<PopupMenu>* subMenus)
{
	if (!startDir.isDirectory()) return;
	bool doMenus = (menuStrings && pMenu && subMenus);

	Array<File> startDirArray;
	const int userFilesToFind = File::findFilesAndDirectories | File::ignoreHiddenFiles;
	int userFiles = startDir.findChildFiles(startDirArray, userFilesToFind, false);

	int midiPgmCount = 0; // messy
	for(int i=0; i<userFiles; ++i)
	{
		File f = startDirArray[i];
		if (f.isDirectory())
		{
			// only recurse one level deep.
			String category = f.getFileNameWithoutExtension();
			File subdir = startDir.getChildFile(category);
			bool doMIDI = (category == "MIDI Programs");   // messy
			
			if(subdir.exists())
			{
				PopupMenu* subPop;
				Array<File> subdirArray;
				if (doMenus) 
				{
					subPop = new PopupMenu();
					subMenus->add(subPop);
				}
				
				int filesInCategory = subdir.findChildFiles(subdirArray, userFilesToFind, false);
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
						// debug() << "     " << subPreset << "\n";
						if (doMenus)
						{
							menuStrings->add(subPreset);
							subPop->addItem(menuStrings->size(), menuStrings->getLast());
						}
					}
				}
				if (doMenus) pMenu->addSubMenu(category, *subPop);
			}					
		}
		else if (f.hasFileExtension(extension))
		{
			String preset = f.getFileNameWithoutExtension();
			results.add(f);
			if (doMenus)
			{
				menuStrings->add(preset);
				pMenu->addItem(menuStrings->size(), menuStrings->getLast());
			}
		}
	}
}

void MLPluginController::populatePresetMenu() 
{
	mPresetMenu.clear();
	mPresetSubMenus.clear();
	mMenuItemStrings.clear();
	mMenuPresetFiles.clear();
	mPresetMenuStartItems = 0;
	
	mMenuItemStrings.add("Save as version");
#if DEMO
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast(), false);
#else
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast());
#endif	
	
	mMenuItemStrings.add("Save");
#if DEMO
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast(), false);
#else
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast());
#endif	
	
	mMenuItemStrings.add("Save as...");
#if DEMO
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast(), false); 
#else
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast());
#endif	

	mMenuItemStrings.add("Revert to saved");
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast()); 

	mPresetMenu.addSeparator();		

	mMenuItemStrings.add("Copy to clipboard");
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast());
	
	mMenuItemStrings.add("Paste from clipboard");
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast());
	
#if ML_MAC
	mPresetMenu.addSeparator();		
	mMenuItemStrings.add("Convert presets...");
	mPresetMenu.addItem(mMenuItemStrings.size(), mMenuItemStrings.getLast()); 
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
	mPresetMenuStartItems = mMenuItemStrings.size();
	if (mFileLocationsOK)
	{
		mPresetMenu.addSeparator();		
		findFilesOneLevelDeep(mUserPresetsFolder, presetFileType, mMenuPresetFiles, 
			&mMenuItemStrings, &mPresetMenu, &mPresetSubMenus);
		mPresetMenu.addSeparator(); 				
		findFilesOneLevelDeep(mFactoryPresetsFolder, presetFileType, mMenuPresetFiles, 
			&mMenuItemStrings, &mPresetMenu, &mPresetSubMenus);	
			
		debug() << "MLPluginController: " << mMenuPresetFiles.size() << " preset files.\n";
		
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

	mScaleMenuItemStrings.add("12-equal");
	mScaleMenu.addItem(mScaleMenuItemStrings.size(), mScaleMenuItemStrings.getLast());

	if(mFileLocationsOK) 
	{
		findFilesOneLevelDeep(mScalesFolder, ".scl", mScaleMenuFiles, 
			&mScaleMenuItemStrings, &mScaleMenu, &mScaleSubMenus);
	}
}

void MLPluginController::setupMenus()
{
	populatePresetMenu();
	populateScaleMenu();
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
		findFilesOneLevelDeep(mUserPresetsFolder, fromFileType, fromFiles, 
			0, 0, 0);
		findFilesOneLevelDeep(mFactoryPresetsFolder, fromFileType, fromFiles, 
			0, 0, 0);
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
debug() << "converting presets...\n";

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
			"Cancel",
			getCurrMenuInstigator());
			
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
			"OK",
			getCurrMenuInstigator());
	}
}


#endif // ML_MAC

