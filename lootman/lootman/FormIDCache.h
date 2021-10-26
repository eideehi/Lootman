#pragma once

#include <unordered_set>

#include "f4se/GameEvents.h"

class ObjectLoadedListener : public BSTEventSink<TESObjectLoadedEvent>
{
public:
    virtual EventResult ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher);
};

namespace FormIDCache
{
    extern ObjectLoadedListener eventListener;

    extern SimpleLock lock;
    extern std::unordered_set<UInt32> cells;
}
