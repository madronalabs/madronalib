
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPlatform.h"
#include <Cocoa/Cocoa.h>
#include "MLDebug.h"
//#include "JuceHeader.h"
	
#ifdef ML_MAC

void MLTextStream::display()
{
	/*
    if (!(juce::MessageManager::getInstance()->isThisTheMessageThread()))
    {
        return;
    }
	 */
	if(mpListener)
	{
		mpListener->display();
	}
	else
	{
		// no listener, send to NSLog() for viewing in XCode or Console
		flush();
		std::string outStr = mLocalStream.str();
		int size = outStr.size();
		if(outStr[size - 1] == '\n')
		{		
			mLocalStream.str("");
			const char* cStr = outStr.c_str();
            {
                NSLog(@"%s", cStr);
            }
		}
	}
}
				
#endif	// ML_MAC
