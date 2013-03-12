
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDebug.h"
const int kMLDebugMaxChars = 32768;

MLTextStream::MLTextStream(const char* name) : 
	mName(name), 
	mActive(true),
	mpListener(0)
{

}

MLTextStream::~MLTextStream()
{
	setActive(false);
	flush();
}

void MLTextStream::setListener(MLTextStreamListener* pL)
{
	mpListener = pL;
}

void MLTextStream::setActive(bool a)
{
	//printf("debug: turning output off.\n");
	mActive = a;
}

void MLTextStream::flush()
{

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
	}
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

