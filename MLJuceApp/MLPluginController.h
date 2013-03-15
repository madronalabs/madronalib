
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINCTRLR_H__
#define __ML_PLUGINCTRLR_H__

#include "MLPluginFormats.h"
#include "MLResponder.h"
#include "MLReporter.h"
#include "MLSignalReporter.h"
#include "MLAppView.h"

// TODO should be pure virtual?


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
	
	void updateParametersFromFilter();

	// from MLButton::Listener
    virtual void buttonClicked (MLButton*);
	
	// from MLMenuButton::Listener
	void showMenu (MLSymbol menuName, MLMenuButton* instigator);

 	// from MLDial::Listener
    virtual void dialValueChanged (MLDial*);
	virtual void dialDragStarted (MLDial*);
	virtual void dialDragEnded (MLDial*);

	// from MLMultiButton::Listener
    virtual void multiButtonValueChanged (MLMultiButton* pSlider, int idx);

	// from MLMultiSlider::Listener
	virtual void multiSliderDragStarted (MLMultiSlider* pSlider, int idx);
	virtual void multiSliderDragEnded (MLMultiSlider* pSlider, int idx);
    virtual void multiSliderValueChanged (MLMultiSlider* pSlider, int idx);

	void loadPresetByIndex (int idx);
	int getIndexOfPreset(const std::string& name, const std::string& dir);
	
	void prevPreset();
	void nextPreset();
	String getPresetString(int n);

	MLPluginProcessor* getProcessor() const { return mpProcessor; }

	// called by wrappers to tell editor what type of plugin it is editing.
	void setPluginWrapperFormat(int format);
	void setupMenus();
	
	MLSymbol getCurrMenuName() { return mCurrMenuName; }
	void setCurrMenuInstigator(MLMenuButton* pI) { mCurrMenuInstigator = pI; }
	MLMenuButton* getCurrMenuInstigator() { return mCurrMenuInstigator; }

	void doScaleMenu(int result);
	void doPresetMenu(int result);

#if ML_MAC
	void getPresetsToConvert(Array<File>* pResults);
	void convertPresets();
#endif // ML_MAC

protected:
	MLPluginFormats::pluginFormat mWrapperFormat;
	MLAppView* mpView;
	
private:
	PopupMenu mPresetMenu; 
	
	MLSymbol mCurrMenuName;
	MLMenuButton* mCurrMenuInstigator;

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
	MLPluginProcessor* mpProcessor;

	OwnedArray<PopupMenu> mPresetSubMenus;
	Array<String> mMenuItemStrings;
	Array<File> mMenuPresetFiles;	
	int mPresetMenuStartItems;
	int mCurrentPresetIndex;
	
	// stored indices for MIDI program changes-- hackish
	std::vector<File> mMIDIProgramFiles;

	PopupMenu mScaleMenu;	
	OwnedArray<PopupMenu> mScaleSubMenus;
	Array<String> mScaleMenuItemStrings;
	Array<File> mScaleMenuFiles;	

	void findFilesOneLevelDeep(File& startDir, String extension, Array<File>& results, 
		Array<String>* menuStrings, PopupMenu* pMenu, OwnedArray<PopupMenu>* subMenus);

	void populatePresetMenu(); 
	void populateScaleMenu();	
 
};




#endif // __ML_PLUGINCTRLR_H__