#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include <shlobj.h>

#include "PapyrusFunctions.h"

IDebugLog gLog;

PluginHandle pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface * papyrus = nullptr;
//F4SEMessagingInterface * messaging = nullptr;

extern "C"
{
    bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
    {
        //gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);
        gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\lootman.log");

        _MESSAGE("lootman");

        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = "Lootman";
        info->version = 11016301;

        if(f4se->isEditor)
        {
            _FATALERROR("loaded in editor, marking as incompatible");
            return false;
        }
        else if(f4se->runtimeVersion != RUNTIME_VERSION_1_10_163)
        {
            _FATALERROR("unsupported runtime version: %08X", f4se->runtimeVersion);
            return false;
        }

        papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
        if(!papyrus)
        {
            _FATALERROR("couldn't get papyrus interface");
            return false;
        }

        /*
        pluginHandle = f4se->GetPluginHandle();
        messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
        if(!messaging)
        {
            _FATALERROR("couldn't get messaging interface");
            return false;
        }
        */

        return true;
    }

    bool F4SEPlugin_Load(const F4SEInterface * f4se)
    {
        if(papyrus)
            papyrus->Register(papyrusFunctions::RegisterFuncs);

        /*
        if(messaging)
            messaging->RegisterListener(pluginHandle, "F4SE", papyrusFunctions::Messaging);
        */

        _DMESSAGE("Lootman plugin is loaded.");
        return true;
    }
}