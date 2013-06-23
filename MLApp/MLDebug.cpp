
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDebug.h"
const int kMLDebugMaxChars = 32768;

MLTextStream::MLTextStream(const char* name) : 
	mName(name), 
	mActive(true),
	mpListener(0),
	mItemCount(0)
{

}

MLTextStream::~MLTextStream()
{
	setActive(false);
	flush();
}

void MLTextStream::setListener(MLTextStreamListener* pL)
{
	// transfer any startup items
	if(!mpListener)
	{
		if(pL)
		{
			std::string outStr = mLocalStream.str();
			pL->getStream() << outStr;
			pL->getStream() << "---------------\n";
		}
	}
	mpListener = pL;
}

void MLTextStream::setActive(bool a)
{
	//printf("debug: turning output off.\n");
	mActive = a;
}

void MLTextStream::flush()
{
	mLocalStream.flush();
}

MLTextStream& debug(void)
{
	static MLTextStream theDebugMessageStream("debug");
	return theDebugMessageStream;
}

MLTextStream& MLError(void)
{
	static MLTextStream theErrorMessageStream("error");
	return theErrorMessageStream;
}


void MLTextStream::display()
{
	if(!mActive) return;
	if(mpListener)
	{
		mpListener->display();
	}
	else
	{
		// no listener, using stdout
		flush();
	}
}


#ifndef ML_MAC

void MLTextStream::displayImmediate()
{
	if(!mActive) return;
	if(mpListener)
	{
		mpListener->display();
	}
	else
	{
		// no listener, using stdout
		flush();
	}
}

#endif // ML_MAC
