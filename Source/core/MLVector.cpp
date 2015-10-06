
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLVector.h"

#if MLVECTOR_SSE

MLVec MLVec::getIntPart() const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	return MLVec(_mm_cvtepi32_ps(vi));
}

MLVec MLVec::getFracPart() const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	__m128 intPart = _mm_cvtepi32_ps(vi);
	return MLVec(_mm_sub_ps(val.v, intPart));
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	intPart = _mm_cvtepi32_ps(vi);
	fracPart = _mm_sub_ps(val.v, intPart.val.v);
}

#else

MLVec MLVec::getIntPart() const
{
	return MLVec((int)val.f[0],(int)val.f[1],(int)val.f[2],(int)val.f[3]);
}

MLVec MLVec::getFracPart() const
{
	return *this - getIntPart();
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
	MLVec ip = getIntPart();
	intPart = ip;
	fracPart = *this - ip;
}

#endif

bool MLVec::operator==(const MLVec& b) const
{	
	return (val.f[0] == b.val.f[0]) && (val.f[1] == b.val.f[1]) && (val.f[2] == b.val.f[2]) && (val.f[3] == b.val.f[3]);  
}

bool MLVec::operator!=(const MLVec& b) const
{	
	return !operator==(b);  
}

void MLVec::normalize()
{
	float mag = magnitude();
	MLVec b;
	b.set(mag);
	(*this) *= b;
}

void MLVec::quantize(int q)
{
	int i0, i1, i2, i3;
	i0 = val.f[0];
	i1 = val.f[1];
	i2 = val.f[2];
	i3 = val.f[3];
	i0 = (i0/q)*q;
	i1 = (i1/q)*q;
	i2 = (i2/q)*q;
	i3 = (i3/q)*q;
	val.f[0] = i0;
	val.f[1] = i1;
	val.f[2] = i2;
	val.f[3] = i3;
}
float MLVec::magnitude() const
{
	float a = val.f[0];
	float b = val.f[1];
	float c = val.f[2];
	float d = val.f[3];
	return sqrtf(a*a + b*b + c*c + d*d);
}

float Vec2::magnitude() const
{
	float a = val.f[0];
	float b = val.f[1];
	return sqrtf(a*a + b*b);
}

float Vec3::magnitude() const
{
	float a = val.f[0];
	float b = val.f[1];
	float c = val.f[2];
	return sqrtf(a*a + b*b + c*c);
}

float Vec4::magnitude() const
{
	float a = val.f[0];
	float b = val.f[1];
	float c = val.f[2];
	float d = val.f[3];
	return sqrtf(a*a + b*b + c*c + d*d);
}


//
#pragma mark MLRect

MLRect::MLRect(float px, float py, float w, float h) 
{ 
	val.f[0] = px; 
	val.f[1] = py; 
	val.f[2] = w; 
	val.f[3] = h; 
}

MLRect::MLRect(const Vec2& corner1, const Vec2& corner2) 
{ 
	float x1, x2, y1, y2;
	x1 = min(corner1.x(), corner2.x());
	x2 = max(corner1.x(), corner2.x());
	y1 = min(corner1.y(), corner2.y());
	y2 = max(corner1.y(), corner2.y());
	val.f[0] = x1; 
	val.f[1] = y1; 
	val.f[2] = x2 - x1; 
	val.f[3] = y2 - y1; 
}

MLRect MLRect::intersect(const MLRect& b) const
{
	MLRect ret;
	float l, r, t, bot;
	l = max(left(), b.left());
	r = min(right(), b.right());
	if (r > l)
	{
		t = max(top(), b.top());
		bot = min(bottom(), b.bottom());
		if (bot > t)
		{
			ret = MLRect(l, t, r - l, bot - t);
		}
	}
	return ret;
}

bool MLRect::intersects(const MLRect& b) const
{ 
	MLRect rx = intersect(b);
	return (rx.area() > 0);
}

MLRect MLRect::unionWith(const MLRect& b) const
{
	MLRect ret;
	if (area() > 0.)
	{
		float l, r, t, bot;
		l = min(left(), b.left());
		r = max(right(), b.right());

		t = min(top(), b.top());
		bot = max(bottom(), b.bottom());
		ret = MLRect(l, t, r - l, bot - t);
	}
	else
	{
		ret = b;
	}
	return ret;
}

void MLRect::setToIntersectionWith(const MLRect& b)
{
	*this = intersect(b);
}

void MLRect::setToUnionWith(const MLRect& b)
{
	*this = unionWith(b);
}

void MLRect::translate(const Vec2& b)
{
	*this += b;
}

void MLRect::setCenter(const Vec2& b)
{
	val.f[0] = b.val.f[0] - val.f[2]*0.5f;
	val.f[1] = b.val.f[1] - val.f[3]*0.5f;
}

void MLRect::centerInRect(const MLRect& b)
{
    setCenter(b.getCenter());
}

MLRect MLRect::translated(const Vec2& b) const
{
	return *this + b; 
}

MLRect MLRect::withCenter(const Vec2& b) const
{
	return (translated(b - Vec2(left() + width()*0.5f, top() + height()*0.5f))); 
}

MLRect MLRect::withCenter(const float cx, const float cy)
{
	return (translated(Vec2(cx - left() - width()*0.5f, cy - top() - height()*0.5f))); 
}

MLRect MLRect::withTopLeft(const Vec2& b) const
{
	return (MLRect(b.val.f[0], b.val.f[1], width(), height()));
}

MLRect MLRect::withTopLeft(const float cx, const float cy) 
{
	return (MLRect(cx, cy, width(), height()));
}


Vec2 MLRect::getCenter() const
{
	return Vec2(left() + width()*0.5f, top() + height()*0.5f);
}

Vec2 MLRect::getTopLeft() const
{
	return Vec2(left(), top());
}

Vec2 MLRect::getDims() const
{
	return Vec2(width(), height());
}

Vec2 MLRect::getBottomRight() const
{
	return Vec2(right(), bottom());
}

std::ostream& operator<< (std::ostream& out, const Vec2& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec3& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
	out << ", ";
	out << r.z();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec4& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
	out << ", ";
	out << r.z();
	out << ", ";
	out << r.w();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const MLRect& r)
{
	out << "[";
	out << r.left();
	out << ", ";
	out << r.top();
	out << ", ";
	out << r.width();
	out << ", ";
	out << r.height();
	out << "]";
	return out;
}
