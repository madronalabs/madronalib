
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#include <Cocoa/Cocoa.h>
#include "MLDebug.h"
	
#ifdef ML_MAC

void MLTextStream::displayImmediate()
{
	if(!mActive) return;
	if(mpListener)
	{
		mpListener->display();
	}
	else
	{
		std::string outStr = mLocalStream.str();
		int size = outStr.size();
		if(outStr[size - 1] == '\n')
		{		
			mLocalStream.str("");
			const char* cStr = outStr.c_str();
			NSLog(@"%s", cStr);
		}
	}
}
		
				
#endif	// ML_MAC
