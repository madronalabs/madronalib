
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
				std::cout << item;
			}
		}
		return *this;
	}
	
	void setListener(MLTextStreamListener* pL);	
	void setActive(bool a);
	void flush(void);
	
	// empty stream to destination, hopefully from message thread
	void display();

private:
	std::string mName;
	bool mActive;
	MLTextStreamListener* mpListener;
};

extern "C" MLTextStream& debug(void);
extern "C" MLTextStream& MLError(void);


#endif // _ML_DEBUG_H
