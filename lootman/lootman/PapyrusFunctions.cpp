#include "PapyrusFunctions.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameData.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include <map>
#include <string>
#include <stdio.h>

#pragma warning(disable:4996)

std::map<std::string, int> gConfigInt;

/*
SimpleLock gLock;
tArray<TESObjectREFR *> gObjectContainer;

class TESObjectLoadedEventSink : public BSTEventSink<TESObjectLoadedEvent>
{
public:
    virtual EventResult ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher)
    {
        TESForm * form = LookupFormByID(evn->formId);
        if(!form)
            return kEvent_Continue;
        
        TESObjectREFR * ref = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
        if(ref)
        {
            if(evn->loaded == 0)
            {
                SimpleLocker locker(&gLock);

                for(int i = 0; i < gObjectContainer.count; i++)
                {
                    if(gObjectContainer[i] == ref)
                        gObjectContainer.Remove(i);
                }
            }
            else
            {
                SimpleLocker locker(&gLock);

                gObjectContainer.Insert(0, ref);
                //_DMESSAGE("Load object[name=%s, type=%03d]", CALL_MEMBER_FN(ref, GetReferenceName)(), ref->baseForm->formType);
            }
        }
        
        return kEvent_Continue;
    };
};
TESObjectLoadedEventSink objectLoadedShink;
*/

namespace papyrusFunctions
{
    bool _ExcludeItem(TESForm * form, VMArray<UInt32> excludeTypes)
    {
        if(!excludeTypes.IsNone())
        {
            UInt32 type;
            for(int i = 0; i < excludeTypes.Length(); i++)
            {
                excludeTypes.Get(&type, i);
                if(type > 0 && form->formType == type)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool _IncludeItem(TESForm * form, VMArray<UInt32> includeTypes)
    {
        if(includeTypes.IsNone())
            return true;

        UInt32 type;
        for(UInt32 i = 0; i < includeTypes.Length(); i++)
        {
            includeTypes.Get(&type, i);
            if(type > 0 && form->formType == type)
            {
                return true;
            }
        }
        return false;
    }

    VMArray<TESObjectREFR *> FindReferencesInRange(StaticFunctionTag *,
                                                   TESObjectREFR * ref,
                                                   UInt32 distance,
                                                   VMArray<UInt32> includeTypes,
                                                   VMArray<UInt32> excludeTypes)
    {
        VMArray<TESObjectREFR *> result;
        
        if(!ref)
            return result;

        /*
        NiPoint3 pos = ref->pos;

        SimpleLocker locker(&gLock);

        for(int i = 0; i < gObjectContainer.count; i++)
        {
            TESObjectREFR * entry = gObjectContainer[i];
            
            if(distance > 0)
            {
                NiPoint3 pos2 = ref->pos;
                float x = pos.x - pos2.x;
                float y = pos.y - pos2.y;
                float z = pos.z - pos2.z;
                
                if(std::sqrtf((x * x) + (y * y) + (z * z)) > distance)
                {
                    continue;
                }
            }
            
            TESForm * form = entry->baseForm;
            if(!form)
                continue;
            
            if(!_ExcludeItem(form, excludeTypes) && _IncludeItem(form, includeTypes))
            {
                result.Push(&entry);
            }
        }
        */

        
        TESObjectCELL * cell = ref->parentCell;
        if(!cell)
            return result;
        
        NiPoint3 pos = ref->pos;
        auto findReferences = [&](TESObjectCELL * cell)
        {
            for(int i = 0; i < cell->objectList.count; i++)
            {
                TESObjectREFR * entry = cell->objectList.entries[i];

                if(distance > 0)
                {
                    NiPoint3 pos2 = ref->pos;
                    float x = pos.x - pos2.x;
                    float y = pos.y - pos2.y;
                    float z = pos.z - pos2.z;

                    if(std::sqrtf((x * x) + (y * y) + (z * z)) > distance)
                    {
                        continue;
                    }
                }

                TESForm * form = entry->baseForm;
                if(!form)
                    continue;

                if(!_ExcludeItem(form, excludeTypes) && _IncludeItem(form, includeTypes))
                {
                    result.Push(&entry);
                }
            }
        };

        findReferences(cell);
        
        TESForm * form = LookupFormByID(cell->preVisCell);
        if(!form)
            return result;

        TESObjectCELL * preVisCell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
        if(preVisCell)
        {
            if((cell->flags & TESObjectCELL::kFlag_IsInterior) == (preVisCell->flags & TESObjectCELL::kFlag_IsInterior))
            {
                findReferences(preVisCell);
            }
        }
        

        return result;
    }

    VMArray<TESForm *> FindItemsInInventory(StaticFunctionTag *,
                                            TESObjectREFR * ref,
                                            VMArray<UInt32> includeTypes,
                                            VMArray<UInt32> excludeTypes)
    {
        VMArray<TESForm*> result;

        if(!ref)
            return result;

        BGSInventoryList * inventory = ref->inventoryList;
        if(inventory)
        {
            inventory->inventoryLock.LockForRead();

            for(int i = 0; i < inventory->items.count; i++)
            {
                BGSInventoryItem item;
                inventory->items.GetNthItem(i, item);
                TESForm * form = item.form;

                // プレイアブルではないアイテム（多分）は無視する
                if((form->flags & 4) != 0)
                    continue;

                // 鍵は自動取得すると、インベントリに表示されなくて受け取れないので無視する
                // TODO: 鍵を入れることができるコンテナのModがあったので、調査してみる
                if(form->formType == FormType::kFormType_KEYM)
                    continue;

                if(!_ExcludeItem(form, excludeTypes) && _IncludeItem(form, includeTypes))
                {
                    result.Push(&form);
                }
            }

            inventory->inventoryLock.Unlock();
        }

        return result;
    }

    VMArray<TESForm *> FindJunkOrModInInventory(StaticFunctionTag *, TESObjectREFR * ref, bool findJunk)
    {
        VMArray<TESForm*> result;

        if(!ref)
            return result;

        BGSInventoryList * inventory = ref->inventoryList;
        if(inventory)
        {
            inventory->inventoryLock.LockForRead();

            for(int i = 0; i < inventory->items.count; i++)
            {
                BGSInventoryItem item;
                inventory->items.GetNthItem(i, item);
                TESForm * form = item.form;

                // プレイアブルではないアイテム（多分）は無視する
                if((form->flags & 4) != 0)
                    continue;

                if(form->formType != FormType::kFormType_MISC)
                    continue;

                TESObjectMISC * misc = DYNAMIC_CAST(form, TESForm, TESObjectMISC);
                if((findJunk && misc->components) || (!findJunk && !misc->components))
                {
                    result.Push(&form);
                }
            }

            inventory->inventoryLock.Unlock();
        }

        return result;
    }

    // オブジェクトがワークショップとリンクしているか評価して返す
    // ロジックはPapyrusObjectReference.cppのAttachWireLatentから拝借
    bool IsLinkedToWorkshop(StaticFunctionTag *, TESObjectREFR * ref)
    {
        // ワークショップのキーワードを取得
        BGSKeyword * keyword = nullptr;
        BGSDefaultObject * workshopItemDefault = (*g_defaultObjectMap)->GetDefaultObject("WorkshopItem");
        if(workshopItemDefault)
        {
            keyword = DYNAMIC_CAST(workshopItemDefault->form, TESForm, BGSKeyword);
        }

        // ワークショップのキーワード取得に失敗
        if(!keyword)
            return false;

        // 取得したキーワードを使用して
        // オブジェクトにリンクしているワークショップ（と思われる）オブジェクトを取得
        TESObjectREFR * workshopRef = GetLinkedRef_Native(ref, keyword);
        if(!workshopRef)
            return false;

        // 取得したオブジェクトが本当にワークショップであるか確認
        BSExtraData* extraDataWorkshop = workshopRef->extraDataList->GetByType(ExtraDataType::kExtraData_WorkshopExtraData);
        if(!extraDataWorkshop)
            return false;

        return true;
    }

    // キー文字列に対応する、Lootmanのコンフィグ値を取得する
    UInt32 GetConfigInt(StaticFunctionTag *, BSFixedString key)
    {
        auto it = gConfigInt.find(key.c_str());
        if(it != gConfigInt.end())
            return it->second;

        return 0;
    }

    // Lootmanのコンフィグを、ファイルから読み込み反映する
    void LoadConfig(StaticFunctionTag *)
    {
        const char * configPath = ".\\Data\\F4SE\\Plugins\\lootman.ini";
        const char * sectionSettings = "settings";

        gConfigInt["hotkey_open_inevtnory_combination"] = GetPrivateProfileInt(sectionSettings, "hotkey_open_inevtnory_combination", 160, configPath);
        gConfigInt["hotkey_open_inevtnory_main"] = GetPrivateProfileInt(sectionSettings, "hotkey_open_inevtnory_main", 76, configPath);
        gConfigInt["hotkey_toggle_looting_combination"] = GetPrivateProfileInt(sectionSettings, "hotkey_toggle_looting_combination", 163, configPath);
        gConfigInt["hotkey_toggle_looting_main"] = GetPrivateProfileInt(sectionSettings, "hotkey_toggle_looting_main", 76, configPath);
        gConfigInt["looting_alch_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_alch_enabled", 1, configPath);
        gConfigInt["looting_ammo_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_ammo_enabled", 1, configPath);
        gConfigInt["looting_armo_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_armo_enabled", 1, configPath);
        gConfigInt["looting_cont_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_cont_enabled", 1, configPath);
        gConfigInt["looting_flor_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_flor_enabled", 1, configPath);
        gConfigInt["looting_ingr_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_ingr_enabled", 0, configPath);
        gConfigInt["looting_misc_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_misc_enabled", 1, configPath);
        gConfigInt["looting_npc__enabled"] = GetPrivateProfileInt(sectionSettings, "looting_npc__enabled", 1, configPath);
        gConfigInt["looting_range"] = GetPrivateProfileInt(sectionSettings, "looting_range", 800, configPath);
        gConfigInt["looting_weap_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_weap_enabled", 1, configPath);
    }

    // ランダムなプロセスID(10桁のランダムな16進数文字列)を生成して返す
    // デバッグログの処理単位IDとして使用する
    // ロジックは次のURLから拝借 # https://stackoverflow.com/questions/12110209/how-to-fill-a-string-with-random-hex-characters
    BSFixedString GetRandomProcessID(StaticFunctionTag *)
    {
        char str[11] = {0};
        const char * hex = "0123456789ABCDEF";
        for(int i = 0 ; i < 10; i++)
        {
            str[i] = hex[rand() % 16];
        }
        return BSFixedString(str);
    }
}

bool papyrusFunctions::RegisterFuncs(VirtualMachine* vm)
{
    _DMESSAGE("Lootman register papyrus functions.");

    vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, VMArray<TESObjectREFR *>, TESObjectREFR *, UInt32, VMArray<UInt32>, VMArray<UInt32>>("FindReferencesInRange", "Lootman", papyrusFunctions::FindReferencesInRange, vm));
    vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, VMArray<TESForm *>, TESObjectREFR *, VMArray<UInt32>, VMArray<UInt32>>("FindItemsInInventory", "Lootman", papyrusFunctions::FindItemsInInventory, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMArray<TESForm *>, TESObjectREFR *, bool>("FindJunkOrModInInventory", "Lootman", papyrusFunctions::FindJunkOrModInInventory, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, TESObjectREFR *>("IsLinkedToWorkshop", "Lootman", papyrusFunctions::IsLinkedToWorkshop, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, BSFixedString>("GetConfigInt", "Lootman", papyrusFunctions::GetConfigInt, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("LoadConfig", "Lootman", papyrusFunctions::LoadConfig, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>("GetRandomProcessID", "Lootman", papyrusFunctions::GetRandomProcessID, vm));

    vm->SetFunctionFlags("Lootman", "FindReferencesInRangeFromType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "FindItemsInInventoryFromType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "FindJunkOrModInInventory", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "IsLinkedToWorkshop", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetConfigInt", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "LoadConfig", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetRandomProcessID", IFunction::kFunctionFlag_NoWait);

    return true;
}

/*
void papyrusFunctions::Messaging(F4SEMessagingInterface::Message * msg)
{
    if(msg->type == F4SEMessagingInterface::kMessage_GameDataReady)
    {
        if(msg->data)
        {
            GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&objectLoadedShink);
        }
    }
}
*/