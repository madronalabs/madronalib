
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_PATH_H
#define _ML_PATH_H

#include <string.h>

#include "MLDebug.h"
#include "MLSymbol.h"

// an MLPath describes the address of one or more elements in a tree
// of such elements, for example one or more MLProcs in a graph.
// it has a list of symbols, each of which the name of a container, 
// except the last symbol which can be the name of any proc in the 
// previous container.  
// 
// the copy number lets the path refer to one specific MLProc in a
// multiple container.

// TODO Design fail: the copy number cannot possibly handle multis inside multis,
// which need to be possible eventually. Rather there needs to be the
// possibility of either a wildcard or a copy number at each multi.

// TODO dynamic, come on
const int kMLPathMaxSymbols = 14;

class MLPath
{
friend std::ostream& operator<< (std::ostream& out, const MLPath & r);
public:
	MLPath();	
	MLPath(const char * str);
	MLPath(const MLSymbol sym);
	MLPath(const MLPath& b);	
	MLPath(const MLPath& b, int start);
	~MLPath();
	
	// boolean test.
	explicit operator bool() const { return !empty(); }
	bool empty() const;
	int length() const;
	void addSymbol(MLSymbol sym);
	
	MLSymbol head() const;
	const MLPath tail() const;
		
	void setCopy(int c) { mCopy = c; }
	int getCopy() const { return mCopy; }
	
private:
	MLSymbol mpData[kMLPathMaxSymbols];
	unsigned char mStart;
	unsigned char mEnd;
	unsigned char mCopy;
	unsigned char _dummy; // sizeof(MLPath) = 32
};

std::ostream& operator<< (std::ostream& out, const MLPath & r);


#endif // _ML_PATH_H

