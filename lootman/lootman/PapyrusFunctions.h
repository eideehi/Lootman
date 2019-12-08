#pragma once

class VirtualMachine;
struct StaticFunctionTag;

#include "f4se/PluginAPI.h"
#include "f4se/GameTypes.h"

namespace papyrusFunctions
{
    bool RegisterFuncs(VirtualMachine * vm);
    //void Messaging(F4SEMessagingInterface::Message * msg);
}
