#include "PapyrusLootman.h"

#include <algorithm>
#include <vector>
#include <map>

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameData.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "Globals.h"

#ifdef _DEBUG

#include <sstream>
#include <chrono>
#include <iomanip>

#include "Debug.h"

#endif

namespace PapyrusLootman
{
    DECLARE_STRUCT(MiscComponent, "MiscObject")

    struct ObjectReferenceWithDistance
    {
        TESObjectREFR * ref;
        float distance;

        ObjectReferenceWithDistance(TESObjectREFR * ptr, float num)
        {
            ref = ptr;
            distance = num;
        }

        bool operator<(const ObjectReferenceWithDistance &other) const
        {
            // Papyrusのループでは逆順で走査するので、近い方から使用するために降順でソートさせる
            return distance > other.distance;
        }
    };

    // エクストラデータからすべてのModデータを取得して返す
    VMArray<BGSMod::Attachment::Mod *> _GetAllMods(ExtraDataList * extraDataList)
    {
        VMArray<BGSMod::Attachment::Mod *> result;

        if(!extraDataList)
        {
            return result;
        }

        BSExtraData * extraData = extraDataList->GetByType(ExtraDataType::kExtraData_ObjectInstance);
        if(!extraData)
        {
            return result;
        }

        BGSObjectInstanceExtra * objectModData = DYNAMIC_CAST(extraData, BSExtraData, BGSObjectInstanceExtra);
        if(!objectModData)
        {
            return result;
        }

        BGSObjectInstanceExtra::Data * data = objectModData->data;
        if(!data || !data->forms)
        {
            return result;
        }

        for(UInt32 i = 0; i < (data->blockSize / sizeof(BGSObjectInstanceExtra::Data::Form)); i++)
        {
            BGSMod::Attachment::Mod * objectMod = (BGSMod::Attachment::Mod *)Runtime_DynamicCast(LookupFormByID(data->forms[i].formId), RTTI_TESForm, RTTI_BGSMod__Attachment__Mod);

            if(!objectMod)
            {
                continue;
            }

            result.Push(&objectMod);
        }

        return result;
    }

    // Mod一覧の中にレジェンダリーが存在するか評価して返す
    bool _HasLegendaryMod(VMArray<BGSMod::Attachment::Mod *> mods)
    {
        for(UInt32 i = 0; i < mods.Length(); i++)
        {
            BGSMod::Attachment::Mod * objectMod = nullptr;
            mods.Get(&objectMod, i);

            if(objectMod->flags == 25)
            {
                // おそらく objectMod->flags == 25 はレジェンダリー
                return true;
            }
        }
        return false;
    }

    // フォームがプレイアプルであるか評価して返す
    bool _IsPlayable(TESForm * form)
    {
        return form && (form->flags & 1 << 2) == 0;
    }

    // オブジェクト参照がパピルスで操作不可能なネイティブオブジェクトであるか評価して返す
    bool _IsNativeObject(TESObjectREFR * ref)
    {
        return (ref->formID >> 24) == 0xFF && (ref->baseForm->formID >> 24) == 0xFF && (ref->flags & 1 << 14) != 0;
    }

    // 対象のオブジェクト参照を起点に、指定の範囲内に存在するオブジェクト参照を取得し、指定のフォームタイプでフィルタリングして返す
    VMArray<TESObjectREFR *> FindAllReferencesOfFormType(StaticFunctionTag *, TESObjectREFR * ref, UInt32 range, UInt32 formType)
    {
#ifdef _DEBUG
        const char * processId = _GetRandomProcessID();
        _MESSAGE("| %s | FIND REFERENCE START", processId);
#endif
        VMArray<TESObjectREFR *> result;

        if(!ref)
        {
            return result;
        }

        TESObjectCELL * cell = ref->parentCell;
        if(!cell)
        {
            return result;
        }

        std::vector<ObjectReferenceWithDistance> tmp;
        NiPoint3 pos1 = ref->pos;

        auto find = [&](TESObjectCELL * cell)
        {
            for(int i = 0; i < cell->objectList.count; i++)
            {
                TESObjectREFR * obj = cell->objectList.entries[i];
                if(!obj)
                {
                    continue;
                }

                if((obj->flags & TESForm::kFlag_IsDeleted) != 0) continue;// 削除済みのオブジェクトは無視

                TESForm * form = obj->baseForm;
                if(!_IsPlayable(form) || form->formType != formType)
                {
                    continue;
                }

#ifdef _DEBUG
                if(_IsNativeObject(obj)) { _MESSAGE("| %s | NATIVE OBJECT ?", processId); _TraceTESObjectREFR(processId, obj, 0); }
#endif
                if(_IsNativeObject(obj)) continue;// パピルスにバインド不可能なネイティブオブジェクトを無視

                NiPoint3 pos2 = obj->pos;
                float x = pos1.x - pos2.x;
                float y = pos1.y - pos2.y;
                float z = pos1.z - pos2.z;
                float distance = std::sqrtf((x * x) + (y * y) + (z * z));

                if(distance != 0 && (range == 0 || distance <= range))
                {
#ifdef _DEBUG
                    _TraceReferenceFlags(processId, obj, 0);
#endif
                    tmp.push_back(ObjectReferenceWithDistance(obj, distance));
                }
            }
        };

        find(cell);

        TESForm * form = LookupFormByID(cell->preVisCell);
        if(form)
        {
            TESObjectCELL * preVisCell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
            if(preVisCell && (preVisCell->flags & 16) != 0)
            {
                find(preVisCell);
            }
        }

        std::sort(tmp.begin(), tmp.end());
        for(ObjectReferenceWithDistance &element : tmp)
        {
#ifdef _DEBUG
            _MESSAGE("| %s | Distance: [%f]", processId, element.distance);
            _TraceTESObjectREFR(processId, element.ref, 0);
#endif
            result.Push(&element.ref);
        }

#ifdef _DEBUG
        _MESSAGE("| %s | FIND REFERENCE END", processId);
#endif
        return result;
    }

    // 対象のオブジェクト参照のインベントリから、指定のフォームタイプのアイテムのみ取得して返す
    VMArray<TESForm *> GetInventoryItemsOfFormTypes(StaticFunctionTag *, TESObjectREFR * ref, VMArray<UInt32> formTypes)
    {
#ifdef _DEBUG
        const char * processId = _GetRandomProcessID();
        _MESSAGE("| %s | FIND INVENTORY ITEMS START", processId);
#endif
        VMArray<TESForm *> result;

        if(!ref || formTypes.IsNone())
        {
            return result;
        }

#ifdef _DEBUG
        _MESSAGE("| %s | Target: [Name=%s, ID=%08X]", processId, CALL_MEMBER_FN(ref, GetReferenceName)(), ref->formID);
#endif

        BGSInventoryList * inventoryList = ref->inventoryList;
        if(!inventoryList)
        {
            return result;
        }

        inventoryList->inventoryLock.LockForRead();

        for(int i = 0; i < inventoryList->items.count; i++)
        {
            BGSInventoryItem item;
            inventoryList->items.GetNthItem(i, item);

            TESForm * form = item.form;
            if(!_IsPlayable(form))
            {
                continue;
            }

            UInt8 formType = form->formType;
            bool formTypeIsInArray = false;
            for(UInt32 j = 0; j < formTypes.Length(); j++)
            {
                UInt32 type;
                formTypes.Get(&type, j);

                if(type == -1)
                {
                    formTypeIsInArray = (formType == FormType::kFormType_ALCH) ||
                                        (formType == FormType::kFormType_AMMO) ||
                                        (formType == FormType::kFormType_ARMO) ||
                                        (formType == FormType::kFormType_BOOK) ||
                                        (formType == FormType::kFormType_INGR) ||
                                        (formType == FormType::kFormType_KEYM) ||
                                        (formType == FormType::kFormType_MISC) ||
                                        (formType == FormType::kFormType_WEAP);
                    break;
                }
                else if(formType == type)
                {
                    formTypeIsInArray = true;
                    break;
                }
            }
            if(!formTypeIsInArray)
            {
                continue;
            }

#ifdef _DEBUG
            _TraceBGSInventoryItem(processId, &item, 0);
#endif

            if(form->formType == FormType::kFormType_WEAP)
            {
                bool isDroppedWeapon = false;
                item.stack->Visit([&isDroppedWeapon](BGSInventoryItem::Stack * stack) mutable
                {
                    isDroppedWeapon = (stack->flags & 1 << 5) != 0;
                    return !isDroppedWeapon;
                });
                if(isDroppedWeapon)
                {
                    continue;
                }
            }

            result.Push(&form);
        }

        inventoryList->inventoryLock.Unlock();

#ifdef _DEBUG
        _MESSAGE("| %s | FIND INVENTORY ITEMS END", processId);
#endif
        return result;
    }

    // 対象のオブジェクト参照がレジェンダリーアイテムであるか評価して返す
    // オブジェクト参照のフォームがプレイアブルでない、あるいは武器でも防具でもない場合はfalseを返す
    bool IsLegendaryItem(StaticFunctionTag *, VMRefOrInventoryObj * ref)
    {
        if(!ref)
        {
            return false;
        }

        TESForm * baseForm = nullptr;
        ExtraDataList * extraDataList = nullptr;
        ref->GetExtraData(&baseForm, &extraDataList);
        if(!_IsPlayable(baseForm) || (baseForm->formType != FormType::kFormType_WEAP && baseForm->formType != FormType::kFormType_ARMO))
        {
            return false;
        }

        return _HasLegendaryMod(_GetAllMods(extraDataList));
    }

    // 対象のオブジェクト参照が指定されたフォームのレジェンダリーアイテムを所持しているか評価して返す
    // フォームがプレイアブルでない、あるいは武器でも防具でもない場合はfalseを返す
    bool HasLegendaryItem(StaticFunctionTag *, TESObjectREFR * ref, TESForm * form)
    {
        if(!ref || !_IsPlayable(form) || (form->formType != FormType::kFormType_WEAP && form->formType != FormType::kFormType_ARMO))
        {
            return false;
        }

        BGSInventoryList * inventoryList = ref->inventoryList;
        if(!inventoryList)
        {
            return false;
        }

        inventoryList->inventoryLock.LockForRead();

        for(int i = 0; i < inventoryList->items.count; i++)
        {
            BGSInventoryItem item;
            inventoryList->items.GetNthItem(i, item);

            TESForm * formInInventory = item.form;
            if(!formInInventory || formInInventory->formID != form->formID)
            {
                continue;
            }

#ifdef _DEBUG
            _TraceBGSInventoryItem(_GetRandomProcessID(), &item, 0);
#endif

            bool hasLegendaryMod = false;
            item.stack->Visit([&hasLegendaryMod](BGSInventoryItem::Stack * stack) mutable
            {
                if(_HasLegendaryMod(_GetAllMods(stack->extraData)))
                {
                    hasLegendaryMod = true;
                    return false;
                }
                return true;
            });

            if(hasLegendaryMod)
            {
                return true;
            }
        }

        inventoryList->inventoryLock.Unlock();

        return false;
    }

    // 対象のキーワードを含む、すべてのフォームリストを検索して返す
    VMArray<TESForm *> GetInjectionDataForList(StaticFunctionTag *, BSFixedString identify)
    {
        VMArray<TESForm *> result;

        auto dataIt = Globals::formListData.FindMember(identify);
        if(dataIt == Globals::formListData.MemberEnd() || !dataIt->value.IsArray())
        {
            return result;
        }

        for (auto it = dataIt->value.Begin(); it != dataIt->value.End(); ++it)
        {
            if(!it->IsString())
            {
                continue;
            }

            std::string value = it->GetString();
            std::string::size_type delimiter = value.find('|');
            if(delimiter != std::string::npos)
            {
                std::string modName = value.substr(0, delimiter);
                const ModInfo * info = (*g_dataHandler)->LookupModByName(modName.c_str());
                if(!info)
                {
                    _WARNING("* Mod is not found [%s]", modName.c_str());
                    continue;
                }

                std::string lowerFormId = value.substr(delimiter + 1);
                UInt32 formId = info->GetFormID(std::stoul(lowerFormId, nullptr, 16));
                TESForm * form = LookupFormByID(formId);
                if(!form)
                {
                    _WARNING("* Form is not found [ModName=%s, LowerFormId=%s, FormId=%08X]", modName.c_str(), lowerFormId.c_str(), formId);
                    continue;
                }

                result.Push(&form);
            }
        }

        return result;
    }

    // フォームのフォームタイプを取得して返す
    UInt32 GetFormType(StaticFunctionTag *, TESForm * form)
    {
        return !form ? -1 : form->formType;
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

    // オブジェクトをスクラップした場合のコンポーネントを取得する。
    VMArray<MiscComponent> GetScrapComponents(StaticFunctionTag *, VMRefOrInventoryObj * ref)
    {
        VMArray<MiscComponent> result;
        if(!ref)
        {
            return result;
        }

        TESForm * baseForm = nullptr;
        ExtraDataList * extraDataList = nullptr;
        ref->GetExtraData(&baseForm, &extraDataList);
        if(!baseForm || !extraDataList)
        {
            return result;
        }

        if(baseForm->formType != FormType::kFormType_ARMO && baseForm->formType != FormType::kFormType_WEAP)
        {
            return result;
        }

        std::map<BGSComponent *, UInt32> map;
        auto push = [&map](BGSConstructibleObject * cobj)
        {
            if(!cobj)
            {
                return;
            }

            for(UInt32 i = 0; i < cobj->components->count; i++)
            {
                BGSConstructibleObject::Component cobjComponent;
                cobj->components->GetNthItem(i, cobjComponent);

                TESObjectMISC * misc = DYNAMIC_CAST(cobjComponent.component, TESForm, TESObjectMISC);
                if(misc)
                {
                    for(UInt32 j = 0; j < cobjComponent.count; j++)
                    {
                        for(UInt32 k = 0; k < misc->components->count; k++)
                        {
                            TESObjectMISC::Component miscComponent;
                            misc->components->GetNthItem(k, miscComponent);

                            map[miscComponent.component] += (UInt32)miscComponent.count;
                        }
                    }
                }
                else
                {
                    map[cobjComponent.component] += cobjComponent.count;
                }
            }
        };

        tArray<BGSConstructibleObject *> cobjList = (*g_dataHandler)->arrCOBJ;
        auto find = [&cobjList](TESForm * form)
        {
            for(UInt32 i = 0; i < cobjList.count; i++)
            {
                BGSConstructibleObject * cobj = nullptr;
                cobjList.GetNthItem(i, cobj);

                if(!cobj || !cobj->createdObject || !cobj->components)
                {
                    continue;
                }

                if(form->formID == cobj->createdObject->formID)
                {
                    return cobj;
                }

                BGSListForm * formList = DYNAMIC_CAST(cobj->createdObject, TESForm, BGSListForm);
                if(formList)
                {
                    for(UInt32 j = 0; j < formList->forms.count; j++)
                    {
                        TESForm * item = nullptr;
                        formList->forms.GetNthItem(j, item);

                        if(item && form->formID == item->formID)
                        {
                            return cobj;
                        }
                    }
                }
            }
            return (BGSConstructibleObject *)nullptr;
        };

        VMArray<BGSMod::Attachment::Mod *> mods = _GetAllMods(extraDataList);
        for(UInt32 i = 0; i < mods.Length(); i++)
        {
            BGSMod::Attachment::Mod * objectMod = nullptr;
            mods.Get(&objectMod, i);

            push(find(objectMod));
        }

        push(find(baseForm));

        for(auto const &it : map)
        {
            MiscComponent comp;
            comp.Set("object", it.first);
            comp.Set("count", (UInt32)(it.second / 2));
            result.Push(&comp);
        }

        return result;
    }

#ifdef _DEBUG
    // ランダムなプロセスID(10桁のランダムな16進数文字列)を生成して返す
    BSFixedString GetRandomProcessID(StaticFunctionTag *)
    {
        return _GetRandomProcessID();
    }

    // フォームIDを16進数の文字列にして返す
    BSFixedString GetHexID(StaticFunctionTag *, TESForm * form)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << form->formID;
        return ss.str().c_str();
    }

    // ミリ秒を取得して返す
    BSFixedString GetMilliseconds(StaticFunctionTag *)
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str().c_str();
    }

    // フォームの識別名を取得して返す
    BSFixedString GetIdentify(StaticFunctionTag *, TESForm * form)
    {
        TESFullName * fullName = DYNAMIC_CAST(form, TESForm, TESFullName);
        if (fullName && strlen(fullName->name))
        {
            return fullName->name;
        }
        return form->GetEditorID();
    }
#endif
}

bool PapyrusLootman::RegisterFuncs(VirtualMachine* vm)
{
    _MESSAGE(">> Lootman papyrus functions register phase start.");

    vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, VMArray<TESObjectREFR *>, TESObjectREFR *, UInt32, UInt32>("FindAllReferencesOfFormType", "Lootman", PapyrusLootman::FindAllReferencesOfFormType, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, TESForm *>("GetFormType", "Lootman", PapyrusLootman::GetFormType, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<TESForm *>, BSFixedString>("GetInjectionDataForList", "Lootman", PapyrusLootman::GetInjectionDataForList, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMArray<TESForm *>, TESObjectREFR *, VMArray<UInt32>>("GetInventoryItemsOfFormTypes", "Lootman", PapyrusLootman::GetInventoryItemsOfFormTypes, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, TESObjectREFR *, TESForm *>("HasLegendaryItem", "Lootman", PapyrusLootman::HasLegendaryItem, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, VMRefOrInventoryObj *>("IsLegendaryItem", "Lootman", PapyrusLootman::IsLegendaryItem, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, TESObjectREFR *>("IsLinkedToWorkshop", "Lootman", PapyrusLootman::IsLinkedToWorkshop, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<MiscComponent>, VMRefOrInventoryObj *>("GetScrapComponents", "Lootman", PapyrusLootman::GetScrapComponents, vm));

    //vm->SetFunctionFlags("Lootman", "FindAllReferencesOfFormType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetFormType", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetInjectionDataForList", IFunction::kFunctionFlag_NoWait);
    //vm->SetFunctionFlags("Lootman", "GetInventoryItemsOfFormTypes", IFunction::kFunctionFlag_NoWait);
    //vm->SetFunctionFlags("Lootman", "HasLegendaryItem", IFunction::kFunctionFlag_NoWait);
    //vm->SetFunctionFlags("Lootman", "IsLegendaryItem", IFunction::kFunctionFlag_NoWait);
    //vm->SetFunctionFlags("Lootman", "IsLinkedToWorkshop", IFunction::kFunctionFlag_NoWait);
    //vm->SetFunctionFlags("Lootman", "GetScrapComponents", IFunction::kFunctionFlag_NoWait);

#ifdef _DEBUG
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, TESForm *>("GetHexID", "Lootman", PapyrusLootman::GetHexID, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, TESForm *>("GetIdentify", "Lootman", PapyrusLootman::GetIdentify, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>("GetMilliseconds", "Lootman", PapyrusLootman::GetMilliseconds, vm));
    vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>("GetRandomProcessID", "Lootman", PapyrusLootman::GetRandomProcessID, vm));

    vm->SetFunctionFlags("Lootman", "GetHexID", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetIdentify", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetMilliseconds", IFunction::kFunctionFlag_NoWait);
    vm->SetFunctionFlags("Lootman", "GetRandomProcessID", IFunction::kFunctionFlag_NoWait);
#endif

    _MESSAGE(">> Lootman papyrus functions register phase end.");
    return true;
}
