
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_DEBUG_H
#define _ML_DEBUG_H

#include "MLPlatform.h"
#include "MLTextStreamListener.h"

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
			if(mpListener)
			{
				mpListener->getStream() << item;
			}
			else
			{
				mLocalStream << item;
				displayImmediate();
				/*
				std::cerr << item;
				// for catching initial messages before UI is made
				if(mItemCount++ < kMaxLocalItems)
				{
					mLocalStream << item;
				}
				*/
			}
		}
		return *this;
	}
	
	void setListener(MLTextStreamListener* pL);	
	void setActive(bool a);
	void flush(void);
	
	void displayImmediate();

	// empty stream to destination, hopefully from message thread
	void display();

private:
	std::string mName;
	bool mActive;
	MLTextStreamListener* mpListener;
	
	std::stringstream mLocalStream;
	int mItemCount;
};

extern "C" MLTextStream& debug(void);
extern "C" MLTextStream& MLError(void);

#endif // _ML_DEBUG_H
