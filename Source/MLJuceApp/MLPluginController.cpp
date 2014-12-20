
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

MLPluginController::MLPluginController(MLPluginProcessor* const pProcessor) :
	MLWidget::Listener(),
	MLReporter(),
	MLSignalReporter(pProcessor),
	mpView(nullptr),
    mpProcessor(pProcessor)
{	
	// initialize reference
	WeakReference<MLPluginController> initWeakReference = this;
	
	listenTo(pProcessor);
	listenTo(pProcessor->getEnvironment());
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

void MLPluginController::handleWidgetAction(MLWidget* pw, MLSymbol action, MLSymbol targetProperty, const MLProperty& val)
{
	if(action == "click")
	{
		
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

#pragma mark presets

void MLPluginController::prevPreset()
{
    mpProcessor->prevPreset();
	MLReporter::fetchChangedProperties();
}

void MLPluginController::nextPreset()
{
    mpProcessor->nextPreset();
	MLReporter::fetchChangedProperties();
}

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
                        AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
                    }
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
            MLMenu* menu = findMenuByName("preset");
            if (menu)
            {
                const std::string& fullName = menu->getItemFullName(result);                
                getProcessor()->loadStateFromPath(fullName);
				// TODO do filename stripping here instead of in button?
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
            mpProcessor->setProperty("key_scale", "12-equal");
            break;
        default:
            MLMenu* menu = findMenuByName("key_scale");
            if (menu)
            {
                // set model param to the full name of the file in the menu
                const std::string& fullName = menu->getItemFullName(result);
                mpProcessor->setProperty("key_scale", fullName);
            }
            break;
    }
}

void MLPluginController::doMoreMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):
		{
			bool enabled = mpProcessor->getEnvironment()->getFloatProperty("osc_enabled");
			mpProcessor->getEnvironment()->setProperty("osc_enabled", !enabled);
			break;
		}
        default:
			mpProcessor->getEnvironment()->setProperty("osc_port_offset", result - 2);
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
			else if(menuName == "key_more")
			{
				doMoreMenu(result);
			}
		}
	}
}

void MLPluginController::populatePresetMenu(const MLFileCollection& presetFiles)
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
	
#if SHOW_CONVERT_PRESETS
#if ML_MAC
	menu->addItem("Update presets...");
#endif
#endif
	menu->addSeparator();
    
    // add factory presets, those starting with the plugin name    
    MLMenuPtr factoryMenu(new MLMenu(presetFiles.getName()));
    presetFiles.getRoot()->buildMenuIncludingPrefix(factoryMenu, MLProjectInfo::projectName);
    menu->appendMenu(factoryMenu);
    
    menu->addSeparator();
    
    // add user presets, all the others
    MLMenuPtr userMenu(new MLMenu(presetFiles.getName()));
    presetFiles.getRoot()->buildMenuExcludingPrefix(userMenu, MLProjectInfo::projectName);
    menu->appendMenu(userMenu);
    menu->buildIndex();
}

// create a menu of the factory scales.
//
void MLPluginController::populateScaleMenu(const MLFileCollection& fileCollection)
{
    MLMenu* pMenu = createMenu("key_scale");
	
	// MLTEST pMenu was NULL here once, I swear it. How?
	
	pMenu->clear();
 	pMenu->addItem("12-equal");
    MLMenuPtr p = fileCollection.buildMenu();
    pMenu->appendMenu(p);
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
				
				int p = 1;
				for(it = nodeIndex.begin(); it != nodeIndex.end(); it++)
				{
					const std::string& name = *it;
					MLMenu::NodePtr subNode = node->getSubnodeByName(name);
					{
						std::ostringstream s;
						s << p++;
						const std::string pStr(s.str());
						subNode->setDisplayPrefix(std::string("[") + pStr + std::string("] "));
					}
				}
			}
		}
	}
}

#pragma mark MLFileCollection::Listener

void MLPluginController::processFileFromCollection (MLSymbol action, const MLFile& fileToProcess, const MLFileCollection& collection, int idx, int size)
{
	MLSymbol collectionName = collection.getName();
	
	if(action == "begin")
	{
	}
	else if(action == "process")
	{
		if(collectionName == "convert_user_presets")
		{
			File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
			File destRoot(newPresetsFolder);
			const std::string& relativeName = fileToProcess.getLongName();
			
			// If file at destination does not exist, or is older than the source, convert
			// source and overwrite destination.
			File destFile = destRoot.getChildFile(String(relativeName)).withFileExtension("mlpreset");
			if(!destFile.exists())
			{
				mpProcessor->loadPatchStateFromFile(fileToProcess);
				mpProcessor->saveStateToRelativePath(relativeName);
			}
			
			if(fileToProcess.getJuceFile().getLastModificationTime() > destFile.getLastModificationTime())
			{
				mpProcessor->loadPatchStateFromFile(fileToProcess);
				mpProcessor->saveStateToRelativePath(relativeName);
			}
		}
		else if(collectionName == "move_user_presets")
		{
			// move from old place to new if new file does not exist.
			File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
			File destRoot(newPresetsFolder);
			const std::string& relativeName = fileToProcess.getLongName();
			File destFile = destRoot.getChildFile(String(relativeName));
			if(!destFile.exists())
			{
				String presetStr(fileToProcess.getJuceFile().loadFileAsString());
				destFile.create();
				destFile.replaceWithText(presetStr);
			}
		}

	}
	else if(action == "update")
	{
		// unimplemented
	}
	else if(action == "end")
	{
		// search is ending, so we populate menus and the like.
		if(collectionName == "convert_user_presets")
		{
			mpProcessor->scanAllFilesImmediate();
			mpProcessor->suspendProcessing(false);
		}
		else if(collectionName == "scales")
		{
			populateScaleMenu(collection);
		}
		else if(collectionName == "presets")
		{
			populatePresetMenu(collection);
			flagMIDIProgramsInPresetMenu();
		}
	}
}

#if ML_MAC

#pragma mark ConvertProgressDisplayThread

// ConvertProgressDisplayThread: progress display for preset converter.
// TODO write on our own base class to replace ThreadWithProgressWindow.

class ConvertProgressDisplayThread : public ThreadWithProgressWindow, public MLPropertyListener, public DeletedAtShutdown
{
public:
    ConvertProgressDisplayThread(MLPluginController* pC, MLFileCollectionPtr pFiles) :
        pController(pC),
        MLPropertyListener(&(*pFiles)),
        mpFileCollection(pFiles),
        myProgress(0),
        mFilesConverted(0),
        ThreadWithProgressWindow ("", true, true)
    {
    }
    
    ~ConvertProgressDisplayThread() {}
    
    void run() override
    {
        std::string rootStr = mpFileCollection->getRoot()->getAbsolutePath();
        rootStr = std::string("Updating presets from ") + rootStr + std::string("...");
        setProgress(-1.0);
        setStatusMessage (rootStr);

        while(myProgress < 1.0)
        {
            if (threadShouldExit()) return;
            updateChangedProperties();
            wait(10);
        }
    }
    
    void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal)
    {
        if(property == "progress")
        {
            float p = newVal.getFloatValue();
            if(p < 1.)
            {
                mFilesConverted++;
            }
            myProgress = p;
            setProgress(p);
        }
    }
    
    MLPluginController* pController;
    MLFileCollectionPtr mpFileCollection;
    float myProgress;
    int mFilesConverted;
    
    // This method gets called from the message thread to end our thread.
    void threadComplete (bool userPressedCancel)
    {
        File destDir = getDefaultFileLocation(kPresetFiles);
        String destDirName = destDir.getFullPathName();
        if (userPressedCancel)
        {
            pController->cancelConvert();
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets was cancelled.",
                "Some presets may not have been converted.");
        }
        else
        {
            if(mFilesConverted)
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets successful.",
                    String("Converted presets were added to ") + destDirName + ".");
            }
            else
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "No presets found to convert.", "");
            }
        }
    }
};


#pragma mark ConvertPresetsThread

class ConvertPresetsThread : public Thread, public DeletedAtShutdown
{
public:
    ConvertPresetsThread(MLFileCollectionPtr p, MLFileCollectionPtr q) :
        Thread("update_presets_thread"),
        mPresetsToMove(p),
        mPresetsToConvert(q)
    {
    }
    
    ~ConvertPresetsThread()
    {
        stopThread(1000);
    }
    
    void run()
    {
        int interFileDelay = 2;
        
        // move files in immediate mode
        mPresetsToMove->searchForFilesImmediate(interFileDelay);
        
        // wait for move to finish
        while(mPresetsToMove->getFloatProperty("progress") < 1.)
        {
            if (threadShouldExit()) return;
            wait(10);
        }
        
        // convert files in immediate mode
        mPresetsToConvert->searchForFilesImmediate(interFileDelay);
        
        // wait for convert to finish
        while(mPresetsToConvert->getFloatProperty("progress") < 1.)
        {
            if (threadShouldExit()) return;
            wait(10);
        }
    }
    
private:
    MLFileCollectionPtr mPresetsToConvert;
    MLFileCollectionPtr mPresetsToMove;
};

// only convert .aupreset (AU) to .mlpreset (VST) now. After Aalto 1.6 there will be no need to convert presets.
void MLPluginController::convertPresets()
{
    if(!mpProcessor) return;
    
    File presetsFolder = getDefaultFileLocation(kOldPresetFiles);
    if (presetsFolder != File::nonexistent)
    {
        mPresetsToMove = MLFileCollectionPtr(new MLFileCollection("move_user_presets", getDefaultFileLocation(kOldPresetFiles), ".mlpreset"));
        mPresetsToMove->addListener(this);
        mPresetsToConvert = MLFileCollectionPtr(new MLFileCollection("convert_user_presets", getDefaultFileLocation(kOldPresetFiles), ".aupreset"));
        mPresetsToConvert->addListener(this);

        // turn off audio -- will be turned back on by finish or cancel
        mpProcessor->suspendProcessing(true);

        mConvertProgressThread = std::tr1::shared_ptr<ThreadWithProgressWindow>(new ConvertProgressDisplayThread(this, mPresetsToConvert));
        mConvertProgressThread->launchThread();
        
        mConvertPresetsThread = std::tr1::shared_ptr<Thread>(new ConvertPresetsThread(mPresetsToMove, mPresetsToConvert));
        mConvertPresetsThread->startThread();
    }
    else
    {
        debug() << "convertPresets: couldn't find preset folder " << presetsFolder.getFullPathName() << ".\n";
    }
}

void MLPluginController::cancelConvert()
{
    mPresetsToMove->cancelSearch();
    mPresetsToConvert->cancelSearch();
    mConvertPresetsThread->stopThread(1000);
    mpProcessor->scanAllFilesImmediate();
    mpProcessor->suspendProcessing(false);
}



#endif // ML_MAC

