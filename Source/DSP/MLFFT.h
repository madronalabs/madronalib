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

int FFT1DReal(MLSignal& cReal);
int FFT1DRealInverse(MLSignal& cReal);

//void FFTEachRowReal(MLSignal& cReal);
//void FFTEachRowRealInverse(MLSignal& cReal);

void FFTEachRow(MLSignal& aReal, MLSignal& bImag);
void FFTEachRowInverse(MLSignal& aReal, MLSignal& bImag);

void FFTEachRowDivide(MLSignal& aReal, MLSignal& bImag, MLSignal& cReal, MLSignal& dImag);

int FFT2DReal(MLSignal& cReal);
int FFT2DRealInverse(MLSignal& cReal);

#endif /* defined(__Soundplane__MLFFT__) */
