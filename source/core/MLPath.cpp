
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPath.h"

namespace ml {

	// an empty path
	Path::Path() :
		mSize(0), mCopy(0)
	{	
		memset(mpData, '\0', kPathMaxSymbols*sizeof(ml::Symbol));
	}

	// parse an input string into our representation: an array of ml::Symbols.
	Path::Path(const char * str) :
		mSize(0), mCopy(0)
	{
		memset(mpData, '\0', kPathMaxSymbols*sizeof(ml::Symbol));
		parsePathString(str);
	}

	// allocate a path with one symbol.
	Path::Path(const ml::Symbol sym) :
		mSize(0), mCopy(0)
	{
		memset(mpData, '\0', kPathMaxSymbols*sizeof(ml::Symbol));
		addSymbol(sym);
	}

	void Path::parsePathString(const char* pathStr)
	{
		const int pathStrBytes = strlen(pathStr);	

		// TODO this small-local alloc could be a pattern
		constexpr int kLocalBufSize = 128;
		char UTF8buf[kLocalBufSize] = {0};	
		std::vector<char> UTF8Vec; // used only if length > kLocalBufSize
		
		char* pBuf;
		if(pathStrBytes < kLocalBufSize)
		{
			pBuf = UTF8buf;
		}
		else
		{
			UTF8Vec.resize(pathStrBytes + 1, 0);
			pBuf = UTF8Vec.data();
		}
		char* beginPoint = pBuf;
		char* beginSymbol = pBuf;
		char* endPoint = pBuf;			

		auto first = utf::codepoint_iterator<const char*>(pathStr);		
		auto it = first;
		int pointSizeAsUTF8;
		char c = 0;
		do
		{
			do
			{
				// write the codepoint as UTF-8 to the buffer and advance pb
				endPoint = utf::internal::utf_traits<utf::utf8>::encode(*it, beginPoint);				
				pointSizeAsUTF8 = endPoint - beginPoint;
				
				// if we have a one-byte character, see if it's a slash
				c = (pointSizeAsUTF8 == 1) ? *it : -1;
				beginPoint = endPoint;
				++it;
			}	
			while((c != '/') && (c != 0));
			
			int newSymbolBytes = (endPoint - beginSymbol) - 1;
			addSymbol(ml::Symbol(beginSymbol, newSymbolBytes));
			beginSymbol = endPoint;
		}
		while(c != 0);
	}

	Path::Path(const Path& b)
	{	
		memcpy(mpData, b.mpData, sizeof(Path));
	}

	Path::~Path() 
	{
	}

	void Path::addSymbol(ml::Symbol sym)
	{
		if (mSize < kPathMaxSymbols)
		{
			mpData[mSize++] = sym;
		}
		else 
		{
			// TODO something!
			//debug() << "Path::addSymbol: max path length exceeded!\n";
		}
	}

	ml::Symbol Path::head() const
	{
		return mpData[0];
	}

	Path Path::tail() const
	{
		Path r;
		for(int n=1; n<mSize; ++n)
		{
			r.addSymbol(mpData[n]);
		}
		return r;
	}
		
	std::ostream& operator<< (std::ostream& out, const ml::Path & r)
	{
		for(auto sym : r)
		{
			out << "/";
			out << sym;
		}
		unsigned copy = r.getCopy();
		if (copy)
		{
			out << "(#" << copy << ")";
		}
		return out;
	}

} // namespace ml


