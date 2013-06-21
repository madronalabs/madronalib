#include <iostream>
#include "MLProc.h"
#include "MLDSP.h"

void testSymbols()
{
	MLSymbol a("test#15");
	MLSymbol b("test#*");
	MLSymbol c("test2#17");
	
	/*
	debug << a.getString() << "\n";
	debug << b.getString() << "\n";
	
	MLPath p("voices/voice#2/osc/eskimo/level");
	debug << p << "\n";
	
	debug << q << "\n";
	*/
	
	theSymbolTable().dump();
	theSymbolTable().audit();
}

void testSignals()
{
	MLSignal x(256);
	for(int i=0; i<5; ++i)
	{
		x[i] = (float)i*0.001;
	}
	
	x[1] = 1.5f;
	
	const float q = x[1];
	
	float a ( x[1]);
	float b = x[2];
	debug() << "result:" << a << ", " << b << ", " << x[3] << "\n";
	
	const MLSignal y = x;
	debug() << y[1];
}

int main (int argc, char * const argv[]) 
{
    // insert code here...
    std::cout << "MadronaLib tests.\n";
	
	testSignals();
    return 0;
}

