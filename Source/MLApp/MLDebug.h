
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


#ifdef ML_WINDOWS

	if (!(juce::MessageManager::getInstance()->isThisTheMessageThread())) 
	{
		printf("Windows: no debugging outside of message thread!\n");
		return *this;
	}

#endif 

		if(mActive)
		{		
			// If we have a listener Widget, send the item to
			// the Listener’s stream. 
			if(mpListener)
			{
                std::stringstream& s = mpListener->getStream();
				s << item;
			}
			// Else send the item to the local stream, which will be
			// made visible next time display() is called.
			else
			{
#if defined (NDEBUG) || defined (_WINDOWS)
				// in case display() is never called, don’t allow local stream
				// to grow without limit.
				if(mItemsInLocalStream++ < kMaxLocalItems)
				{
					mLocalStream << item;
				}
                else
                {                    
                    mLocalStream << "\n******** debug stream full, some items after this will be lost! ********\n\n ";
                }
#else
			std::cout << item;
#endif
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

// Send a message to the application’s or plugin’s debug output stream.
// in release builds this will be disabled completely.
//
extern MLTextStream& debug(void);
                    
// Send a message to the application or plugin’s error output.
// in release builds these messages will still be logged.
//
extern MLTextStream& MLError(void);

// Send a message to the application or plugin’s console, if one exists.
//
extern MLTextStream& MLConsole(void);

// start and stop display of debug messages.
//
extern void startDebugging();
extern void stopDebugging();

#endif // _ML_DEBUG_H
