#include <shlobj.h>

#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include "Globals.h"
#include "PapyrusLootman.h"

IDebugLog gLog;

PluginHandle pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface * papyrus = nullptr;

extern "C"
{
    bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
    {
        gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\lootman.log");

        _MESSAGE(">> Lootman plugin query phase start.");

        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = "Lootman";
        info->version = 11016307;// FO4 1.10.163-07

        if(f4se->isEditor)
        {
            _FATALERROR(">>   Loaded in editor, marking as incompatible.");
            return false;
        }

        if(f4se->runtimeVersion != RUNTIME_VERSION_1_10_163)
        {
            _FATALERROR(">>   Unsupported runtime version: %08X", f4se->runtimeVersion);
            return false;
        }

        papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
        if(!papyrus)
        {
            _FATALERROR(">>   Couldn't get papyrus interface");
            return false;
        }

        _MESSAGE(">> Lootman plugin query phase end.");
        return true;
    }

    bool F4SEPlugin_Load(const F4SEInterface * f4se)
    {
        _MESSAGE(">> Lootman plugin load phase start.");

        if(!Globals::Initialize())
        {
            _FATALERROR(">>   Failed to initialization for globals.");
            return false;
        }

        if(!papyrus)
        {
            _FATALERROR(">>   Couldn't get papyrus interface.");
            return false;
        }

        if(!papyrus->Register(PapyrusLootman::RegisterFuncs))
        {
            _FATALERROR(">>   Failed to register papyrus functions.");
            return false;
        }

        _MESSAGE(">> Lootman plugin load phase end.");
        return true;
    }
}
