
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINCTRLR_H__
#define __ML_PLUGINCTRLR_H__

#include "MLProjectInfo.h"
#include "MLReporter.h"
#include "MLSignalReporter.h"
#include "MLAppView.h"

class MLPluginController :
	public Timer,
	public MLWidget::Listener,
	public MLReporter,
	public MLSignalReporter,
    public MLFileCollection::Listener
{
public:
	
	MLPluginController(MLPluginProcessor* pProcessor);
    ~MLPluginController();

	MLAppView* getView(); 
	void setView(MLAppView* v); 
	
	// things to do after View is set
	virtual void initialize();

	void timerCallback();

	// MLWidget::Listener
	virtual void handleWidgetAction(MLWidget* w, ml::Symbol action, ml::Symbol target, const MLProperty& val = MLProperty());
	
    // MLFileCollection::Listener
	void processFileFromCollection (ml::Symbol action, const MLFile file, const MLFileCollection& collection, int idx, int size);
 	
	// menus
	//
	// controllers can override to rebuild menus before showing.
	virtual void updateMenu(ml::Symbol menuName);
	// controllers override menuItemChosen to respond to menus.
	virtual void menuItemChosen(ml::Symbol menuName, int result);
 	virtual void showMenu (ml::Symbol menuName, ml::Symbol instigatorName);
	MLMenu* createMenu(ml::Symbol menuName);
	MLMenu* findMenuByName(ml::Symbol menuName); // TODO move into new controller base class
	
	// TODO move menus to MLSymbols internally, with a translation table to turn the symbols into int'l strings
	void buildMenuFromSymbolVector(ml::Symbol menuName, const std::vector<std::string> & v);

	MLPluginProcessor* getProcessor() const { return mpProcessor; }

#if ML_MAC
	
	std::auto_ptr<MLFileCollection> mPresetsToConvertAU1, mPresetsToConvertAU2, mPresetsToConvertVST1, mPresetsToConvertVST2;
	
	class ConvertProgressDisplayThread :
		public ThreadWithProgressWindow,
		public DeletedAtShutdown
	{
	public:
		ConvertProgressDisplayThread(MLPluginController* pC) :
			ThreadWithProgressWindow (" ", true, true),
			pController(pC),
			myProgress(0.),
			mFilesConverted(0)
		{
			setStatusMessage ("Getting ready...");
		}
		
		void run() override;
		
		MLPluginController* pController;
		float myProgress;
		int mFilesConverted;
		
		// This method gets called from the message thread to end our thread.
		void threadComplete (bool userPressedCancel);
	};

    void convertPresets();
	float getConvertProgress();
	int getFilesConverted() const;
	void endConvertPresets();
	
#endif // ML_MAC

protected:
	    
	MLAppView* mpView;
    
	WeakReference<MLPluginController>::Master masterReference;
	friend class WeakReference<MLPluginController>;	

private:
	
	// file action queue for converting presets
	class FileAction
	{
	public:
		FileAction() {}
		FileAction(ml::Symbol action, const MLFile file, const MLFileCollection* collection, int idx, int size) :
			mAction(action), mFile(file), mCollection(collection), mIdx(idx), mSize(size)
			{}
		~FileAction() {}
		
		ml::Symbol mAction;
		const MLFile mFile;
		const MLFileCollection* mCollection;
		int mIdx;
		int mSize;
	};
	std::vector<FileAction> mFileActionData;
	PaUtilRingBuffer mFileActionQueue;
	void clearFileActionQueue();
	void doFileQueueAction(FileAction a);
	
	void doScaleMenu(int result);
	void doPresetMenu(int result);
	void doSettingsMenu(int result);
	void loadPresetByMenuIndex(int result);
	
	void populateScaleMenu(const MLFileCollection& f);
	void populatePresetMenu(const MLFileCollection& f);
	void populateSettingsMenu();
	
	void flagMIDIProgramsInPresetMenu();

	MLPluginProcessor* mpProcessor;
	std::string mVersionString;

	MLMenuMapT mMenuMap;
	int mClockDivider;
	
	bool mConvertingPresets;
	int mFilesToProcess;
	int mFilesConverted;
	int mFilesProcessed;
	int mMaxFileQueueSize;
	
	// todo symbols for menus and get rid of these
	int mProtocolMenuItemStart;
	int mOSCMenuItemStart;
	
	// TODO store preset menu iterator
};




#endif // __ML_PLUGINCTRLR_H__