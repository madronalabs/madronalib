#include <iostream>
#include "MLProc.h"

int main (int argc, char * const argv[]) 
{
    // insert code here...
    std::cout << "Hello, World!\n";
	
	MLSymbol a("test#15");
	MLSymbol b("test#*");
	MLSymbol c("test2#17");
	
	debug << a.getString() << "\n";
	debug << b.getString() << "\n";
	
	MLPath p("voices/voice#2/osc/eskimo/level");
	debug << p << "\n";
	
	MLPath q("chateau/crisp#*/osc/crisp#1/level");
	debug << q << "\n";
	
	theSymbolTable().dump();
	theSymbolTable().audit();
	
    return 0;
}


