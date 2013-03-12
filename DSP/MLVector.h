
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_VECTOR__
#define __ML_VECTOR__

#define MLVECTOR_SSE 0
 
// vector classes for small objects with fixed sizes, where the overhead
// of MLSignals would be too big. 

#include "MLDSP.h"

#if MLVECTOR_SSE

typedef union 
{
    __m128 v;
	float f[4];
} MLV4;

#else

typedef struct 
{
	float f[4];
}	MLV4;

#endif

class MLVec
{
public:
	MLVec() { clear(); }
	MLVec(const float f) { set(f); }
	
	virtual ~MLVec(){};

#if MLVECTOR_SSE
	
	MLVec(const MLVec& b) : val(b.val) {}
	MLVec(const float a, const float b, const float c, const float d)
		{ val.f[0] = a; val.f[1] = b; val.f[2] = c; val.f[3] = d; }
	inline MLVec(__m128 vk) { val.v = vk; } 
	inline void clear() { val.v = _mm_setzero_ps(); }
	inline void set(float f) { val.v = _mm_set1_ps(f); }
	inline MLVec & operator+=(const MLVec& b) { val.v = _mm_add_ps(val.v, b.val.v); return *this; }
	inline MLVec & operator-=(const MLVec& b) { val.v = _mm_sub_ps(val.v, b.val.v); return *this; }	
	inline MLVec & operator*=(const MLVec& b) { val.v = _mm_mul_ps(val.v, b.val.v); return *this; }
	inline MLVec & operator/=(const MLVec& b) { val.v = _mm_div_ps(val.v, b.val.v); return *this; }
	inline const MLVec operator-() const { return MLVec(_mm_sub_ps(_mm_setzero_ps(), val.v)); }

#else

	MLVec(const MLVec& b) { val = b.val; }
	MLVec(const float a, const float b, const float c, const float d)
		{ val.f[0] = a; val.f[1] = b; val.f[2] = c; val.f[3] = d; }
	inline void clear()
		{ val.f[0] = val.f[1] = val.f[2] = val.f[3] = 0.f; }
	inline void set(float f) 
		{ val.f[0] = val.f[1] = val.f[2] = val.f[3] = f; }
	inline MLVec & operator+=(const MLVec& b) 
		{ val.f[0]+=b.val.f[0]; val.f[1]+=b.val.f[1]; val.f[2]+=b.val.f[2]; val.f[3]+=b.val.f[3]; 
		return *this; }
	inline MLVec & operator-=(const MLVec& b) 
		{ val.f[0]-=b.val.f[0]; val.f[1]-=b.val.f[1]; val.f[2]-=b.val.f[2]; val.f[3]-=b.val.f[3]; 
		return *this; }
	inline MLVec & operator*=(const MLVec& b) 
		{ val.f[0]*=b.val.f[0]; val.f[1]*=b.val.f[1]; val.f[2]*=b.val.f[2]; val.f[3]*=b.val.f[3]; 
		return *this; }
	inline MLVec & operator/=(const MLVec& b)
		{ val.f[0]/=b.val.f[0]; val.f[1]/=b.val.f[1]; val.f[2]/=b.val.f[2]; val.f[3]/=b.val.f[3]; 
		return *this; }
	inline const MLVec operator-() const 
		{ return MLVec(-val.f[0], -val.f[1], -val.f[2], -val.f[3]); }

#endif

	// inspector, return by value
	inline float operator[] (unsigned i) const { return val.f[i]; }
	// mutator, return by reference
	inline float& operator[] (unsigned i) { return val.f[i]; }

	bool operator==(const MLVec& b) const;
	bool operator!=(const MLVec& b) const;
 
	inline const MLVec operator+ (const MLVec& b) const { return MLVec(*this) += b; }
	inline const MLVec operator- (const MLVec& b) const { return MLVec(*this) -= b; }
	inline const MLVec operator* (const MLVec& b) const { return MLVec(*this) *= b; }
	inline const MLVec operator/ (const MLVec& b) const { return MLVec(*this) /= b; }

	inline MLVec & operator*=(const float f) { (*this) *= MLVec(f); return *this; }
	inline const MLVec operator* (const float f) const { return MLVec(*this) *= f; }

	virtual float magnitude() const;
	void normalize();
	MLVec getIntPart() const;
	MLVec getFracPart() const;
	void getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const;

	MLV4 val;
};


#if MLVECTOR_SSE

inline const MLVec vmin(const MLVec&a, const MLVec&b) { return MLVec(_mm_min_ps(a.val.v, b.val.v)); }
inline const MLVec vmax(const MLVec&a, const MLVec&b) { return MLVec(_mm_max_ps(a.val.v, b.val.v)); }
inline const MLVec vclamp(const MLVec&a, const MLVec&b, const MLVec&c) { return vmin(c, vmax(a, b)); }
inline const MLVec vsqrt(const MLVec& a) { return MLVec(_mm_sqrt_ps(a.val.v)); }

#else

inline const MLVec vmin(const MLVec&a, const MLVec&b) 
	{ return MLVec(min(a.val.f[0],b.val.f[0]),min(a.val.f[1],b.val.f[1]),
		min(a.val.f[2],b.val.f[2]),min(a.val.f[3],b.val.f[3])); }
inline const MLVec vmax(const MLVec&a, const MLVec&b) 
	{ return MLVec(max(a.val.f[0],b.val.f[0]),max(a.val.f[1],b.val.f[1]),
		max(a.val.f[2],b.val.f[2]),max(a.val.f[3],b.val.f[3])); }
inline const MLVec vclamp(const MLVec&a, const MLVec&b, const MLVec&c) { return vmin(c, vmax(a, b)); }
inline const MLVec vsqrt(const MLVec& a) 
	{ return MLVec(sqrt(a.val.f[0]), sqrt(a.val.f[1]), sqrt(a.val.f[2]), sqrt(a.val.f[3])); }
inline const MLVec vlerp(const MLVec& a, const MLVec&b, const float m) 
	{ return a + MLVec(m)*(b - a); }
	

#endif

class Vec2 : public MLVec 
{
public:
	Vec2() : MLVec() {}
	Vec2(const MLVec& b) : MLVec(b) {};
	Vec2(float px, float py) { val.f[0] = px; val.f[1] = py; val.f[2] = 0.; val.f[3] = 0.; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	float magnitude() const;
};

class Vec3 : public MLVec 
{
public:
	Vec3() : MLVec() {}
	Vec3(const MLVec& b) : MLVec(b) {};
	Vec3(float px, float py, float pz) { val.f[0] = px; val.f[1] = py; val.f[2] = pz; val.f[3] = 0.; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	float z() const { return val.f[2]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	void setZ(float f) { val.f[2] = f; }
	float magnitude() const;
};

class Vec4 : public MLVec 
{
public:
	Vec4() : MLVec() {}
	Vec4(const MLVec& b) : MLVec(b) {};
	Vec4(float px, float py, float pz, float pw) { val.f[0] = px; val.f[1] = py; val.f[2] = pz; val.f[3] = pw; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	float z() const { return val.f[2]; }
	float w() const { return val.f[3]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	void setZ(float f) { val.f[2] = f; }
	void setW(float f) { val.f[3] = f; }
	float magnitude() const;
};

// rectangle stored in left / top / width / height format.
class MLRect : public MLVec 
{
public:
	MLRect() : MLVec() {}
	MLRect(const MLVec& b) : MLVec(b) {};
	MLRect(float x, float y, float width, float height); 
	MLRect(const Vec2& corner1, const Vec2& corner2);
	inline float left() const { return val.f[0]; }
	float top() const { return val.f[1]; }	
	float right() const { return val.f[0] + val.f[2]; }
	float bottom() const { return val.f[1] + val.f[3]; }	
	float width() const { return val.f[2]; }
	float height() const { return val.f[3]; }
	
	inline float area() const { return width()*height(); }
	inline bool contains(const Vec2& p) const { return (within(p.x(), left(), right()) && within(p.y(), top(), bottom())); }
	MLRect intersect(const MLRect& b) const;
	MLRect unionWith(const MLRect& b) const;
	bool intersects(const MLRect& p) const;

	void setToIntersectionWith(const MLRect& b); 
	void setToUnionWith(const MLRect& b); 
	
	inline void setLeft(float px){ val.f[0] = px; }
	inline void setTop(float py){ val.f[1] = py; }
	inline void setWidth(float w){ val.f[2] = w; }
	inline void setHeight(float h){ val.f[3] = h; }
	
	inline void setRight(float px){ val.f[0] = px - val.f[2]; }
	inline void setBottom(float py){ val.f[1] = py - val.f[3]; }
	void translate(const Vec2& b);
	void setCenter(const Vec2& b);

	inline void stretchWidth(float d){ val.f[0] -= d*0.5; val.f[2] += d; }	
	inline void stretchHeight(float d){ val.f[1] -= d*0.5; val.f[3] += d; }
	
	inline void stretchWidthTo(float w){ float d = w - width(); stretchWidth(d); }
	inline void stretchHeightTo(float h){ float d = h - height(); stretchHeight(d); }	
	
	inline void expand(float d){ stretchWidth(d); stretchHeight(d); }
	inline void expand(const Vec2& b){ stretchWidth(b.x()); stretchHeight(b.y()); }
	
	MLRect translated(const Vec2& b) const;
	MLRect withCenter(const Vec2& b) const;
	MLRect withCenter(const float cx, const float cy);

	Vec2 getCenter() const;
	Vec2 getSize() const;
	Vec2 getTopLeft() const;
	Vec2 getBottomRight() const;
	
	// JUCE adapters
	inline bool contains(int px, int py) const { return (within(px, (int)left(), (int)right()) && within(py, (int)top(), (int)bottom())); }
	inline void setBounds(int l, int t, int w, int h) { *this = MLRect(l, t, w, h); }
	inline int x() const { return left(); }
	inline int y() const { return top(); }
	inline int getWidth() const { return width(); }
	inline int getHeight() const { return height(); }
};

std::ostream& operator<< (std::ostream& out, const Vec2& r);
std::ostream& operator<< (std::ostream& out, const Vec3& r);
std::ostream& operator<< (std::ostream& out, const Vec4& r);
std::ostream& operator<< (std::ostream& out, const MLRect& r);

#define MLPoint Vec2

#endif // __ML_VECTOR__

