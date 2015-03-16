//
//  MLFFT.h
//  Soundplane
//
//  Created by Randy Jones on 3/14/15.
//
//

#ifndef __Soundplane__MLFFT__
#define __Soundplane__MLFFT__

#include <stdio.h>

#include "MLSignal.h"

int FFT(int dir, int m, float *x, float *y);
int FFT2D(MLSignal& cReal, MLSignal& cImag, int nx, int ny, int dir);
int FFT2DReal(MLSignal& cReal, int nx, int ny, int dir);

#endif /* defined(__Soundplane__MLFFT__) */
