
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINCTRLR_H__
#define __ML_PLUGINCTRLR_H__

#include "MLPluginFormats.h"
#include "MLResponder.h"
#include "MLReporter.h"
#include "MLSignalReporter.h"
#include "MLPatcher.h"
#include "MLAppView.h"


class MLPluginController : 
	public MLResponder,
	public MLReporter,
	public MLSignalReporter
{
public:
	MLPluginController(MLPluginProcessor* const pProcessor);
    ~MLPluginController();

	MLAppView* getView(); 
	void setView(MLAppView* v); 
	
	// things to do after View is set
	virtual void initialize() {}

	// from MLResponder
    virtual void buttonClicked (MLButton*);
	void showMenu (MLSymbol menuName, MLSymbol instigatorName);
	virtual void menuItemChosen(MLSymbol menuName, int result);

	void dialValueChanged (MLDial*);
	void dialDragStarted (MLDial*);
	void dialDragEnded (MLDial*);
	void multiButtonValueChanged (MLMultiButton* pSlider, int idx);
	void multiSliderDragStarted (MLMultiSlider* pSlider, int idx);
	void multiSliderDragEnded (MLMultiSlider* pSlider, int idx);
	void multiSliderValueChanged (MLMultiSlider* pSlider, int idx);

	void loadPresetByIndex (int idx);
	int getIndexOfPreset(const std::string& name, const std::string& dir);
	
	void prevPreset();
	void nextPreset();
	String getPresetString(int n);

	MLPluginProcessor* getProcessor() const { return mpProcessor; }

	// called by wrappers to tell editor what type of plugin it is editing.
	void setPluginWrapperFormat(int format);
	void setupMenus();
	
	MLMenu* findMenuByName(MLSymbol menuName);	

//	MLSymbol getCurrMenuName() { return mCurrMenuName; }
//	void setCurrMenuInstigator(MLMenuButton* pI) { mCurrMenuInstigator = pI; }
//	MLMenuButton* getCurrMenuInstigator() { return mCurrMenuInstigator; }

	void doScaleMenu(int result);
	void doPresetMenu(int result);

#if ML_MAC
	void getPresetsToConvert(Array<File>* pResults);
	void convertPresets();
#endif // ML_MAC

protected:
	MLPluginFormats::pluginFormat mWrapperFormat;
	MLAppView* mpView;
	juce::CriticalSection mViewLock;
	
	WeakReference<MLPluginController>::Master masterReference;
	friend class WeakReference<MLPluginController>;	
private:
	MLPluginProcessor* mpProcessor; // contains Model
	//PopupMenu mPresetMenu; 
	
	String mCurrentPresetName;
	String mCurrentScaleName;
	String mCurrentScaleDir;
	std::string mVersionString;

	// TODO v.2
//	ScopedPointer <DirectoryContentsList> mFactoryPresetsList;
//	ScopedPointer <DirectoryContentsList> mUserPresetsList;
//	ScopedPointer <DirectoryContentsList> mScalesList;
//   TimeSliceThread mFactoryPresetsThread, mUserPresetsThread, mScalesThread;
 
	File mFactoryPresetsFolder, mUserPresetsFolder, mScalesFolder;
	File mCurrentPresetFolder;
	bool mFileLocationsOK;

	// stored indices for MIDI program changes-- hackish
	std::vector<File> mMIDIProgramFiles;

	MLMenuMapT mMenuMap; 	
	Array<File> mMenuPresetFiles;	
	int mPresetMenuStartItems;
	int mCurrentPresetIndex;
	Array<File> mScaleMenuFiles;	

	void findFilesOneLevelDeep(File& startDir, String extension, Array<File>& results, MLMenu* pMenu);

	void populatePresetMenu(); 
	void populateScaleMenu();	
 
};




#endif // __ML_PLUGINCTRLR_H__