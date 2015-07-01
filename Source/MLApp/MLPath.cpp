
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPath.h"

// an empty path
MLPath::MLPath() :
	mStart(0), mEnd(0), mCopy(0)
{	
	memset(mpData, '\0', kMLPathMaxSymbols*sizeof(MLSymbol));
}

// parse an input string into our representation: an array of MLSymbols.
MLPath::MLPath(const char * str) :
	mStart(0), mEnd(0), mCopy(0)
{
	memset(mpData, '\0', kMLPathMaxSymbols*sizeof(MLSymbol));
	unsigned symStart = 0;
	unsigned symEnd = 0;
	
	// get token up to each delimiter
	int endPos = 0;
	while(str[endPos])
	{
		for(endPos = symStart; str[endPos]; endPos++)
		{
			if (str[endPos] == '/') 
			{
				break;
			}
		}
		symEnd = endPos - 1;
		
		// make symbol
		addSymbol(std::string(str+symStart, symEnd - symStart + 1));
		symStart = symEnd + 2;
	}
}

// allocate a path with one symbol.
MLPath::MLPath(const MLSymbol sym) :
	mStart(0), mEnd(0), mCopy(0)
{
	memset(mpData, '\0', kMLPathMaxSymbols*sizeof(MLSymbol));
	addSymbol(sym);
}

MLPath::MLPath(const MLPath& b)
{	
	memcpy(mpData, b.mpData, sizeof(MLPath));
}

MLPath::MLPath(const MLPath& b, int start) 
{	
	memcpy(mpData, b.mpData, sizeof(MLPath));
	mStart = start;
}

MLPath::~MLPath() 
{
}

void MLPath::addSymbol(MLSymbol sym)
{
	if (mEnd < kMLPathMaxSymbols-1)
	{
		mpData[mEnd++] = sym;
	}
	else 
	{
		debug() << "MLPath::addSymbol: max path length exceeded!\n";
	}
}

MLSymbol MLPath::head() const
{
	return mpData[mStart];
}

// return a new Path referring to this one
const MLPath MLPath::tail() const
{
	int start = mStart;
	if (start < mEnd)
	{
		start++;
	}
	return MLPath(*this, start);
}

bool MLPath::empty() const
{
	return (mStart == mEnd);
}

int MLPath::length() const
{
	return mEnd - mStart;
}

std::ostream& operator<< (std::ostream& out, const MLPath & r)
{
	if (!r.empty())
	{
		for(int i=r.mStart; i<r.mEnd; ++i)
		{
			out << r.mpData[i];
			if (i < r.mEnd - 1)
			{
				out << "/";
			}
		}
		unsigned copy = r.getCopy();
		if (copy)
		{
			out << "(#" << copy << ")";
		}
	}
	return out;
}


