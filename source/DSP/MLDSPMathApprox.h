// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2026 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <cstring>
#include <utility>

#include "MLDSPMathSIMD.h"

// cephes-derived approximate math functions adapted from code by Julien Pommier
// Copyright (C) 2007 Julien Pommier and licensed under the zlib license

// Float constants
static const float4 _ps_1(1.0f);
static const float4 _ps_0p5(0.5f);
static const float4 _ps_cephes_SQRTHF(0.707106781186547524f);
static const float4 _ps_cephes_log_p0(7.0376836292E-2f);
static const float4 _ps_cephes_log_p1(-1.1514610310E-1f);
static const float4 _ps_cephes_log_p2(1.1676998740E-1f);
static const float4 _ps_cephes_log_p3(-1.2420140846E-1f);
static const float4 _ps_cephes_log_p4(+1.4249322787E-1f);
static const float4 _ps_cephes_log_p5(-1.6668057665E-1f);
static const float4 _ps_cephes_log_p6(+2.0000714765E-1f);
static const float4 _ps_cephes_log_p7(-2.4999993993E-1f);
static const float4 _ps_cephes_log_p8(+3.3333331174E-1f);
static const float4 _ps_cephes_log_q1(-2.12194440e-4f);
static const float4 _ps_cephes_log_q2(0.693359375f);
static const float4 _ps_exp_hi(88.3762626647949f);
static const float4 _ps_exp_lo(-88.3762626647949f);
static const float4 _ps_cephes_LOG2EF(1.44269504088896341f);
static const float4 _ps_cephes_exp_C1(0.693359375f);
static const float4 _ps_cephes_exp_C2(-2.12194440e-4f);
static const float4 _ps_cephes_exp_p0(1.9875691500E-4f);
static const float4 _ps_cephes_exp_p1(1.3981999507E-3f);
static const float4 _ps_cephes_exp_p2(8.3334519073E-3f);
static const float4 _ps_cephes_exp_p3(4.1665795894E-2f);
static const float4 _ps_cephes_exp_p4(1.6666665459E-1f);
static const float4 _ps_cephes_exp_p5(5.0000001201E-1f);
static const float4 _ps_minus_cephes_DP1(-0.78515625f);
static const float4 _ps_minus_cephes_DP2(-2.4187564849853515625e-4f);
static const float4 _ps_minus_cephes_DP3(-3.77489497744594108e-8f);
static const float4 _ps_sincof_p0(-1.9515295891E-4f);
static const float4 _ps_sincof_p1(8.3321608736E-3f);
static const float4 _ps_sincof_p2(-1.6666654611E-1f);
static const float4 _ps_coscof_p0(2.443315711809948E-005f);
static const float4 _ps_coscof_p1(-1.388731625493765E-003f);
static const float4 _ps_coscof_p2(4.166664568298827E-002f);
static const float4 _ps_cephes_FOPI(1.27323954473516f);  // 4 / M_PI

// Integer constants
static const int4 _pi32_min_norm_pos(0x00800000);  // the smallest non denormalized float number
static const int4 _pi32_mant_mask(0x7f800000);
static const int4 _pi32_inv_mant_mask(~0x7f800000);
static const int4 _pi32_sign_mask((int32_t)0x80000000);
static const int4 _pi32_inv_sign_mask(~0x80000000);
static const int4 _pi32_1(1);
static const int4 _pi32_inv1(~1);
static const int4 _pi32_2(2);
static const int4 _pi32_4(4);
static const int4 _pi32_0x7f(0x7f);

// ----------------------------------------------------------------
// vecLog - natural logarithm

inline float4 log(float4 x) {
  int4 emm0;
  float4 one = _ps_1;
  float4 invalid_mask = (x <= setZero());
  
  x = max(x, reinterpretIntAsFloat(_pi32_min_norm_pos));
  
  emm0 = shiftRightElements(reinterpretFloatAsInt(x), 23);
  
  x = andBits(x, reinterpretIntAsFloat(_pi32_inv_mant_mask));
  x = orBits(x, _ps_0p5);
  
  emm0 = emm0 - _pi32_0x7f;
  float4 e = intToFloat(emm0);
  
  e = e + one;
  
  float4 mask = (x < _ps_cephes_SQRTHF);
  float4 tmp = andBits(x, mask);
  x = x - one;
  e = e - andBits(one, mask);
  x = x + tmp;
  
  float4 z = x * x;
  
  float4 y = _ps_cephes_log_p0;
  y = y * x;
  y = y + _ps_cephes_log_p1;
  y = y * x;
  y = y + _ps_cephes_log_p2;
  y = y * x;
  y = y + _ps_cephes_log_p3;
  y = y * x;
  y = y + _ps_cephes_log_p4;
  y = y * x;
  y = y + _ps_cephes_log_p5;
  y = y * x;
  y = y + _ps_cephes_log_p6;
  y = y * x;
  y = y + _ps_cephes_log_p7;
  y = y * x;
  y = y + _ps_cephes_log_p8;
  y = y * x;
  
  y = y * z;
  
  tmp = e * _ps_cephes_log_q1;
  y = y + tmp;
  
  tmp = z * _ps_0p5;
  y = y - tmp;
  
  tmp = e * _ps_cephes_log_q2;
  x = x + y;
  x = x + tmp;
  x = orBits(x, invalid_mask);
  return x;
}

// ----------------------------------------------------------------
// exp - exponential

inline float4 exp(float4 x) {
  float4 tmp = setZero(), fx;
  int4 emm0;
  float4 one = _ps_1;
  
  x = min(x, _ps_exp_hi);
  x = max(x, _ps_exp_lo);
  
  fx = x * _ps_cephes_LOG2EF;
  fx = fx + _ps_0p5;
  
  emm0 = floatToIntTruncate(fx);
  tmp = intToFloat(emm0);
  
  float4 mask = (tmp > fx);
  mask = andBits(mask, one);
  fx = tmp - mask;
  
  tmp = fx * _ps_cephes_exp_C1;
  float4 z = fx * _ps_cephes_exp_C2;
  x = x - tmp;
  x = x - z;
  z = x * x;
  
  float4 y = _ps_cephes_exp_p0;
  y = y * x;
  y = y + _ps_cephes_exp_p1;
  y = y * x;
  y = y + _ps_cephes_exp_p2;
  y = y * x;
  y = y + _ps_cephes_exp_p3;
  y = y * x;
  y = y + _ps_cephes_exp_p4;
  y = y * x;
  y = y + _ps_cephes_exp_p5;
  y = y * z;
  y = y + x;
  y = y + one;
  
  emm0 = floatToIntTruncate(fx);
  emm0 = emm0 + _pi32_0x7f;
  emm0 = shiftLeftElements(emm0, 23);
  float4 pow2n = reinterpretIntAsFloat(emm0);
  
  y = y * pow2n;
  return y;
}

// ----------------------------------------------------------------
// vecSin - sine

inline float4 sin(float4 x) {
  float4 xmm1, xmm2 = setZero(), xmm3, sign_bit, y;
  int4 emm0, emm2;
  
  sign_bit = x;
  x = andBits(x, reinterpretIntAsFloat(_pi32_inv_sign_mask));
  sign_bit = andBits(sign_bit, reinterpretIntAsFloat(_pi32_sign_mask));
  
  y = x * _ps_cephes_FOPI;
  
  emm2 = floatToIntTruncate(y);
  emm2 = emm2 + _pi32_1;
  emm2 = andBits(emm2, _pi32_inv1);
  y = intToFloat(emm2);
  
  emm0 = andBits(emm2, _pi32_4);
  emm0 = shiftLeftElements(emm0, 29);
  
  emm2 = andBits(emm2, _pi32_2);
  emm2 = compareEqualInt(emm2, setZeroInt());
  
  float4 swap_sign_bit = reinterpretIntAsFloat(emm0);
  float4 poly_mask = reinterpretIntAsFloat(emm2);
  sign_bit = xorBits(sign_bit, swap_sign_bit);
  
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = y * xmm1;
  xmm2 = y * xmm2;
  xmm3 = y * xmm3;
  x = x + xmm1;
  x = x + xmm2;
  x = x + xmm3;
  
  y = _ps_coscof_p0;
  float4 z = x * x;
  
  y = y * z;
  y = y + _ps_coscof_p1;
  y = y * z;
  y = y + _ps_coscof_p2;
  y = y * z;
  y = y * z;
  float4 tmp = z * _ps_0p5;
  y = y - tmp;
  y = y + _ps_1;
  
  float4 y2 = _ps_sincof_p0;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p1;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p2;
  y2 = y2 * z;
  y2 = y2 * x;
  y2 = y2 + x;
  
  xmm3 = poly_mask;
  y2 = andBits(xmm3, y2);
  y = andNotBits(xmm3, y);
  y = y + y2;
  y = xorBits(y, sign_bit);
  return y;
}

// ----------------------------------------------------------------
// cos - cosine

inline float4 cos(float4 x) {
  float4 xmm1, xmm2 = setZero(), xmm3, y;
  int4 emm0, emm2;
  
  x = andBits(x, reinterpretIntAsFloat(_pi32_inv_sign_mask));
  
  y = x * _ps_cephes_FOPI;
  
  emm2 = floatToIntTruncate(y);
  emm2 = emm2 + _pi32_1;
  emm2 = andBits(emm2, _pi32_inv1);
  y = intToFloat(emm2);
  emm2 = emm2 - _pi32_2;
  
  emm0 = andNotBits(emm2, _pi32_4);
  emm0 = shiftLeftElements(emm0, 29);
  
  emm2 = andBits(emm2, _pi32_2);
  emm2 = compareEqualInt(emm2, setZeroInt());
  
  float4 sign_bit = reinterpretIntAsFloat(emm0);
  float4 poly_mask = reinterpretIntAsFloat(emm2);
  
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = y * xmm1;
  xmm2 = y * xmm2;
  xmm3 = y * xmm3;
  x = x + xmm1;
  x = x + xmm2;
  x = x + xmm3;
  
  y = _ps_coscof_p0;
  float4 z = x * x;
  
  y = y * z;
  y = y + _ps_coscof_p1;
  y = y * z;
  y = y + _ps_coscof_p2;
  y = y * z;
  y = y * z;
  float4 tmp = z * _ps_0p5;
  y = y - tmp;
  y = y + _ps_1;
  
  float4 y2 = _ps_sincof_p0;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p1;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p2;
  y2 = y2 * z;
  y2 = y2 * x;
  y2 = y2 + x;
  
  xmm3 = poly_mask;
  y2 = andBits(xmm3, y2);
  y = andNotBits(xmm3, y);
  y = y + y2;
  y = xorBits(y, sign_bit);
  
  return y;
}

// ----------------------------------------------------------------
// vecSinCos - simultaneous sine and cosine

inline std::pair<float4, float4> sincos(float4 x) {
  float4 xmm1, xmm2, xmm3 = setZero(), sign_bit_sin, y;
  int4 emm0, emm2, emm4;
  
  sign_bit_sin = x;
  x = andBits(x, reinterpretIntAsFloat(_pi32_inv_sign_mask));
  sign_bit_sin = andBits(sign_bit_sin, reinterpretIntAsFloat(_pi32_sign_mask));
  
  y = x * _ps_cephes_FOPI;
  
  emm2 = floatToIntTruncate(y);
  
  emm2 = emm2 + _pi32_1;
  emm2 = andBits(emm2, _pi32_inv1);
  y = intToFloat(emm2);
  
  emm4 = emm2;
  
  emm0 = andBits(emm2, _pi32_4);
  emm0 = shiftLeftElements(emm0, 29);
  float4 swap_sign_bit_sin = reinterpretIntAsFloat(emm0);
  
  emm2 = andBits(emm2, _pi32_2);
  emm2 = compareEqualInt(emm2, setZeroInt());
  float4 poly_mask = reinterpretIntAsFloat(emm2);
  
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = y * xmm1;
  xmm2 = y * xmm2;
  xmm3 = y * xmm3;
  x = x + xmm1;
  x = x + xmm2;
  x = x + xmm3;
  
  emm4 = emm4 - _pi32_2;
  emm4 = andNotBits(emm4, _pi32_4);
  emm4 = shiftLeftElements(emm4, 29);
  float4 sign_bit_cos = reinterpretIntAsFloat(emm4);
  
  sign_bit_sin = xorBits(sign_bit_sin, swap_sign_bit_sin);
  
  float4 z = x * x;
  y = _ps_coscof_p0;
  
  y = y * z;
  y = y + _ps_coscof_p1;
  y = y * z;
  y = y + _ps_coscof_p2;
  y = y * z;
  y = y * z;
  float4 tmp = z * _ps_0p5;
  y = y - tmp;
  y = y + _ps_1;
  
  float4 y2 = _ps_sincof_p0;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p1;
  y2 = y2 * z;
  y2 = y2 + _ps_sincof_p2;
  y2 = y2 * z;
  y2 = y2 * x;
  y2 = y2 + x;
  
  xmm3 = poly_mask;
  float4 ysin2 = andBits(xmm3, y2);
  float4 ysin1 = andNotBits(xmm3, y);
  y2 = y2 - ysin2;
  y = y - ysin1;
  
  xmm1 = ysin1 + ysin2;
  xmm2 = y + y2;
  
  float4 s = xorBits(xmm1, sign_bit_sin);
  float4 c = xorBits(xmm2, sign_bit_cos);
  return {s, c};
}

// ----------------------------------------------------------------
// Fast polynomial approximations


constexpr float kSinC1 = 0.99997937679290771484375f;
constexpr float kSinC2 = -0.166624367237091064453125f;
constexpr float kSinC3 = 8.30897875130176544189453125e-3f;
constexpr float kSinC4 = -1.92649182281456887722015380859375e-4f;
constexpr float kSinC5 = 2.147840177713078446686267852783203125e-6f;

constexpr float kCosC1 = 0.999959766864776611328125f;
constexpr float kCosC2 = -0.4997930824756622314453125f;
constexpr float kCosC3 = 4.1496001183986663818359375e-2f;
constexpr float kCosC4 = -1.33926304988563060760498046875e-3f;
constexpr float kCosC5 = 1.8791708498611114919185638427734375e-5f;

constexpr float kTanhN = 27.0f;
constexpr float kTanhD = 9.0f;

// ----------------------------------------------------------------
// Pure polynomial approximations — template directly

// sin(x) for x in [-pi, pi]: max error about 6e-6. Diverges quickly outside that range, so wrap first.
template<typename T>
inline T sinApprox(T x) {
  T x2 = x * x;
  return x * (T{kSinC1} + (x2 * (T{kSinC2} + (x2 * (T{kSinC3} + (x2 * (T{kSinC4} + (x2 * T{kSinC5}))))))));
}

// cos(x) for x in [-pi, pi]: max error about 4e-5. Diverges quickly outside that range, so wrap first.
template<typename T>
inline T cosApprox(T x) {
  T x2 = x * x;
  return T{kCosC1} + (x2 * (T{kCosC2} + (x2 * (T{kCosC3} + (x2 * (T{kCosC4} + (x2 * T{kCosC5})))))));
}

// tanh(x) for x in [-3, 3]: max error about 0.025, reaching exactly +/-1 at +/-3. Unbounded beyond, so clamp first.
template<typename T>
inline T tanhApprox(T x) {
  T x2 = x * x;
  return x * (T{kTanhN} + x2) / (T{kTanhN} + T{kTanhD} * x2);
}

// ----------------------------------------------------------------
// Scalar bit-manipulation primitives (match the float4/int4 versions)

inline int32_t reinterpretFloatAsInt(float x) {
  int32_t i; std::memcpy(&i, &x, 4); return i;
}

inline float reinterpretIntAsFloat(int32_t i) {
  float x; std::memcpy(&x, &i, 4); return x;
}

inline int32_t floatToIntTruncate(float x) { return static_cast<int32_t>(x); }
inline float intToFloat(int32_t x) { return static_cast<float>(x); }

inline int32_t shiftRightElements(int32_t a, int count) { return static_cast<int32_t>(static_cast<uint32_t>(a) >> count); }
inline int32_t shiftLeftElements(int32_t a, int count) { return a << count; }

inline int32_t andBits(int32_t a, int32_t b) { return a & b; }
inline int32_t orBits(int32_t a, int32_t b) { return a | b; }
inline float andBits(float a, float b) { return reinterpretIntAsFloat(reinterpretFloatAsInt(a) & reinterpretFloatAsInt(b)); }
inline float orBits(float a, float b) { return reinterpretIntAsFloat(reinterpretFloatAsInt(a) | reinterpretFloatAsInt(b)); }

constexpr float kExpC1 = 2139095040.f;
constexpr float kExpC2 = 12102203.1615614f;
constexpr float kExpC3 = 1065353216.f;
constexpr float kExpC4 = 0.510397365625862338668154f;
constexpr float kExpC5 = 0.310670891004095530771135f;
constexpr float kExpC6 = 0.168143436463395944830000f;
constexpr float kExpC7 = -2.88093587581985443087955e-3f;
constexpr float kExpC8 = 1.3671023382430374383648148e-2f;

constexpr float kLogC1 = -89.970756366f;
constexpr float kLogC2 = 3.529304993f;
constexpr float kLogC3 = -2.461222105f;
constexpr float kLogC4 = 1.130626167f;
constexpr float kLogC5 = -0.288739945f;
constexpr float kLogC6 = 3.110401639e-2f;
constexpr float kLogC7 = 0.69314718055995f;

template<typename T>
inline T expApprox(T x) {
  using I = std::conditional_t<std::is_same_v<T, float>, int32_t, int4>;
  
  T val2 = x * T{kExpC2} + T{kExpC3};
  T val3 = min(val2, T{kExpC1});
  T val4 = max(val3, T{0.0f});
  I val4i = floatToIntTruncate(val4);
  
  T xu = reinterpretIntAsFloat(andBits(val4i, I{0x7F800000}));
  T b = reinterpretIntAsFloat(orBits(andBits(val4i, I{0x7FFFFF}), I{0x3F800000}));
  
  return xu * (T{kExpC4} + (b * (T{kExpC5} + (b * (T{kExpC6} + (b * (T{kExpC7} + (b * T{kExpC8}))))))));
}

template<typename T>
inline T logApprox(T x) {
  using I = std::conditional_t<std::is_same_v<T, float>, int32_t, int4>;
  
  I valAsInt = reinterpretFloatAsInt(x);
  I expi = shiftRightElements(valAsInt, 23);
  
  I valMasked = orBits(andBits(valAsInt, I{0x7FFFFF}), I{0x3F800000});
  T xm = reinterpretIntAsFloat(valMasked);
  
  T poly = xm * (T{kLogC2} + (xm * (T{kLogC3} + (xm * (T{kLogC4} + (xm * (T{kLogC5} + (xm * T{kLogC6}))))))));
  
  T addcst;
  if constexpr (std::is_same_v<T, float>)
  {
    addcst = x > 0.0f ? kLogC1 : FLT_MIN;
  }
  else
  {
    addcst = select(T{kLogC1}, T{FLT_MIN}, (x > T{0.0f}));
  }
  
  return poly + addcst + T{kLogC7} * intToFloat(expi);
}
