#include "PapyrusLootman.h"

#include <algorithm>
#include <map>
#include <unordered_set>
#include <vector>

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameData.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "FormIDCache.h"
#include "InjectionData.h"

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
            // The Papyrus loop scans in reverse order, so it sorts in descending order so that it is processed from the one closest to the player
            return distance > other.distance;
        }
    };

    // Get all mod data from extradata
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

    // Verify that the Legendary is present in the Mod list
    bool _HasLegendaryMod(VMArray<BGSMod::Attachment::Mod *> mods)
    {
        for(UInt32 i = 0; i < mods.Length(); i++)
        {
            BGSMod::Attachment::Mod * objectMod = nullptr;
            mods.Get(&objectMod, i);

            // The 25th bit is the flag for the Legendary item (probably)
            if(objectMod->flags == 25)
            {
                return true;
            }
        }
        return false;
    }

    // Verify that the form is playable
    bool _IsPlayable(TESForm * form)
    {
        return form && (form->flags & 1 << 2) == 0;
    }

    // Verify that an object reference is a native object that cannot be manipulated by papyrus
    bool _IsNativeObject(TESObjectREFR * ref)
    {
        return (ref->formID >> 24) == 0xFF && (ref->baseForm->formID >> 24) == 0xFF && (ref->flags & 1 << 14) != 0;
    }

    // Retrieves objects that exist within a certain range starting from a specified object, and returns the objects filtered by form type
    VMArray<TESObjectREFR *> FindAllReferencesOfFormType(StaticFunctionTag *, TESObjectREFR * ref, UInt32 range, UInt32 formType)
    {
#ifdef _DEBUG
        const char * processId = _GetRandomProcessID();
        _MESSAGE("| %s | *** FindAllReferencesOfFormType start ***", processId);
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

        std::unordered_set<UInt32> knownId;
        std::vector<ObjectReferenceWithDistance> foundObjects;
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

                // Ignore deleted or disabled objects.
                if((obj->flags & (TESForm::kFlag_IsDeleted | TESForm::kFlag_IsDisabled)) != 0)
                {
                    continue;
                }

                TESForm * form = obj->baseForm;
                if(form->formType != formType || !_IsPlayable(form))
                {
                    continue;
                }

                // Ignore native objects that cannot be bound to papyrus
                if(_IsNativeObject(obj))
                {
#ifdef _DEBUG
                    _MESSAGE("| %s |   ** Maybe a native object **", processId);
                    _TraceTESObjectREFR(processId, obj, 2);
#endif
                    continue;
                }

                if(!knownId.insert(obj->formID).second)
                {
                    continue;
                }

                NiPoint3 pos2 = obj->pos;
                float x = pos1.x - pos2.x;
                float y = pos1.y - pos2.y;
                float z = pos1.z - pos2.z;
                float distance = std::sqrtf((x * x) + (y * y) + (z * z));

                // Ignore objects with a distance of 0 because they are players
                if(distance == 0)
                {
                    continue;
                }

                if(distance <= range)
                {
                    foundObjects.push_back(ObjectReferenceWithDistance(obj, distance));
                }
            }
        };

        find(cell);

        {
            SimpleLocker locker(&FormIDCache::lock);
            for(auto it = FormIDCache::cells.begin(); it != FormIDCache::cells.end(); ++it)
            {
                cell = DYNAMIC_CAST(LookupFormByID(*it), TESForm, TESObjectCELL);
                // Not explore cells that are not 3D loaded
                if(cell && (cell->flags & 16) != 0)
                {
                    find(cell);
                }
            }
        }

#ifdef _DEBUG
        _MESSAGE("| %s |   ** Found objects **", processId);
#endif
        std::sort(foundObjects.begin(), foundObjects.end());
        for(ObjectReferenceWithDistance &element : foundObjects)
        {
#ifdef _DEBUG
            _MESSAGE("| %s |     Distance: [%f]", processId, element.distance);
            _TraceTESObjectREFR(processId, element.ref, 2);
            _TraceReferenceFlags(processId, element.ref, 3);
#endif
            result.Push(&element.ref);
        }

#ifdef _DEBUG
        _MESSAGE("| %s | *** FindAllReferencesOfFormType end ***", processId);
#endif
        return result;
    }

    // Get and return only items of a specified form type from an inventory of object references
    VMArray<TESForm *> GetInventoryItemsOfFormTypes(StaticFunctionTag *, TESObjectREFR * ref, VMArray<UInt32> formTypes)
    {
#ifdef _DEBUG
        const char * processId = _GetRandomProcessID();
        _MESSAGE("| %s | *** GetInventoryItemsOfFormTypes start ***", processId);
#endif
        VMArray<TESForm *> result;

        if(!ref || formTypes.IsNone())
        {
            return result;
        }

#ifdef _DEBUG
        _MESSAGE("| %s |   Target: [Name=%s, ID=%08X]", processId, CALL_MEMBER_FN(ref, GetReferenceName)(), ref->formID);
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

#ifdef _DEBUG
            _MESSAGE("| %s |   [Item %d]", processId, i);
            _TraceBGSInventoryItem(processId, &item, 2);
#endif

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

            if(form->formType == FormType::kFormType_WEAP)
            {
                bool isDroppedWeapon = false;
                item.stack->Visit([&isDroppedWeapon](BGSInventoryItem::Stack * stack) mutable
                {
                    isDroppedWeapon = (stack->flags & 1 << 5) != 0;
                    return !isDroppedWeapon;
                });
#ifdef _DEBUG
                _MESSAGE("| %s |       Is dropped weapon: %s", processId, isDroppedWeapon ? "true" : "false");
#endif
                if(isDroppedWeapon)
                {
                    continue;
                }
            }

            result.Push(&form);
        }

        inventoryList->inventoryLock.Unlock();

#ifdef _DEBUG
        _MESSAGE("| %s | *** GetInventoryItemsOfFormTypes end ***", processId);
#endif
        return result;
    }

    // Verify the object is a Legendary item. Returns false if the object is not playable, or if it is neither a weapon nor armor
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

    // Verify the existence of the specified item's legendary in the object's inventory. Returns false if the item is not playable, or if it is neither a weapon nor armor
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

    // Get and return the injection data to be registered in the form list
    VMArray<TESForm *> GetInjectionDataForList(StaticFunctionTag *, BSFixedString identify)
    {
        VMArray<TESForm *> result;

        auto dataIt = InjectionData::formListData.FindMember(identify);
        if(dataIt == InjectionData::formListData.MemberEnd() || !dataIt->value.IsArray())
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
                    _WARNING("* Form is not found [ModName: %s, LowerFormID: %s, FormID: %08X]", modName.c_str(), lowerFormId.c_str(), formId);
                    continue;
                }

                result.Push(&form);
            }
        }

        return result;
    }

    // Get and returns the form type of the form
    UInt32 GetFormType(StaticFunctionTag *, TESForm * form)
    {
        return !form ? -1 : form->formType;
    }

    // Verify the object is linked to the workshop
    // Source code used for reference: PapyrusObjectReference#AttachWireLatent
    bool IsLinkedToWorkshop(StaticFunctionTag *, TESObjectREFR * ref)
    {
        BGSKeyword * keyword = nullptr;
        BGSDefaultObject * workshopItemDefault = (*g_defaultObjectMap)->GetDefaultObject("WorkshopItem");
        if(workshopItemDefault)
        {
            keyword = DYNAMIC_CAST(workshopItemDefault->form, TESForm, BGSKeyword);
        }

        if(!keyword)
        {
            return false;
        }

        TESObjectREFR * workshopRef = GetLinkedRef_Native(ref, keyword);
        if(!workshopRef)
        {
            return false;
        }

        BSExtraData* extraDataWorkshop = workshopRef->extraDataList->GetByType(ExtraDataType::kExtraData_WorkshopExtraData);
        if(!extraDataWorkshop)
        {
            return false;
        }

        return true;
    }

    // Return the result of scrapping an object
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
    // Generate and return a random process ID
    BSFixedString GetRandomProcessID(StaticFunctionTag *)
    {
        return _GetRandomProcessID();
    }

    // Converts and return the form ID to a hexadecimal string
    BSFixedString GetHexID(StaticFunctionTag *, TESForm * form)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << form->formID;
        return ss.str().c_str();
    }

    // Get and return the current millisecond
    BSFixedString GetMilliseconds(StaticFunctionTag *)
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str().c_str();
    }

    // Get and return the form's identify
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
