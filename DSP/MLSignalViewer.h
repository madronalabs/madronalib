
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SIGNALVIEWER_H__
#define __ML_SIGNALVIEWER_H__

#include "MLWidget.h"

#include "MLProcContainer.h"
#include "MLSignalImage.h"
#include "MLProcScope.h"
#include "MLSymbol.h"
#include "MLPath.h"

#include "MLSignal.h"
#include "MLDSPEngine.h"

class MLSignalViewer
{
public:
	static const char* kViewProcName;

	MLSignalViewer(MLWidget* w, MLSymbol attr, int size = kMLSignalViewBufferSize);
	~MLSignalViewer();
	void setupViewSignal (MLDSPEngine* pEng, const MLSymbol sigName, unsigned voices);
	void doViewSignal();

	// set by the viewing component after looking at the signal, to tell the Editor
	// what to repaint next time around.  When there is nothing to repaint, set (0, 0, 1, 1)
	// so that the component gets a chance to update itself.
	void setRepaintRect(const MLRect& r);
	const MLRect& getRepaintRect() const;
	
	MLSymbol getSignalName() { return mSignalName; }	

protected:
	bool mViewingSignal;
	MLDSPEngine* mpEngine;
	
	MLSymbol mSignalName;
	MLWidget* mpWidget;
	MLSymbol mAttr;
	MLSignal mViewBuffer; 
	MLSignal mViewBuffer2; 
	unsigned mSize;

};



#endif