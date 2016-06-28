
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <string.h>

#include "MLSymbol.h"

// an Path describes the address of one or more elements in a tree
// of such elements, for example one or more MLProcs in a graph.
// it has a list of symbols, each of which the name of a container, 
// except the last symbol which can be the name of any proc in the 
// previous container.  
// 
// the copy number lets the path refer to one specific MLProc in a
// multiple container if the Path ends on that container. Copies are 
// indexed starting at 1. If the copy number is 0, the Path refers
// to all the copies.

// Maximum path depth allows stack allocation / use in audio threads.
const int kPathMaxSymbols = 15;

namespace ml {
	
class Path
{
friend std::ostream& operator<< (std::ostream& out, const Path & r);
	
public:
	Path();	
	Path(const char * str);
	Path(const Symbol sym);
	Path(const Path& b);		
	~Path();

	// boolean test.
	explicit operator bool() const { return (mSize != 0); }
	void addSymbol(Symbol sym);
	
	Symbol head() const;
	Path tail() const;
		
	inline int getSize() const { return static_cast< int >(mSize); }
	inline Symbol getElement(int n) const { return mpData[n]; }
	inline int getCopy() const { return mCopy; }
	
	inline void setCopy(int c) { mCopy = c; } // MLTEST to remove
	
	friend class const_iterator;
	class const_iterator
	{
	public:
		const_iterator(const Path* p) : mpPath(p), mIndex(0) { }		
		const_iterator(const Path* p, int startIndex) : mpPath(p), mIndex(startIndex) { }
		~const_iterator() {}
		
		bool operator==(const const_iterator& b) const 
		{ 
			if (mpPath != b.mpPath) return false;
			return (mIndex == b.mIndex); 
		}
		
		bool operator!=(const const_iterator& b) const 
		{ 
			return !(*this == b); 
		}
		
		const Symbol operator*() const 
		{ 
			return mpPath->getElement(mIndex); 
		}

		const const_iterator& operator++()
		{			
			mIndex++;
			return *this;
		}
		
		const_iterator& operator++(int)
		{
			this->operator++();
			return *this;
		}
		
		const_iterator& operator+(int distance)
		{
			for(int n=0; n<distance; ++n)
			{
				this->operator++();
			}
			return *this;
		}
		
	private:
		const Path* mpPath;
		int mIndex;
	};	
	
	inline const_iterator begin() const
	{
		return const_iterator(this);
	}
	
	inline const_iterator end() const
	{
		return const_iterator(this, static_cast<int> (mSize) );
	}
	
protected:
	void parsePathString(const char* pathStr);

	Symbol mpData[kPathMaxSymbols];
	unsigned char mSize;
	unsigned char mCopy;
	unsigned char _dummy; 
	unsigned char _dummy2; 
	// sizeof(Path) = 64
};

} // namespace ml

