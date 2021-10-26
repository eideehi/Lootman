#include "FormIDCache.h"

#include "f4se/GameRTTI.h"
#include "f4se/GameReferences.h"

EventResult ObjectLoadedListener::ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher)
{
    TESForm * form = LookupFormByID(evn->formId);
    if(!form)
    {
        return kEvent_Continue;
    }

    TESObjectREFR * ref = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
    if(ref)
    {
        UInt8 formType = ref->baseForm->formType;
        if(formType == FormType::kFormType_ACTI ||
           formType == FormType::kFormType_ALCH ||
           formType == FormType::kFormType_AMMO ||
           formType == FormType::kFormType_ARMO ||
           formType == FormType::kFormType_BOOK ||
           formType == FormType::kFormType_CONT ||
           formType == FormType::kFormType_FLOR ||
           formType == FormType::kFormType_INGR ||
           formType == FormType::kFormType_KEYM ||
           formType == FormType::kFormType_MISC ||
           formType == FormType::kFormType_NPC_ ||
           formType == FormType::kFormType_WEAP)
        {
            TESObjectCELL * cell = ref->parentCell;
            if(cell)
            {
                SimpleLocker locker(&FormIDCache::lock);
                if(evn->loaded)
                {
                    FormIDCache::cells.insert(cell->formID);
                }
            }
        }
    }

    return kEvent_Continue;
}

namespace FormIDCache
{
    ObjectLoadedListener eventListener;

    SimpleLock lock;
    std::unordered_set<UInt32> cells;
}
