#pragma once

#ifdef _DEBUG

#include "common/ITypes.h"

#include "f4se/GameForms.h"
#include "f4se/GameExtraData.h"


const char * _FlagsToBinaryString(UInt64 flags);

const char * _FlagsToBinaryString(UInt32 flags);

const char * _FlagsToBinaryString(UInt16 flags);

const char * _FlagsToBinaryString(SInt32 flags);

const char * _FormTypeToString(UInt8 formType);

void _TraceExtraDataList(const char * processId, ExtraDataList * extraDataList, int indent);

void _TraceTESForm(const char * processId, TESForm * form, int indent);

void _TraceTESObjectCELL(const char * processId, TESObjectCELL * cell, int indent);

void _TraceTESObjectREFR(const char * processId, TESObjectREFR * ref, int indent);

void _TraceBGSInventoryItem(const char * processId, BGSInventoryItem * item, int indent);

void _TraceReferenceFlags(const char * processId, TESObjectREFR * ref, int indent);

const char * _GetRandomProcessID();

#endif
