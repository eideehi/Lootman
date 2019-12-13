#include "PapyrusFunctions.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameData.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include <algorithm>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>

std::map<std::string, int> gConfigInt;

namespace papyrusFunctions
{
    struct Wrapper
    {
        TESObjectREFR * ref;
        float distance;

        Wrapper(TESObjectREFR * ptr, float num)
        {
            ref = ptr;
            distance = num;
        }

        bool operator<(const Wrapper &other) const
        {
            return distance > other.distance;
        }
    };

    bool _FormIsIncludeType(TESForm * form, VMArray<UInt32> includeTypes)
    {
        // プレイアブルではないアイテム（多分）は無視する
        if(!form || (form->flags & 4) != 0)
        {
            return false;
        }
        if(includeTypes.IsNone())
        {
            return true;
        }
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

    VMArray<TESForm *> FilterAllFormByFormType(StaticFunctionTag *,
                                               VMArray<TESForm *> forms,
                                               VMArray<UInt32> filterTypes)
    {
        VMArray<TESForm *> result;
        TESForm * form = nullptr;
        for(UInt32 i = 0; i < forms.Length(); i++)
        {
            forms.Get(&form, i);
            if(_FormIsIncludeType(form, filterTypes))
            {
                result.Push(&form);
            }
        }
        return result;
    }

    VMArray<TESObjectREFR *> FilterAllReferenceByFormType(StaticFunctionTag *,
                                                          VMArray<TESObjectREFR *> refs,
                                                          VMArray<UInt32> filterTypes)
    {
        VMArray<TESObjectREFR *> result;
        TESObjectREFR * ref = nullptr;
        for(UInt32 i = 0; i < refs.Length(); i++)
        {
            refs.Get(&ref, i);
            if(ref && _FormIsIncludeType(ref->baseForm, filterTypes))
            {
                result.Push(&ref);
            }
        }
        return result;
    }

    VMArray<TESObjectREFR *> FindAllReferenceWithinRange(StaticFunctionTag *,
                                                         TESObjectREFR * player,
                                                         UInt32 range)
    {
        std::vector<Wrapper> vec;
        VMArray<TESObjectREFR *> result;

        if(!player)
        {
            return result;
        }
        TESObjectCELL * parentCell = player->parentCell;
        if(!parentCell)
        {
            return result;
        }

        NiPoint3 playerPos = player->pos;
        auto find = [&](TESObjectCELL * cell)
        {
            for(int i = 0; i < cell->objectList.count; i++)
            {
                TESObjectREFR * ref = cell->objectList.entries[i];
                NiPoint3 refPos = ref->pos;
                float x = playerPos.x - refPos.x;
                float y = playerPos.y - refPos.y;
                float z = playerPos.z - refPos.z;

                float distance = std::sqrtf((x * x) + (y * y) + (z * z));
                if(range == 0 || distance <= range)
                {
                    vec.push_back(Wrapper(ref, distance));
                }
            }
        };

        find(parentCell);

        TESForm * form = LookupFormByID(parentCell->preVisCell);
        if(form)
        {
            TESObjectCELL * preVisCell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
            if(preVisCell && (preVisCell->flags & 16) != 0)
            {
                find(preVisCell);
            }
        }

        std::sort(vec.begin(), vec.end());
        for(auto &wrapper : vec)
        {
            if(wrapper.ref)
            {
                result.Push(&wrapper.ref);
            }
        }
        return result;
    }

    // 対象のキーワードを含む、すべてのフォームリストを検索して返す
    VMArray<BGSListForm *> FindAllFormListThatHasKeyword(StaticFunctionTag *, BGSKeyword * keyword)
    {
        VMArray<BGSListForm*> result;

        auto list = (*g_dataHandler)->arrFLST;
        for (int i = 0; i < list.count; i++)
        {
            BGSListForm* formList = nullptr;
            list.GetNthItem(i, formList);
            auto forms = formList->forms;
            for (int j = 0; j < forms.count; j++)
            {
                TESForm* form = nullptr;
                forms.GetNthItem(j, form);
                if (keyword->formID == form->formID)
                {
                    result.Push(&formList);
                    break;
                }
            }
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
        {
            return false;
        }

        // 取得したキーワードを使用して
        // オブジェクトにリンクしているワークショップ（と思われる）オブジェクトを取得
        TESObjectREFR * workshopRef = GetLinkedRef_Native(ref, keyword);
        if(!workshopRef)
        {
            return false;
        }

        // 取得したオブジェクトが本当にワークショップであるか確認
        BSExtraData* extraDataWorkshop = workshopRef->extraDataList->GetByType(ExtraDataType::kExtraData_WorkshopExtraData);
        if(!extraDataWorkshop)
        {
            return false;
        }

        return true;
    }

    // キー文字列に対応する、Lootmanのコンフィグ値を取得する
    UInt32 GetConfigInt(StaticFunctionTag *, BSFixedString key)
    {
        auto it = gConfigInt.find(key.c_str());
        if(it != gConfigInt.end())
        {
            return it->second;
        }

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
        gConfigInt["looting_book_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_book_enabled", 0, configPath);
        gConfigInt["looting_book_magazine_only"] = GetPrivateProfileInt(sectionSettings, "looting_book_magazine_only", 1, configPath);
        gConfigInt["looting_cont_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_cont_enabled", 1, configPath);
        gConfigInt["looting_flor_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_flor_enabled", 1, configPath);
        gConfigInt["looting_ingr_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_ingr_enabled", 0, configPath);
        gConfigInt["looting_misc_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_misc_enabled", 1, configPath);
        gConfigInt["looting_npc__enabled"] = GetPrivateProfileInt(sectionSettings, "looting_npc__enabled", 1, configPath);
        gConfigInt["looting_range"] = GetPrivateProfileInt(sectionSettings, "looting_range", 800, configPath);
        gConfigInt["looting_weap_enabled"] = GetPrivateProfileInt(sectionSettings, "looting_weap_enabled", 1, configPath);
        gConfigInt["lootman_carry_weight"] = GetPrivateProfileInt(sectionSettings, "lootman_carry_weight", 1000000, configPath);
        gConfigInt["lootman_overweight_ignore"] = GetPrivateProfileInt(sectionSettings, "lootman_overweight_ignore", 1, configPath);
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

    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMArray<TESForm *>, VMArray<TESForm *>, VMArray<UInt32>>("FilterAllFormByFormType", "Lootman", papyrusFunctions::FilterAllFormByFormType, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMArray<TESObjectREFR *>, VMArray<TESObjectREFR *>, VMArray<UInt32>>("FilterAllReferenceByFormType", "Lootman", papyrusFunctions::FilterAllReferenceByFormType, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMArray<TESObjectREFR *>, TESObjectREFR *, UInt32>("FindAllReferenceWithinRange", "Lootman", papyrusFunctions::FindAllReferenceWithinRange, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<BGSListForm *>, BGSKeyword *>("FindAllFormListThatHasKeyword", "Lootman", papyrusFunctions::FindAllFormListThatHasKeyword, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, TESObjectREFR *>("IsLinkedToWorkshop", "Lootman", papyrusFunctions::IsLinkedToWorkshop, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, BSFixedString>("GetConfigInt", "Lootman", papyrusFunctions::GetConfigInt, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("LoadConfig", "Lootman", papyrusFunctions::LoadConfig, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>("GetRandomProcessID", "Lootman", papyrusFunctions::GetRandomProcessID, vm));

    vm->SetFunctionFlags("Lootman", "FilterAllFormByFormType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "FilterAllReferenceByFormType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "FindAllReferenceWithinRange", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "FindAllFormListThatHasKeyword", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "IsLinkedToWorkshop", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetConfigInt", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "LoadConfig", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetRandomProcessID", IFunction::kFunctionFlag_NoWait);

    return true;
}
