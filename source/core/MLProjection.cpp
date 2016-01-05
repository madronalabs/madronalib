//
//  MLProjection.cpp
//  Virta
//
//  Created by Randy Jones on 12/27/15.
//
//

#include "MLProjection.h"

namespace ml
{
	namespace projections
	{
		// constant projections 		
		const Projection linear( [](float x){return x;} );
		const Projection flip( [](float x){return 1 - x;} );
		const Projection clip( [](float x){return clamp(x, 0.f, 1.f);} );
		const Projection smoothstep( [](float x){return 3*x*x - 2*x*x*x;} );
		const Projection bell( [](float x){float px = x*2 - 1; return powf(2.f, -(10.f*px*px));} );
	}
}