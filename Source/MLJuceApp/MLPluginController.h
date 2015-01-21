
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
	
	MLPluginController(MLPluginProcessor* const pProcessor);
    ~MLPluginController();

	MLAppView* getView(); 
	void setView(MLAppView* v); 
	
	// things to do after View is set
	virtual void initialize();

	void timerCallback();

	// MLWidget::Listener
	virtual void handleWidgetAction(MLWidget* w, MLSymbol action, MLSymbol target, const MLProperty& val = MLProperty());
	
    // MLFileCollection::Listener
	void processFileFromCollection (MLSymbol action, const MLFile& file, const MLFileCollection& collection, int idx, int size);
 	
	// menus
	//
	// controllers can override to rebuild menus before showing.
	virtual void updateMenu(MLSymbol menuName);
	// controllers override menuItemChosen to respond to menus.
	virtual void menuItemChosen(MLSymbol menuName, int result);
 	void showMenu (MLSymbol menuName, MLSymbol instigatorName);
	MLMenu* createMenu(MLSymbol menuName);
	MLMenu* findMenuByName(MLSymbol menuName); // TODO move into new controller base class
	void doScaleMenu(int result);
	void doPresetMenu(int result);
	void doMoreMenu(int result);

	MLPluginProcessor* getProcessor() const { return mpProcessor; }

#if ML_MAC
	
	friend class ConvertPresetsThread;
	class ConvertPresetsThread :
		public Thread,
		public DeletedAtShutdown,
		public MLFileCollection::Listener
	{
	public:
		ConvertPresetsThread(MLPluginController* pC);
		~ConvertPresetsThread();
		
		void run();
		
		// MLFileCollection::Listener
		void processFileFromCollection (MLSymbol action, const MLFile& file, const MLFileCollection& collection, int idx, int size);

	private:
		MLFileCollection* mpPresetsToConvertAU1, *mpPresetsToConvertAU2, *mpPresetsToConvertVST1, *mpPresetsToConvertVST2;
		MLPluginController* mpController;
	};
	
	class ConvertProgressDisplayThread :
		public ThreadWithProgressWindow,
		public DeletedAtShutdown
	{
	public:
		ConvertProgressDisplayThread(MLPluginController* pC) :
			ThreadWithProgressWindow ("", true, true),
			//        MLPropertyListener(&(*pFiles)),
			pController(pC),
			myProgress(0),
			mFilesConverted(0)
		{
		}
		
		void run() override;
		
		//void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
		
		MLPluginController* pController;
		float myProgress;
		int mFilesConverted;
		
		// This method gets called from the message thread to end our thread.
		void threadComplete (bool userPressedCancel);
	};

    void convertPresets();
	void setConvertProgress(float);
	float getConvertProgress() const;
	void fileConverted() { mFilesConverted++; }
	int getFilesConverted() const;
	void endConvertPresets();

protected:
    ConvertPresetsThread* mpConvertPresetsThread;
    
#endif // ML_MAC

protected:
	void prevPreset();
	void nextPreset();
	    
	MLAppView* mpView;
    
	WeakReference<MLPluginController>::Master masterReference;
	friend class WeakReference<MLPluginController>;	

private:
	void populatePresetMenu(const MLFileCollection& f);
	void populateScaleMenu(const MLFileCollection& f);
	void flagMIDIProgramsInPresetMenu();

	MLPluginProcessor* mpProcessor;
	std::string mVersionString;

	MLMenuMapT mMenuMap;
	int mClockDivider;
	bool mConvertPresetsThreadMarkedForDeath;
	float mConvertProgress;
	int mFilesConverted;
};




#endif // __ML_PLUGINCTRLR_H__