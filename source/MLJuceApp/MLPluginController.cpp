
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

const int kControllerTimerRate = 33;

MLPluginController::MLPluginController(MLPluginProcessor* pProcessor) :
	MLWidget::Listener(),
	MLReporter(),
	MLSignalReporter(pProcessor),
	mpView(nullptr),
	mpProcessor(pProcessor),
	mClockDivider(0),
	mConvertingPresets(false),
	mFilesConverted(0),
	mProtocolMenuItemStart(0),
	mOSCMenuItemStart(0)
{
	// initialize reference
	WeakReference<MLPluginController> initWeakReference = this;
	
	createMenu("key_scale");
	createMenu("preset");
	createMenu("settings");

	listenTo(pProcessor);
	listenTo(pProcessor->getEnvironment());
#if ML_MAC
	mFileActionData.resize(0);
	PaUtil_InitializeRingBuffer( &mFileActionQueue, sizeof(FileAction), 0, &(mFileActionData[0]) );
#endif
}

MLPluginController::~MLPluginController()
{
	stopTimer();
	masterReference.clear();
}

MLAppView* MLPluginController::getView() 
{ 
	return mpView; 
}

void MLPluginController::setView(MLAppView* v) 
{ 
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
            regLabel->setPropertyImmediate(MLSymbol("text"), regStr);
        }
    }
	startTimer(kControllerTimerRate);
}

void MLPluginController::timerCallback()
{
    const int lessFrequentThingsDivision = 8;
    mClockDivider++;
	fetchChangedProperties();
    
	if(mClockDivider > lessFrequentThingsDivision)
    {
        // do less frequent things (unused)
        mClockDivider = 0;
    }
	
	if(getView())
		viewSignals();
	
#if ML_MAC
	// read from file action queue and do any needed actions
	if(mConvertingPresets)
	{
		FileAction a;
		int filesRead = 0;
		
		if((filesRead = PaUtil_ReadRingBuffer( &mFileActionQueue, &a, 1 )))
		{
			if(a.mFile.exists())
			{
				doFileQueueAction(a);
			}
			int filesInQueue = PaUtil_GetRingBufferReadAvailable(&mFileActionQueue);
			mMaxFileQueueSize = max(filesInQueue, mMaxFileQueueSize);
			if(!filesInQueue) endConvertPresets();
		}
	}
#endif
}

void MLPluginController::handleWidgetAction(MLWidget* pw, MLSymbol action, MLSymbol targetProperty, const MLProperty& val)
{
	if(action == "click")
	{
		if (targetProperty == "prev")
		{
			getProcessor()->prevPreset();
		}
		else if (targetProperty == "next")
		{
			getProcessor()->nextPreset();
		}
		// TODO make this happen like other menus
		else if (targetProperty == "settings")
		{
			showMenu("settings", "settings");
		}
	}
	else if(action == "begin_gesture")
	{
		int idx = mpProcessor->getParameterIndex(targetProperty);
		if (idx > 0)
		{
			mpProcessor->beginParameterChangeGesture (idx);
		}
	}
	else if(action == "change_property")
	{
		mpProcessor->setPropertyImmediateExcludingListener(targetProperty, val, pw);
	}
	else if (action == "end_gesture")
	{
		int idx = mpProcessor->getParameterIndex(targetProperty);
		if (idx > 0)
		{
			mpProcessor->endParameterChangeGesture (idx);
		}
	}
	else if(action == "show_menu")
	{
		// give subclasses a chance to rebuild menus
		updateMenu(targetProperty);
		showMenu(targetProperty, pw->getWidgetName());
	}
}

#pragma mark menus

static void menuItemChosenCallback (int result, WeakReference<MLPluginController> pC, MLSymbol menuName);

// set the menu map entry for the given name to a new, empty menu.
MLMenu* MLPluginController::createMenu(MLSymbol menuName)
{
	mMenuMap[menuName] = MLMenuPtr(new MLMenu(menuName));
	return findMenuByName(menuName);
}

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

void MLPluginController::buildMenuFromSymbolVector(MLSymbol menuName, std::vector<std::string> & v)
{
	if(MLMenu* pMenu = createMenu(menuName))
	{
		pMenu->clear();
		for(auto itemString : v)
		{
			pMenu->addItem(itemString);
		}
	}
}

void MLPluginController::showMenu (MLSymbol menuName, MLSymbol instigatorName)
{	
	if(!mpView) return;
	
	if(menuName == "key_scale")
	{
		populateScaleMenu(getProcessor()->getScaleCollection());
	}
	else if(menuName == "preset")
	{
		getProcessor()->searchForPresets();
		const MLFileCollection& presets = getProcessor()->getPresetCollection();
		
		populatePresetMenu(presets);
		flagMIDIProgramsInPresetMenu();
	}
	else if(menuName == "settings")
	{
		populateSettingsMenu();
	}
	
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
		break;
            
		case (2):	// save over previous
			if(getProcessor()->saveStateOverPrevious() != MLProc::OK)
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
		break;
            
		case (3):	// save as ...
		{
            String errStr;
            File userPresetsFolder = getDefaultFileLocation(kPresetFiles);
            if (userPresetsFolder != File::nonexistent)
            {
                bool nativeChooserUI = true;
                FileChooser fc ("Save preset as...", userPresetsFolder, String::empty, nativeChooserUI);
                if (fc.browseForFileToSave (true))
                {
                    File saveFile = fc.getResult();
					std::string fullSavePath(saveFile.getFullPathName().toUTF8());
                    getProcessor()->saveStateToLongFileName(fullSavePath);
                }
            }
            else
            {
                errStr = ("Error: user presets folder did not exist and could not be created.");
                AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
            }
		}
		break;
		case (4):	// revert
			getProcessor()->returnToLatestStateLoaded();
		break;

		case (5):	// copy
			SystemClipboard::copyTextToClipboard (getProcessor()->getStateAsText());
		break;
		case (6):	// paste
			getProcessor()->setPatchStateFromText (SystemClipboard::getTextFromClipboard());
		break;

#if SHOW_CONVERT_PRESETS
#if ML_MAC
		case (7):	// show convert alert box
			convertPresets();
			getProcessor()->scanAllFilesImmediate();
		break;
#endif
#endif
        default:    // load preset
            loadPresetByMenuIndex(result);
            break;
	}
}

void MLPluginController::loadPresetByMenuIndex(int result)
{
	MLMenu* menu = findMenuByName("preset");
	if(menu)
	{
		const std::string& fullName = menu->getMenuItemPath(result);
		getProcessor()->loadStateFromPath(fullName);
	}
	MLReporter::fetchChangedProperties();
}

void MLPluginController::doScaleMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):
            mpProcessor->setPropertyImmediate("key_scale", "12-equal");
            break;
        default:
            MLMenu* menu = findMenuByName("key_scale");
            if (menu)
            {
                // set model param to the full name of the file in the menu
                const std::string& fullName = menu->getMenuItemPath(result);
                mpProcessor->setPropertyImmediate("key_scale", fullName);
            }
            break;
    }
}

// TODO menus should respond to symbols when possible, not numbers!!
// menu should return MLPath to item chosen.
void MLPluginController::doSettingsMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):
		{
			bool enabled = mpProcessor->getEnvironment()->getFloatProperty("editor_num");
			mpProcessor->getEnvironment()->setPropertyImmediate("editor_num", !enabled);
			break;
		}
        case (2):
		{
			bool enabled = mpProcessor->getEnvironment()->getFloatProperty("editor_anim");
			mpProcessor->getEnvironment()->setPropertyImmediate("editor_anim", !enabled);
			break;
		}
        case (3):
		{
			// set editor to default size
			MLPoint p = mpProcessor->getDefaultEditorSize();
			mpProcessor->editorResized(p.x(), p.y());
			break;
		}
		default:
		{
			if(result <= mOSCMenuItemStart)
			{
				mpProcessor->getEnvironment()->setPropertyImmediate("protocol", result - mProtocolMenuItemStart - 1);
			}
			else
			{
				// other items set osc port offset.
				mpProcessor->getEnvironment()->setPropertyImmediate("osc_port_offset", result - mOSCMenuItemStart - 1);
			}
		}
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

	//debug() << "    MLPluginController:" << std::hex << (void *)pC << std::dec << "\n";

	// get menu by name from Controller’s menu map		
	const MLMenu* pMenu = pC->findMenuByName(menuName);
	if (pMenu)
	{
		MLWidgetContainer* pView = pC->getView();
		
		//debug() << "    pView:" << std::hex << (void *)pView << std::dec << "\n";
		if(pView != nullptr)
		{	
			//debug() << "        pView widget name:" << pView->getWidgetName() << "\n";
			
			MLWidget* pInstigator = pView->getWidget(pMenu->getInstigator());
			if(pInstigator != nullptr)
			{
				// turn instigator Widget off (typically, release button)
				pInstigator->setPropertyImmediate("value", 0);
			}
		}
		
		pC->menuItemChosen(menuName, result);
	}
}

void MLPluginController::updateMenu(MLSymbol menuName)
{
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
			else if(menuName == "settings")
			{
				doSettingsMenu(result);
			}
		}
	}
}

// create a menu of the factory scales.
//
void MLPluginController::populateScaleMenu(const MLFileCollection& fileCollection)
{
    MLMenu* pMenu = findMenuByName("key_scale");
	pMenu->clear();
 	pMenu->addItem("12-equal");
    pMenu->appendMenu(fileCollection.buildMenu());
}

void MLPluginController::populatePresetMenu(const MLFileCollection& presetFiles)
{
	MLMenu* menu = findMenuByName("preset");
	if (menu == nullptr)
	{
		debug() << "MLPluginController::populatePresetMenu(): menu not found!\n";
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
	
#if SHOW_CONVERT_PRESETS
#if ML_MAC
	menu->addItem("Convert presets...");
#endif
#endif
	menu->addSeparator();
	
	// presets and directories starting with the name of the plugin followed by a space
	// will be sorted into a separate factory area.
	std::string prefix = MLProjectInfo::projectName + std::string(" ");
	
    // add factory presets, those starting with the plugin name
    menu->appendMenu(presetFiles.buildMenu
					 ([=](MLResourceMap<std::string, MLFile>::const_iterator it)
					  {
						  if(it.getDepth() > 0)
						  {
							  return true;
						  }
						  else
						  {
							  std::string fileName = it->getValue().getShortName();					 
							  return (prefix.compare(fileName.substr(0, prefix.length())) == 0);
						  };
					  }
					  )
					 );	
	menu->addSeparator();
    
    // add user presets, meaning all the others, but no "Samples"
	menu->appendMenu(presetFiles.buildMenu
					 ([=](MLResourceMap<std::string, MLFile>::const_iterator it)
					  {
						  if(it.getDepth() > 0)
						  {
							  return true;
						  }
						  else
						  {
							  std::string fileName = it->getValue().getShortName();		
							  if(fileName.compare("Samples") == 0) return false;
							  return (prefix.compare(fileName.substr(0, prefix.length())) != 0);
						  };
					  }
					  )
					 );
	menu->buildIndex();
}

// "gear" menu for environment settings.
//
void MLPluginController::populateSettingsMenu()
{
	MLMenu* pMenu = findMenuByName("settings");
	pMenu->clear();
	
	bool num = (bool)mpProcessor->getEnvironment()->getFloatProperty("editor_num");
	bool anim = (bool)mpProcessor->getEnvironment()->getFloatProperty("editor_anim");
	pMenu->addItem("Show numbers", true, num);
	pMenu->addItem("Animate dials", true, anim);
	pMenu->addItem("Reset editor size");

	// protocols
	const int currProtocol = mpProcessor->getEnvironment()->getFloatProperty("protocol");
	MLMenuPtr protocolMenu(new MLMenu());
	{
		// Mac OS currently has MIDI and MPE and OSC. Windows just MIDI and MPE.
#if ML_MAC		
		const std::vector<std::string> names ({"MIDI", "MIDI MPE", "OSC"});
#else
		const std::vector<std::string> names ({"MIDI", "MIDI MPE"});
#endif
		for(int i=0; i<names.size(); ++i)
		{
			bool ticked = (i == currProtocol);
			protocolMenu->addItem(names[i], true, ticked);
		}
	}
	
	pMenu->addSeparator();
	mProtocolMenuItemStart = pMenu->getSize();
	pMenu->addSubMenu(protocolMenu, "Input protocol");
	mOSCMenuItemStart = pMenu->getSize();

#if ML_MAC
	MLMenuPtr portsMenu(new MLMenu());
	for(int i=0; i<16; ++i)
	{
		int currPort = mpProcessor->getEnvironment()->getFloatProperty("osc_port_offset");
		
		bool ticked = (i == currPort);
		std::ostringstream s;
		s << i;
		const std::string iStr(s.str());
		portsMenu->addItem(iStr, true, ticked);
	}
	pMenu->addSubMenu(portsMenu, "OSC port offset");
#endif
}

void MLPluginController::flagMIDIProgramsInPresetMenu()
{
	MLMenu* pMenu = findMenuByName("preset");
	if(pMenu != nullptr)
	{
		MLMenu::NodePtr node = pMenu->getItem("MIDI Programs");
		if(node.get() != nullptr)
		{
			if(node->getNodeSize(0) > 0)
			{
				std::list<std::string>::const_iterator it;
				const std::list<std::string>& nodeIndex = node->getIndex();
				
				int pgm = 0;
				for(it = nodeIndex.begin(); ((it != nodeIndex.end()) && (pgm <= 127)); it++)
				{
					const std::string& name = *it;
					MLMenu::NodePtr subNode = node->getSubnodeByName(name);
					{
						std::ostringstream s;
						s << pgm++;
						const std::string pStr(s.str());
						subNode->setDisplayPrefix(std::string("[") + pStr + std::string("] "));
					}
				}
			}
		}
	}
}

#pragma mark MLFileCollection::Listener

void MLPluginController::processFileFromCollection (MLSymbol action, const MLFile fileToProcess, const MLFileCollection& collection, int idx, int size)
{
	MLSymbol collectionName(collection.getName());
	if(action == "process")
	{
		if(collectionName.beginsWith(MLSymbol("convert_presets")))
		{			
			// add file action to queue
			FileAction f(action, fileToProcess, &collection, idx, size);
			PaUtil_WriteRingBuffer( &mFileActionQueue, &f, 1 );
		}
	}
}

void MLPluginController::clearFileActionQueue()
{
	int remaining = PaUtil_GetRingBufferReadAvailable(&mFileActionQueue);
	PaUtil_AdvanceRingBufferReadIndex(&mFileActionQueue, remaining);
}

void MLPluginController::doFileQueueAction(FileAction a)
{
	File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
	File destRoot(newPresetsFolder);
	
	// get name relative to collection root.
	const std::string& relativeName = a.mCollection->getRelativePathFromName(a.mFile.getLongName());
	
	// If file at destination does not exist, or is older than the source, convert
	// source and overwrite destination.
	File destFile = destRoot.getChildFile(String(relativeName)).withFileExtension("mlpreset");
	bool destinationExists = destFile.exists();
	bool destinationIsOlder =  destFile.getLastModificationTime() < a.mFile.getJuceFile().getLastModificationTime();
	if((!destinationExists) || (destinationIsOlder))
	{
		mpProcessor->loadPatchStateFromFile(a.mFile);
		mpProcessor->saveStateToRelativePath(relativeName);
		mFilesConverted++;
	}

	mFilesProcessed++;
}
			
#if ML_MAC

#pragma mark ConvertProgressDisplayThread

// ConvertProgressDisplayThread: progress display for preset converter.
// TODO write on our own base class to replace ThreadWithProgressWindow.

void MLPluginController::ConvertProgressDisplayThread::run()
{
	std::string rootStr("Converting .aupreset and .mlpreset files from /Library and ~/Library...");
	setProgress(-1.0);
	setStatusMessage (rootStr);
	
	while((myProgress < 1.0) && !threadShouldExit())
	{
		wait(50);
		myProgress = pController->getConvertProgress();
		setProgress(myProgress);
		mFilesConverted = pController->getFilesConverted();
	}
}

// This method gets called from the message thread to end our thread.
void MLPluginController::ConvertProgressDisplayThread::threadComplete (bool userPressedCancel)
{
	File destDir = getDefaultFileLocation(kPresetFiles);
	String destDirName = destDir.getFullPathName();
	if (userPressedCancel)
	{
		pController->endConvertPresets();
		AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets was cancelled.",
			"Some presets may not have been converted.");
	}
	else
	{
		if(mFilesConverted)
		{
			AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets successful.",
				String(mFilesConverted) + String(" presets were added to ") + destDirName + ".");
		}
		else
		{
			AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "No presets found to convert.", "");
		}
	}
	delete this;
}

// only convert .aupreset (AU) to .mlpreset (VST) now. After Aalto 1.6 / Kaivo 1.2 there will be no need to convert presets.
void MLPluginController::convertPresets()
{
    if(!mpProcessor) return;
	
	mMaxFileQueueSize = 0;
	mFilesProcessed = 0;
	mFilesConverted = 0;
	
    File presetsFolder = getDefaultFileLocation(kOldPresetFiles);
    if (presetsFolder != File::nonexistent)
    {
		// turn off audio -- will be turned back on by finish or cancel
        mpProcessor->suspendProcessing(true);
		
		// clear presets collection
		mpProcessor->clearPresetCollection();

		// clear menu
		findMenuByName("preset")->clear();
		
		mPresetsToConvertAU1 = std::auto_ptr<MLFileCollection>(new MLFileCollection("convert_presets_au1", getDefaultFileLocation(kOldPresetFiles), ".aupreset"));
		mPresetsToConvertAU1->addListener(this);
		mPresetsToConvertAU2 = std::auto_ptr<MLFileCollection>(new MLFileCollection("convert_presets_au2", getDefaultFileLocation(kOldPresetFiles2), ".aupreset"));
		mPresetsToConvertAU2->addListener(this);
		mPresetsToConvertVST1 = std::auto_ptr<MLFileCollection>(new MLFileCollection("convert_presets_vst1", getDefaultFileLocation(kOldPresetFiles), ".mlpreset"));
		mPresetsToConvertVST1->addListener(this);
		mPresetsToConvertVST2 = std::auto_ptr<MLFileCollection>(new MLFileCollection("convert_presets_vst2", getDefaultFileLocation(kOldPresetFiles2), ".mlpreset"));
		mPresetsToConvertVST2->addListener(this);
		
		mFilesToProcess = 0;
		mFilesToProcess += mPresetsToConvertAU1->searchForFilesImmediate();
		mFilesToProcess += mPresetsToConvertAU2->searchForFilesImmediate();
		mFilesToProcess += mPresetsToConvertVST1->searchForFilesImmediate();
		mFilesToProcess += mPresetsToConvertVST2->searchForFilesImmediate();
		
		int fileBufferSize = 1 << bitsToContain(mFilesToProcess);
		mFileActionData.resize(fileBufferSize);
		PaUtil_InitializeRingBuffer( &mFileActionQueue, sizeof(FileAction), fileBufferSize, &(mFileActionData[0]) );
		
		// convert files in immediate mode and wait for finish.
		int interFileDelay = 0;
		mPresetsToConvertAU1->processFilesImmediate(interFileDelay);
		mPresetsToConvertAU2->processFilesImmediate(interFileDelay);
		mPresetsToConvertVST1->processFilesImmediate(interFileDelay);
		mPresetsToConvertVST2->processFilesImmediate(interFileDelay);
		mConvertingPresets = true;
		
		(new ConvertProgressDisplayThread(this))->launchThread();
		
//		startTimer(5); // speed up action for convert
    }
    else
    {
        debug() << "convertPresets: couldn't find preset folder " << presetsFolder.getFullPathName() << ".\n";
    }
}

float MLPluginController::getConvertProgress()
{
	int remaining = PaUtil_GetRingBufferReadAvailable(&mFileActionQueue);
	float p;
	if(!mConvertingPresets)
	{
		p = 1.f;
	}
	else if(remaining > 0.f)
	{
		if(mMaxFileQueueSize > 0)
		{
			p = (float)(mMaxFileQueueSize - remaining) / (float)(mMaxFileQueueSize);
		}
		else 
		{
			p = 0.f;
		}
	}
	else
	{
		p = 1.f;
	}
	return p;
}

int MLPluginController::getFilesConverted() const
{
	return mFilesConverted;
}

void MLPluginController::endConvertPresets()
{
	mConvertingPresets = false;
	clearFileActionQueue();
	
	startTimer(kControllerTimerRate);
	
	// resume processing
	mpProcessor->suspendProcessing(false);
}


#endif // ML_MAC

