
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDebugDisplay.h"
#include "MLLookAndFeel.h"

static const int kMLDebugMaxChars = 32768;

//==============================================================================
MLDebugDisplay::MLDebugDisplay () :
	mpDoc(std::unique_ptr<CodeDocument>(new CodeDocument())),
	mpComp(std::unique_ptr<CodeEditorComponent>(new CodeEditorComponent(*mpDoc, nullptr)))
{
	MLWidget::setComponent(this);
	
	addAndMakeVisible(mpComp.get());
    
    // enable if you want to copy/paste text, but that may screw up keyboard events for plugins.
    mpComp->setWantsKeyboardFocus(false);
	mpComp->loadContent("");
	mpComp->setScrollbarThickness(12);
    
	std::cout << "STARTING Timer \n";
	startTimer(250);
}

MLDebugDisplay::~MLDebugDisplay()
{
	stopTimer();
}

void MLDebugDisplay::display()
{
	const ScopedLock lock(mStreamLock);	

	// if any text is selected let debug output accumulate in stream. 
	int selectSize = mpComp->getHighlightedRegion().getLength();
	if(selectSize > 0) return;
	
	std::cout.flush();
	mStream.flush();
	std::string outStr = mStream.str();
	const char* pOutput = outStr.c_str();
	String newStr(pOutput);

	// erase stream contents
	mStream.str(std::string());

	int len = newStr.length();
	if (len > 0)
	{
		// delete beginning if too big
		const CodeDocument::Position pos = mpComp->getCaretPos();
		const int tc = pos.getPosition();
		if (tc > kMLDebugMaxChars)
		{
			mpComp->loadContent("");
			debug() << "----debug data > " << (int)kMLDebugMaxChars << " bytes, truncated----\n\n";
		}
		
		int lastDocLine = mpDoc->getNumLines();
		int startLine = mpComp->getFirstLineOnScreen();
		int endLine = startLine + mpComp->getNumLinesOnScreen();		
		mpComp->moveCaretToEnd(false);
		mpComp->insertTextAtCaret(newStr);
		
		// if end of doc is off screen, reset saved position (don't scroll to text)
		if(lastDocLine > endLine + 1)
		{
			mpComp->scrollToLine(startLine);
		}
	}
}

void MLDebugDisplay::resizeWidget(const MLRect& b, const int)
{
	Component* pC = getComponent();
	if(pC)
	{	
		// adapt vrect to juce rect
		Rectangle<int> c(b.left(), b.top(), b.width(), b.height());
		pC->setBounds(c);
		mpComp->setBounds(0, 0, b.width(), b.height());
	}
}

void MLDebugDisplay::timerCallback()
{
	display();
}

/*

#if ML_WINDOWS

	// send to Juce Logger
	switch(mDebugMode)
	{	
		case(eMLDebugToStdout):
//				mOutputStream << item;				
			// note that this adds an extra carriage return unless we modify juce Logger						
//				Logger::outputDebugString (mOutputStream.str().c_str());
//				mOutputStream.str(std::string());

*/
	
