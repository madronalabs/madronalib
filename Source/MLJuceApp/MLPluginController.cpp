
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

MLPluginController::MLPluginController(MLPluginProcessor* const pProcessor) :
	MLResponder(),
	MLReporter(pProcessor),
	MLSignalReporter(pProcessor),
	mpView(nullptr),
    mpProcessor(pProcessor)
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
	
	mMIDIProgramFiles.resize(kMLPluginMIDIPrograms);
	
	// initialize reference
	WeakReference<MLPluginController> initWeakReference = this;
}

MLPluginController::~MLPluginController()
{
	masterReference.clear();
}

MLAppView* MLPluginController::getView() 
{ 
	return mpView; 
}

void MLPluginController::setView(MLAppView* v) 
{ 
	// if view is going away, stop reporting parameters and signals
	// const ScopedLock lock(mViewShutdownLock);
	if(!v)
	{
		// debug() << "MLPluginController::setView 0\n";
	}
	mpView = v;
}

// setup info pertaining to the plugin format we are controlling
//
void MLPluginController::initialize()
{
    AudioProcessor::WrapperType w = getProcessor()->wrapperType;
    
	std::string regStr, bitsStr, pluginType;
	switch(w)
	{
		case AudioProcessor::wrapperType_VST:
			pluginType = "VST";
		break;
		case AudioProcessor::wrapperType_AudioUnit:
			pluginType = "AU";
		break;
		case AudioProcessor::wrapperType_Standalone:
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
	mVersionString += (std::string(MLProjectInfo::versionString));
	mVersionString += " (" + pluginType + bitsStr + ")";
	
	regStr = mVersionString;
	#if DEMO
		regStr += " DEMO\n";
	#else
		regStr += ", licensed to:\n";
	#endif
	
	MLAppView* myView = getView();
    if(myView)
    {
        MLLabel* regLabel = static_cast<MLLabel*>(myView->getWidget("reg"));
        if(regLabel)
        {
            regLabel->setStringAttribute(MLSymbol("text"), regStr);
        }
    }
}

// --------------------------------------------------------------------------------
#pragma mark MLButton::Listener

void MLPluginController::buttonClicked (MLButton* button)
{
	const MLSymbol paramName = button->getParamName();
	MLPluginProcessor* const filter = getProcessor();
 	const int tri = button->getAttribute("tri_button");
    float val;
    if(tri)
    {        
        val = button->getAttribute("value");
    }
    else
    {
        // TODO simplify/merge on / off states and 3-way button concepts
        const bool state = button->getToggleState();
        val = state ? button->getOnValue() : button->getOffValue();
    }
    if (filter)
    {
        int idx = filter->getParameterIndex(paramName);
        if (idx >= 0)
        {
            filter->MLSetParameterNotifyingHost(idx, val);
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

/*
void MLPluginController::multiSliderDragStarted (MLMultiSlider* pSlider, int idx)
{
	MLPluginProcessor* const filter = getProcessor();
	if (!filter) return;
	const MLSymbol paramName = pSlider->getParamName();
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
	int paramIdx = filter->getParameterIndex(paramName);
	if (paramIdx >= 0)
	{
		filter->endParameterChangeGesture (paramIdx);
	}
}
*/

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

// called to build the scale menu when the Processor's collection of sample files has changed.
void MLPluginController::scaleFilesChanged(const MLFileCollectionPtr fileCollection)
{
    populateScaleMenu(fileCollection);
}

void MLPluginController::presetFilesChanged(const MLFileCollectionPtr fileCollection)
{
    populatePresetMenu(fileCollection);
}

// --------------------------------------------------------------------------------
#pragma mark presets

void MLPluginController::prevPreset()
{
    mpProcessor->prevPreset();
}

void MLPluginController::nextPreset()
{
    mpProcessor->nextPreset();
}

// --------------------------------------------------------------------------------
#pragma mark menus

static void menuItemChosenCallback (int result, WeakReference<MLPluginController> pC, MLSymbol menuName);

MLMenu* MLPluginController::findMenuByName(MLSymbol menuName)	
{
	MLMenu* r = nullptr;
	MLMenuMapT::iterator menuIter(mMenuMap.find(menuName));		
	if (menuIter != mMenuMap.end())
	{
		MLMenuPtr menuPtr = menuIter->second;
		r = menuPtr.get();
	}	
	return r;
}

// set the menu map entry for the given name to a new, empty menu.
MLMenu* MLPluginController::createMenu(MLSymbol menuName)
{
	mMenuMap[menuName] = MLMenuPtr(new MLMenu(menuName));
	return findMenuByName(menuName);
}

void MLPluginController::showMenu (MLSymbol menuName, MLSymbol instigatorName)
{	
	if(!mpView) return;
	
	MLMenu* menu = findMenuByName(menuName);
	if (menu != nullptr)
	{
		menu->setInstigator(instigatorName);

		// find instigator widget and show menu beside it
		MLWidget* pInstigator = mpView->getWidget(instigatorName);
		if(pInstigator != nullptr)
		{
			Component* pInstComp = pInstigator->getComponent();
			if(pInstComp)
			{
                const int u = pInstigator->getWidgetGridUnitSize();
                int height = ((float)u)*0.35f;
                height = clamp(height, 12, 128);
				JuceMenuPtr juceMenu = menu->getJuceMenu();
				juceMenu->showMenuAsync (PopupMenu::Options().withTargetComponent(pInstComp).withStandardItemHeight(height),
					ModalCallbackFunction::withParam(menuItemChosenCallback, 
                    WeakReference<MLPluginController>(this),menuName)
                );
			}
		}
	}
}

void MLPluginController::doPresetMenu(int result)
{
    switch(result)
	{
		// do another menu command
		case (0):	// dismiss
		break;
		case (1):	// save as version in current dir
			if(getProcessor()->saveStateAsVersion() != MLProc::OK) 
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
            // update menu (overkill)
			getProcessor()->scanPresets();
		break;
            
		case (2):	// save over previous
			if(getProcessor()->saveStateOverPrevious() != MLProc::OK)
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
			getProcessor()->scanPresets();
		break;
            
		case (3):	// save as ...
		{
            int err = 0;
            String errStr;
            File userPresetsFolder = getDefaultFileLocation(kPresetFiles);
            if (userPresetsFolder != File::nonexistent)
            {
                bool nativeChooserUI = true;
                FileChooser fc ("Save preset as...", userPresetsFolder, String::empty, nativeChooserUI);
                if (fc.browseForFileToSave (true))
                {
                    File saveFile = fc.getResult();	
                    err = getProcessor()->saveStateToFullPath(std::string(saveFile.getFullPathName().toUTF8()));
                    if(err)
                    {
                        errStr = "Please choose a location in the ";
                        errStr += MLProjectInfo::projectName;
                        errStr += " folder.";
                    }
                }
            }
            else
            {
                err = 1;
                errStr = ("Presets folder ");
                errStr += userPresetsFolder.getFullPathName();
                errStr += " not found!";
				debug() << "MLPluginController::doPresetMenu: " << errStr << "\n";
            }
            
            if(err)
            {                
                AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
            }
			getProcessor()->scanPresets();
		}
		break;
		case (4):	// revert
			getProcessor()->returnToLatestStateLoaded();
		break;

		case (5):	// copy
			SystemClipboard::copyTextToClipboard (getProcessor()->getStateAsText());
		break;
		case (6):	// paste
			getProcessor()->setStateFromText (SystemClipboard::getTextFromClipboard());
		break;

#if ML_MAC
		case (7):	// show convert alert box
			convertPresets();
			getProcessor()->scanPresets();
		break;
#endif
        default:    // load preset
            MLMenu* menu = findMenuByName("preset");
            if (menu)
            {
                const std::string& fullName = menu->getItemFullName(result);                
                getProcessor()->loadStateFromPath(fullName);
            }
            break;
	}
}

void MLPluginController::doScaleMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):	
            mpProcessor->setModelParam("key_scale", "12-equal");
            break;
        default:
            MLMenu* menu = findMenuByName("key_scale");
            if (menu)
            {
                // set model param to the full name of the file in the menu
                const std::string& fullName = menu->getItemFullName(result);
                mpProcessor->setModelParam("key_scale", fullName);
            }
            break;
    }
}
	
static void menuItemChosenCallback (int result, WeakReference<MLPluginController> wpC, MLSymbol menuName)
{
	MLPluginController* pC = wpC;
	
	// get Controller ptr from weak reference
	if(pC == nullptr)
	{
		debug() << "    null MLPluginController ref!\n";
		return;
	}
	
	if(pC != nullptr)
	{	
		//debug() << "    MLPluginController:" << std::hex << (void *)pC << std::dec << "\n";

		// get menu by name from Controller’s menu map		
		const MLMenu* pMenu = pC->findMenuByName(menuName);
		if (pMenu == nullptr)
		{
			debug() << "    MLPluginController::populatePresetMenu(): menu not found!\n";
		}	
		else
		{		
			MLWidgetContainer* pView = pC->getView();
			
			//debug() << "    pView:" << std::hex << (void *)pView << std::dec << "\n";
			if(pView != nullptr)
			{	
				//debug() << "        pView widget name:" << pView->getWidgetName() << "\n";
				
				MLWidget* pInstigator = pView->getWidget(pMenu->getInstigator());
				if(pInstigator != nullptr)
				{
					// turn instigator Widget off
					pInstigator->setAttribute("value", 0);
				}
			}
			
			pC->menuItemChosen(menuName, result);
		}
	}
    
    debug() << "----------\n";
}

void MLPluginController::menuItemChosen(MLSymbol menuName, int result)
{
	if (result > 0)
	{
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

void MLPluginController::populatePresetMenu(const MLFileCollectionPtr presetFiles)
{
	MLMenu* menu = createMenu("preset");
	if (menu == nullptr)
	{
		MLError() << "MLPluginController::populatePresetMenu(): menu not found!\n";
		return;
	}			
	menu->clear();
	
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
	menu->addItem("Convert presets...");
#endif
	menu->addSeparator();
    
    // add factory presets, those starting with the plugin name    
    MLMenuPtr factoryMenu(new MLMenu(presetFiles->getName()));
    presetFiles->getRoot()->buildMenuIncludingPrefix(factoryMenu, MLProjectInfo::projectName);
    menu->appendMenu(factoryMenu);
    
    menu->addSeparator();
    
    // add user presets, all the others
    MLMenuPtr userMenu(new MLMenu(presetFiles->getName()));
    presetFiles->getRoot()->buildMenuExcludingPrefix(userMenu, MLProjectInfo::projectName);
    menu->appendMenu(userMenu);
    
    menu->buildIndex();
     
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

// create a menu of the factory scales.
//
void MLPluginController::populateScaleMenu(const MLFileCollectionPtr fileCollection)
{
    MLMenu* pMenu = createMenu("key_scale");
	pMenu->clear();
 	pMenu->addItem("12-equal");
    MLMenuPtr p = fileCollection->buildMenu();
    pMenu->appendMenu(p);
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
	switch(mpProcessor->wrapperType)
	{
		case AudioProcessor::wrapperType_RTAS:
			pluginType = "RTAS";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case AudioProcessor::wrapperType_VST:
			pluginType = "VST";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case AudioProcessor::wrapperType_AudioUnit:
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
	
    // get lists of files but don't add to menus
    File presetsFolder = getDefaultFileLocation(kPresetFiles);
    if (presetsFolder != File::nonexistent)
	{
        // TODO replace
    //    findFilesOneLevelDeep(presetsFolder, fromFileType, fromFiles, 0);
    }
     
	debug() << "convertPresets: got " << fromFiles.size() << " preset files of other type.\n";

	// for each fromType file, look to see if it has a toType counterpart. 
	// if not, add it to list.
	for(int i=0; i<fromFiles.size(); i++)
	{
		debug() << fromFiles[i].getFileNameWithoutExtension() << " : ";
		
		if (!(fromFiles[i].withFileExtension(toFileType).exists()))
		{
		debug() << " NOT in our format\n";		
			pResults->add(fromFiles[i]);
		}
		else
		{
		debug() << " IS in our format\n";
		}
	}

	debug() << "got " << pResults->size() << " preset files to convert.\n";
} 


// Convert all the preset files in the array parameter to the format of the current
// plugin version.  Just loads each preset into the filter and saves it out again. 
//
class PresetConverterThread  : public ThreadWithProgressWindow
{
public:
    PresetConverterThread(Array<File> &pFilesToConvert, MLPluginProcessor* filter, AudioProcessor::WrapperType format)
        : ThreadWithProgressWindow (String::empty, true, true),
		mpFiles(pFilesToConvert),
		mpFilter(filter),
		mFormat(format)
    {
        setStatusMessage ("Getting ready...");
		switch(mFormat)
		{
			case AudioProcessor::wrapperType_VST:
				mExtension = ".mlpreset";
			break;
			case AudioProcessor::wrapperType_AudioUnit:
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
                    if(xml)
                    {
                        xml->writeToFile(toFile, String::empty);
                        wait(10);
                    }
				}
				else if (mExtension == ".aupreset")
				{
					mpFilter->loadStateFromFile(fromFile);
					wait(100);
                    mpFilter->saveStateToFullPath(std::string(toFile.getFullPathName().toUTF8()));
					wait(100);
				}
			}
					
            setProgress (i / (double) numFiles);
        }
    }
	
	void timerCallback(){}
	
	private:
		Array<File> &mpFiles;
		MLPluginProcessor* mpFilter;
		String mExtension;
        AudioProcessor::WrapperType mFormat;
};

void MLPluginController::convertPresets()
{
debug() << "converting presets...\n";

	Array<File> filesToConvert;
	getPresetsToConvert(&filesToConvert);
	int numFiles = filesToConvert.size();
	String fromFileType, toFileType, fromPluginType, toPluginType;
	switch(mpProcessor->wrapperType)
	{
		case AudioProcessor::wrapperType_VST:
			fromPluginType = "AU";
			toPluginType = "VST";
			fromFileType = ".aupreset";
			toFileType = ".mlpreset";
		break;
		case AudioProcessor::wrapperType_AudioUnit:
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
			PresetConverterThread demoThread(filesToConvert, getProcessor(), mpProcessor->wrapperType);

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

