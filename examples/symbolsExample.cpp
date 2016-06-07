//
//  symbolsExample.cpp
//  madronalib
//
//  Created by Randy Jones on 5/17/16.
//
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
#include "MLTextUtils.h"

#include <iostream>

#include "MLTextFragment.h"

using namespace ml;

// This example class shows how to write a method like setParam() that can accept string literals
// as parameters so that the symbols are hashed at compile time. 
class TestProc
{
public:
	
	TestProc(){}
	~TestProc(){}
	
	// NOTE: template syntax here is needed to get the string literal length N at compile time.
	template<size_t N>
	inline void setParam(const char(&name)[N], float val)
	{
		std::cout << "setParam - HSL\n";
		HashedCharArray hsl(name);
		Symbol m(hsl);
		map[m] = val;
	}
	
	inline void setParam(const Symbol name, float val)
	{
		std::cout << "setParam - Symbol\n";
		map[name] = val;
	}
	
	template<size_t N>
	inline float getParam(const char(&name)[N])
	{
		std::cout << "getParam - HSL\n";
		HashedCharArray hsl(name);
		Symbol m(hsl);
		return map[m];
	}
	
	inline float getParam(const Symbol name)
	{
		std::cout << "getParam - Symbol\n";
		return map[name];
	}
	
	std::map< Symbol, float > map;
	
};


const char a[] = "x";
const char* b = "fff";
const char* c = "fff";
const char* f() { return b; }

int main()
{
	const char * letters("abcd");
	
	uint32_t hashTest = krHash0(letters, strlen(letters));	
	
	std::cout << std::hex << hashTest << std::dec << "\n";
	
	std::cout << "Symbols example:\n";
	
	std::cout << "\n\nTESTING sym param name ";
	
	TestProc p;
	
	p.setParam("9xo", 880.f);
	p.setParam("frequencies", 440.f);
	p.setParam("frequency", 440.f);
	p.setParam("fxo", 110.f);
	p.setParam("fyo", 909);
	
	char name1[4] = "foo";
	name1[1] = 'x';

	const char name2[4] = "fyo";
	
	// Foo::Foo(const char (&)[N]) [N = 2]
	std::cout << "frequency " << p.getParam("frequency") << "\n";
	
	// Foo::Foo(T, typename IsCharPtr<T>::Type) [T = char *]
	std::cout << "name1 (not const) " << p.getParam(name1) << "\n";
	
	// Foo::Foo(const char (&)[N]) [N = 2]
	std::cout << "name2 (const) " << p.getParam(name2) << "\n";
	
	// Foo::Foo(T, typename IsCharPtr<T>::Type) [T = const char *]
	std::cout << "ff " << p.getParam(b) << "\n";
		
	theSymbolTable().dump();
	
	TextFragment t ("hello");
	TextFragment u (",");
	TextFragment v (" ");
	TextFragment w ("world!");
	
	std::cout << t << u << v << w << "\n";
	
	TextFragment test1 ("hello.exe");
	int dotLoc = textUtils::findLast(test1, 'l');
	std::cout << "loc: " << dotLoc << "\n";

#if HAVE_U8_LITERALS
	TextFragment kobayashi(u8"小林 尊");
#else
	TextFragment kobayashi("\xE5\xB0\x8F\xE6\x9E\x97\x20\xE5\xB0\x8A");
#endif	
	
	utf::codepoint_type hayashi[1]{0x6797};
	int hayashiLoc = textUtils::findFirst(kobayashi, hayashi[0]);
	std::cout << "hayashi loc: " << hayashiLoc << "\n";

	auto first = utf::codepoint_iterator<const char*>(kobayashi.text);
	auto last = utf::codepoint_iterator<const char*>(kobayashi.text + kobayashi.length);

	for (auto it = first; it != last; ++it) 
	{
		std::cout << std::hex << *it << " ";
	}	
	std::cout << "\n";
	
	// UTF-8 encode a single codepoint to preallocated buffer
	const int kBufSize = 4;
	unsigned char buf[kBufSize];
	auto hv = utf::make_stringview(hayashi);
	unsigned char* pb = buf;
	for (utf::codepoint_iterator<const char32_t *> it = hv.begin(); it != hv.end(); ++it) 
	{
		pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
		if((pb - buf) >= kBufSize) break;
	}

	for(int n = 0; n < kBufSize; ++n)
	{
		std::cout << "0x" << std::hex << (unsigned long)buf[n] << std::dec << " ";
	}
	std::cout << "\n";	
	
	return 0;
}



