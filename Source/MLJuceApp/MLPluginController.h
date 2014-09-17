
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINCTRLR_H__
#define __ML_PLUGINCTRLR_H__

#include "MLProjectInfo.h"
#include "MLReporter.h"
#include "MLSignalReporter.h"
#include "MLAppView.h"
#include "MLPluginEditor.h"

class MLPluginController : 
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

	// MLWidget::Listener
	virtual void handleWidgetAction(MLWidget* w, MLSymbol action, MLSymbol target, const MLProperty& val = MLProperty());
	
    // MLFileCollection::Listener
	void processFileFromCollection (const MLFile& file, const MLFileCollection& collection, int idx, int size);
 	
	// menus
 	void showMenu (MLSymbol menuName, MLSymbol instigatorName);
	virtual void menuItemChosen(MLSymbol menuName, int result);
	MLMenu* createMenu(MLSymbol menuName);
	MLMenu* findMenuByName(MLSymbol menuName); // TODO move into new controller base class
	void doScaleMenu(int result);
	void doPresetMenu(int result);

	MLPluginProcessor* getProcessor() const { return mpProcessor; }

#if ML_MAC
    void updatePresets();
    void cancelUpdate();
    MLFileCollectionPtr mPresetsToConvert;
    MLFileCollectionPtr mPresetsToMove;
    std::tr1::shared_ptr<ThreadWithProgressWindow> mConvertProgressThread;
    std::tr1::shared_ptr<Thread> mConvertPresetsThread;
    
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

};




#endif // __ML_PLUGINCTRLR_H__