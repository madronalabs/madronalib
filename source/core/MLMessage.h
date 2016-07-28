//
// MLMessage.h
// madronalib
//
// work in progress. Just some ideas at this point. 

#pragma once

#include "MLSymbol.h"

namespace ml 
{
	
class Message
{
private:
	Symbol mTitle;
	// TextFragmentLikeStorage mBody; // serialized list of {Symbol, Property} pairs uses small stack objects when possible.
	
	// tokenized / serialized properties in message body uses TextFragment for small stack objects.
	// could be a wrapper around TextFragment.
	
public:	
	Message(Symbol title);  // then list of n* named properties.

};

	
} 
