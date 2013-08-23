
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_DEBUG_H
#define _ML_DEBUG_H

#include "MLPlatform.h"
#include "MLTextStreamListener.h"
#include "JuceHeader.h" // requires JUCE only for message thread checking

#include <iostream>

class MLTextStream
{
public:
	// set maximum to prevent local stream from growing too large
	static const int kMaxLocalItems = 1024;

	MLTextStream(const char* name);
	~MLTextStream();

	// send item to stream
	template<typename itemType>
	MLTextStream& (operator<<)(itemType const& item)
	{
		if(mActive)
		{		
			// If we have a listener Widget, send the item to
			// the Listener’s stream. 
			if(mpListener)
			{
				mpListener->getStream() << item;
			}
			// Else send the item to the local stream, which will be
			// made visible next time display() is called.
			else
			{
				// in case display() is never called, don’t allow local stream 
				// to grow without limit.
				if(mItemsInLocalStream++ < kMaxLocalItems)
				{
					mLocalStream << item;
				}
			}
		}
		return *this;
	}
	
	void sendOutputToListener(MLTextStreamListener* pL);	
	void setActive(bool b) { mActive = b; }
	void flush(void);

	// empty local stream to destination from message thread
	void display();

private:
	std::string mName;
	bool mActive;
	MLTextStreamListener* mpListener;
	
	std::stringstream mLocalStream;
	int mItemsInLocalStream;
};

extern MLTextStream& debug(void);
extern MLTextStream& MLError(void);
extern MLTextStream& MLConsole(void);

#endif // _ML_DEBUG_H
