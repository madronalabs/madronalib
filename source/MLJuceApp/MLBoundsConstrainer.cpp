
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLBoundsConstrainer.h"

MLBoundsConstrainer::MLBoundsConstrainer() : 
	mZoomable(true)
{
	setMinimumSize(480, 360);
}

MLBoundsConstrainer::~MLBoundsConstrainer()
{
}

void MLBoundsConstrainer::checkBounds (Rectangle<int>& bounds,
                                              const Rectangle<int>& ,
                                              const Rectangle<int>& ,
                                              const bool ,
                                              const bool ,
                                              const bool ,
                                              const bool )
{
	int x = bounds.getX();
	int y = bounds.getY();
	int w = bounds.getWidth();
	int h = bounds.getHeight();
	int minw = getMinimumWidth();
	int maxw = getMaximumWidth();
	int minh = getMinimumHeight();
	int maxh = getMaximumHeight();

	double r = getFixedAspectRatio();	

	// clamp dims
	w = clamp (w, minw, maxw);
	h = clamp (h, minh, maxh);
	
	// fix ratio
	const int rLimitH = 800;
	const int rLimitW = (double)rLimitH * r;			
	if(r > 0.0)
	{
		if(!mZoomable)
		{
			double inR = (double)w / (double)h;
			if(inR > r)
			{
				h = (double)w/r;
			}
			else
			{
				w = (double)h*r;
			}
		}
		else if((w < rLimitW) && (h < rLimitH))
		{		
			double inR = (double)w / (double)h;
			if(inR > r)
			{
				h = (double)w/r;
			}
			else
			{
				w = (double)h*r;
			}
		}
		else if(w < rLimitW)
		{		
			w = rLimitW;
		}
		else if(h < rLimitH)
		{		
			h = rLimitH;
		}
	}
			
 	bounds = Rectangle<int>(x, y, w, h);
    jassert (! bounds.isEmpty());
}
