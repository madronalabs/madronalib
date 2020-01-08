// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/base/ustring.h"
#include <list>
#include <algorithm>

namespace Steinberg {
namespace Vst {
namespace ml {

//-----------------------------------------------------------------------------
class BaseParameter : public Parameter
{
public:
	BaseParameter	(	const TChar* title, 
						const TChar* units, 
						int32 stepCount, 
						ParamValue defaultValueNormalized,
						int32 flags,
						int32 tag,
						UnitID unitID = kRootUnitId);

	bool fromString (const TChar* string, ParamValue& _valueNormalized) const SMTG_OVERRIDE;
	bool setNormalized (ParamValue v) SMTG_OVERRIDE;

};

//-----------------------------------------------------------------------------
class IndexedParameter : public BaseParameter
{
public:
	IndexedParameter (	const TChar* title, 
						const TChar* units, 
						int32 stepCount, 
						ParamValue defaultValueNormalized,
						int32 flags,
						int32 tag,
						UnitID unitID = kRootUnitId);

	ParamValue toPlain (ParamValue _valueNormalized) const SMTG_OVERRIDE;
	ParamValue toNormalized (ParamValue plainValue) const SMTG_OVERRIDE;

	void toString (ParamValue _valueNormalized, String128 string) const SMTG_OVERRIDE;
	bool fromString (const TChar* string, ParamValue& _valueNormalized) const SMTG_OVERRIDE;

	void setIndexString (int32 index, const String128 str);
protected:
	~IndexedParameter ();
	String128* indexString;
};

//-----------------------------------------------------------------------------
class ScaledParameter : public BaseParameter
{
public:
	ScaledParameter (	const TChar* title, 
						const TChar* units, 
						int32 stepCount, 
						ParamValue defaultValueNormalized,
						int32 flags,
						int32 tag,
						ParamValue minValue = 0.,
						ParamValue maxValue = 1.,
						bool printAsInteger = false,
						UnitID unitID = kRootUnitId);

	ParamValue toPlain (ParamValue _valueNormalized) const SMTG_OVERRIDE;
	ParamValue toNormalized (ParamValue plainValue) const SMTG_OVERRIDE;

	void toString (ParamValue _valueNormalized, String128 string) const SMTG_OVERRIDE;
	bool fromString (const TChar* string, ParamValue& _valueNormalized) const SMTG_OVERRIDE;
protected:
	ParamValue minValue;
	ParamValue maxValue;
	bool printAsInteger;
};

}}} // namespaces
