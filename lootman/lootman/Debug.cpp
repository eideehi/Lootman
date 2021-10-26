#include "Debug.h"

#ifdef _DEBUG

#include <bitset>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <random>
#include <sstream>

#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

const char * _FlagsToBinaryString(UInt64 flags)
{
    return BSFixedString(std::bitset< 64 >(flags).to_string().c_str());
}

const char * _FlagsToBinaryString(UInt32 flags)
{
    return BSFixedString(std::bitset< 32 >(flags).to_string().c_str());
}

const char * _FlagsToBinaryString(UInt16 flags)
{
    return BSFixedString(std::bitset< 16 >(flags).to_string().c_str());
}

const char * _FlagsToBinaryString(SInt32 flags)
{
    return BSFixedString(std::bitset< 32 >(flags).to_string().c_str());
}

const char * _FormTypeToString(UInt8 formType)
{
    switch(formType)
    {
        case FormType::kFormType_ACTI: return "ACTI"; case FormType::kFormType_ALCH: return "ALCH"; case FormType::kFormType_AMMO: return "AMMO";
        case FormType::kFormType_ARMO: return "ARMO"; case FormType::kFormType_BOOK: return "BOOK"; case FormType::kFormType_CONT: return "CONT";
        case FormType::kFormType_FLOR: return "FLOR"; case FormType::kFormType_INGR: return "INGR"; case FormType::kFormType_KEYM: return "KEYM";
        case FormType::kFormType_MISC: return "MISC"; case FormType::kFormType_NPC_: return "NPC_"; case FormType::kFormType_WEAP: return "WEAP";
        case FormType::kFormType_TES4: return "TES4"; case FormType::kFormType_GRUP: return "GRUP"; case FormType::kFormType_GMST: return "GMST";
        case FormType::kFormType_KYWD: return "KYWD"; case FormType::kFormType_LCRT: return "LCRT"; case FormType::kFormType_AACT: return "AACT";
        case FormType::kFormType_TRNS: return "TRNS"; case FormType::kFormType_CMPO: return "CMPO"; case FormType::kFormType_TXST: return "TXST";
        case FormType::kFormType_MICN: return "MICN"; case FormType::kFormType_GLOB: return "GLOB"; case FormType::kFormType_DMGT: return "DMGT";
        case FormType::kFormType_CLAS: return "CLAS"; case FormType::kFormType_FACT: return "FACT"; case FormType::kFormType_HDPT: return "HDPT";
        case FormType::kFormType_EYES: return "EYES"; case FormType::kFormType_RACE: return "RACE"; case FormType::kFormType_SOUN: return "SOUN";
        case FormType::kFormType_ASPC: return "ASPC"; case FormType::kFormType_SKIL: return "SKIL"; case FormType::kFormType_MGEF: return "MGEF";
        case FormType::kFormType_SCPT: return "SCPT"; case FormType::kFormType_LTEX: return "LTEX"; case FormType::kFormType_ENCH: return "ENCH";
        case FormType::kFormType_SPEL: return "SPEL"; case FormType::kFormType_SCRL: return "SCRL"; case FormType::kFormType_TACT: return "TACT";
        case FormType::kFormType_DOOR: return "DOOR"; case FormType::kFormType_LIGH: return "LIGH"; case FormType::kFormType_STAT: return "STAT";
        case FormType::kFormType_SCOL: return "SCOL"; case FormType::kFormType_MSTT: return "MSTT"; case FormType::kFormType_GRAS: return "GRAS";
        case FormType::kFormType_TREE: return "TREE"; case FormType::kFormType_FURN: return "FURN"; case FormType::kFormType_LVLN: return "LVLN";
        case FormType::kFormType_IDLM: return "IDLM"; case FormType::kFormType_NOTE: return "NOTE"; case FormType::kFormType_PROJ: return "PROJ";
        case FormType::kFormType_HAZD: return "HAZD"; case FormType::kFormType_BNDS: return "BNDS"; case FormType::kFormType_SLGM: return "SLGM";
        case FormType::kFormType_TERM: return "TERM"; case FormType::kFormType_LVLI: return "LVLI"; case FormType::kFormType_WTHR: return "WTHR";
        case FormType::kFormType_CLMT: return "CLMT"; case FormType::kFormType_SPGD: return "SPGD"; case FormType::kFormType_RFCT: return "RFCT";
        case FormType::kFormType_REGN: return "REGN"; case FormType::kFormType_NAVI: return "NAVI"; case FormType::kFormType_CELL: return "CELL";
        case FormType::kFormType_REFR: return "REFR"; case FormType::kFormType_ACHR: return "ACHR"; case FormType::kFormType_PMIS: return "PMIS";
        case FormType::kFormType_PARW: return "PARW"; case FormType::kFormType_PGRE: return "PGRE"; case FormType::kFormType_PBEA: return "PBEA";
        case FormType::kFormType_PFLA: return "PFLA"; case FormType::kFormType_PCON: return "PCON"; case FormType::kFormType_PBAR: return "PBAR";
        case FormType::kFormType_PHZD: return "PHZD"; case FormType::kFormType_WRLD: return "WRLD"; case FormType::kFormType_LAND: return "LAND";
        case FormType::kFormType_NAVM: return "NAVM"; case FormType::kFormType_TLOD: return "TLOD"; case FormType::kFormType_DIAL: return "DIAL";
        case FormType::kFormType_INFO: return "INFO"; case FormType::kFormType_QUST: return "QUST"; case FormType::kFormType_IDLE: return "IDLE";
        case FormType::kFormType_PACK: return "PACK"; case FormType::kFormType_CSTY: return "CSTY"; case FormType::kFormType_LSCR: return "LSCR";
        case FormType::kFormType_LVSP: return "LVSP"; case FormType::kFormType_ANIO: return "ANIO"; case FormType::kFormType_WATR: return "WATR";
        case FormType::kFormType_EFSH: return "EFSH"; case FormType::kFormType_TOFT: return "TOFT"; case FormType::kFormType_EXPL: return "EXPL";
        case FormType::kFormType_DEBR: return "DEBR"; case FormType::kFormType_IMGS: return "IMGS"; case FormType::kFormType_IMAD: return "IMAD";
        case FormType::kFormType_FLST: return "FLST"; case FormType::kFormType_PERK: return "PERK"; case FormType::kFormType_BPTD: return "BPTD";
        case FormType::kFormType_ADDN: return "ADDN"; case FormType::kFormType_AVIF: return "AVIF"; case FormType::kFormType_CAMS: return "CAMS";
        case FormType::kFormType_CPTH: return "CPTH"; case FormType::kFormType_VTYP: return "VTYP"; case FormType::kFormType_MATT: return "MATT";
        case FormType::kFormType_IPCT: return "IPCT"; case FormType::kFormType_IPDS: return "IPDS"; case FormType::kFormType_ARMA: return "ARMA";
        case FormType::kFormType_ECZN: return "ECZN"; case FormType::kFormType_LCTN: return "LCTN"; case FormType::kFormType_MESG: return "MESG";
        case FormType::kFormType_RGDL: return "RGDL"; case FormType::kFormType_DOBJ: return "DOBJ"; case FormType::kFormType_DFOB: return "DFOB";
        case FormType::kFormType_LGTM: return "LGTM"; case FormType::kFormType_MUSC: return "MUSC"; case FormType::kFormType_FSTP: return "FSTP";
        case FormType::kFormType_FSTS: return "FSTS"; case FormType::kFormType_SMBN: return "SMBN"; case FormType::kFormType_SMQN: return "SMQN";
        case FormType::kFormType_SMEN: return "SMEN"; case FormType::kFormType_DLBR: return "DLBR"; case FormType::kFormType_MUST: return "MUST";
        case FormType::kFormType_DLVW: return "DLVW"; case FormType::kFormType_WOOP: return "WOOP"; case FormType::kFormType_SHOU: return "SHOU";
        case FormType::kFormType_EQUP: return "EQUP"; case FormType::kFormType_RELA: return "RELA"; case FormType::kFormType_SCEN: return "SCEN";
        case FormType::kFormType_ASTP: return "ASTP"; case FormType::kFormType_OTFT: return "OTFT"; case FormType::kFormType_ARTO: return "ARTO";
        case FormType::kFormType_MATO: return "MATO"; case FormType::kFormType_MOVT: return "MOVT"; case FormType::kFormType_SNDR: return "SNDR";
        case FormType::kFormType_DUAL: return "DUAL"; case FormType::kFormType_SNCT: return "SNCT"; case FormType::kFormType_SOPM: return "SOPM";
        case FormType::kFormType_COLL: return "COLL"; case FormType::kFormType_CLFM: return "CLFM"; case FormType::kFormType_REVB: return "REVB";
        case FormType::kFormType_PKIN: return "PKIN"; case FormType::kFormType_RFGP: return "RFGP"; case FormType::kFormType_AMDL: return "AMDL";
        case FormType::kFormType_LAYR: return "LAYR"; case FormType::kFormType_COBJ: return "COBJ"; case FormType::kFormType_OMOD: return "OMOD";
        case FormType::kFormType_MSWP: return "MSWP"; case FormType::kFormType_ZOOM: return "ZOOM"; case FormType::kFormType_INNR: return "INNR";
        case FormType::kFormType_KSSM: return "KSSM"; case FormType::kFormType_AECH: return "AECH"; case FormType::kFormType_SCCO: return "SCCO";
        case FormType::kFormType_AORU: return "AORU"; case FormType::kFormType_SCSN: return "SCSN"; case FormType::kFormType_STAG: return "STAG";
        case FormType::kFormType_NOCM: return "NOCM"; case FormType::kFormType_LENS: return "LENS"; case FormType::kFormType_LSPR: return "LSPR";
        case FormType::kFormType_GDRY: return "GDRY"; case FormType::kFormType_OVIS: return "OVIS";
        default: return "UKWN";
    }
}

const char * _MakeIndent(int depth, std:: string indent = "  ")
{
    if(depth <= 0) return "";
    std::stringstream ss;
    for(int i = 0; i < depth; i++)
    {
        ss << indent;
    }
    return BSFixedString(ss.str().c_str());
}

void _TraceExtraDataList(const char * processId, ExtraDataList * extraDataList, int indent)
{
    if(!extraDataList)
    {
        return;
    }

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);
    const char * i3 = _MakeIndent(indent + 2);

    bool showHeader = false;

    BSExtraData * extraData;
    if(extraDataList->HasType(ExtraDataType::kExtraData_UniqueID))
    {
        extraData = extraDataList->GetByType(ExtraDataType::kExtraData_UniqueID);
        ExtraUniqueID * uniqueId = DYNAMIC_CAST(extraData, BSExtraData, ExtraUniqueID);
        if(uniqueId)
        {
            if(!showHeader)
            {
                _MESSAGE("| %s | %s[ ExtraDataList ]", processId, i1);
                showHeader = true;
            }

            _MESSAGE("| %s | %s[ ExtraUniqueID ]", processId, i2);
            _MESSAGE("| %s | %sUniqueID  : %08X", processId, i3, uniqueId->uniqueId);
            _MESSAGE("| %s | %sunk1A     : [D=%d, H=%08X, B=%s]", processId, i3, uniqueId->unk1A, uniqueId->unk1A, _FlagsToBinaryString(uniqueId->unk1A));
            _MESSAGE("| %s | %sFormOwner : %08X", processId, i3, uniqueId->formOwner);
        }
    }

    if(extraDataList->HasType(ExtraDataType::kExtraData_Flags))
    {
        extraData = extraDataList->GetByType(ExtraDataType::kExtraData_Flags);
        ExtraFlags * flags = DYNAMIC_CAST(extraData, BSExtraData, ExtraFlags);
        if(flags)
        {
            if(!showHeader)
            {
                _MESSAGE("| %s | %s[ ExtraDataList ]", processId, i1);
                showHeader = true;
            }

            _MESSAGE("| %s | %s[ ExtraFlags ]", processId, i2);
            _MESSAGE("| %s | %sFlags: [D=%d, B=%s]", processId, i3, flags->flags, _FlagsToBinaryString(flags->flags));
        }
    }

    if(extraDataList->HasType(ExtraDataType::kExtraData_ObjectInstance))
    {
        extraData = extraDataList->GetByType(ExtraDataType::kExtraData_ObjectInstance);
        BGSObjectInstanceExtra * objectInstanceData = DYNAMIC_CAST(extraData, BSExtraData, BGSObjectInstanceExtra);
        if(objectInstanceData && objectInstanceData->data && objectInstanceData->data->forms)
        {
            if(!showHeader)
            {
                _MESSAGE("| %s | %s[ ExtraDataList ]", processId, i1);
                showHeader = true;
            }

            _MESSAGE("| %s | %s[ BGSObjectInstanceExtra ]", processId, i2);

            const char * i4 = _MakeIndent(indent + 3);

            int count = 0;
            BGSObjectInstanceExtra::Data * data = objectInstanceData->data;
            for(UInt32 i = 0; i < (data->blockSize / sizeof(BGSObjectInstanceExtra::Data::Form)); i++)
            {
                _MESSAGE("| %s | %s[ InstanceData_%d ]", processId, i3, count);
                _MESSAGE("| %s | %sID    : %08X", processId, i4, data->forms[i].formId);
                _MESSAGE("| %s | %sunk04 : [D=%d, H=%08X, B=%s]", processId, i4, data->forms[i].unk04, data->forms[i].unk04, _FlagsToBinaryString(data->forms[i].unk04));
                count++;
            }
        }
    }

    if(showHeader && indent == 0)
    {
        _MESSAGE("| %s | %s[ ============= ]", processId, i1);
    }
}

void _TraceTESForm(const char * processId, TESForm * form, int indent)
{
    if(!form)
    {
        return;
    }

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);

    TESFullName * fullName = DYNAMIC_CAST(form, TESForm, TESFullName);

    _MESSAGE("| %s | %s[ TESForm ]", processId, i1);
    _MESSAGE("| %s | %sName  : %s", processId, i2, fullName ? fullName->name.c_str() : form->GetFullName());
    _MESSAGE("| %s | %sID    : %08X", processId, i2, form->formID);
    _MESSAGE("| %s | %sType  : %s", processId, i2, _FormTypeToString(form->formType));
    _MESSAGE("| %s | %sFlags : [D=%d, B=%s]", processId, i2, form->flags, _FlagsToBinaryString(form->flags));

    if(indent == 0)
    {
        _MESSAGE("| %s | %s[ ======= ]", processId, i1);
    }
}

void _TraceTESObjectCELL(const char * processId, TESObjectCELL * cell, int indent)
{
    if(!cell)
    {
        return;
    }

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);

    _MESSAGE("| %s | %s[ TESObjectCELL ]", processId, i1);
    _MESSAGE("| %s | %sName  : %s", processId, i2, cell->GetFullName());
    _MESSAGE("| %s | %sID    : %08X", processId, i2, cell->formID);
    _MESSAGE("| %s | %sFlags : [D=%d, B=%s]", processId, i2, cell->flags, _FlagsToBinaryString(cell->flags));
    _MESSAGE("| %s | %sunk30 : [D=%d, B=%s]", processId, i2, cell->unk30, _FlagsToBinaryString(cell->unk30));
    _MESSAGE("| %s | %sunk38 : [D=%d, B=%s]", processId, i2, cell->unk38, _FlagsToBinaryString(cell->unk38));
    _MESSAGE("| %s | %sunk42 : [D=%d, B=%s]", processId, i2, cell->unk42, _FlagsToBinaryString(cell->unk42));
    _MESSAGE("| %s | %sunk44 : [D=%d, B=%s]", processId, i2, cell->unk44, _FlagsToBinaryString(cell->unk44));
    _MESSAGE("| %s | %sunk60 : [D=%d, B=%s]", processId, i2, cell->unk60, _FlagsToBinaryString(cell->unk60));
    _MESSAGE("| %s | %sunk64 : [F=%f]", processId, i2, cell->unk64);
    _MESSAGE("| %s | %sunk88 : [D=%d, B=%s]", processId, i2, cell->unk88, _FlagsToBinaryString(cell->unk88));
    _MESSAGE("| %s | %sunkD0 : [D=%d, B=%s]", processId, i2, cell->unkD0, _FlagsToBinaryString(cell->unkD0));
    _MESSAGE("| %s | %sunkD8 : [D=%d, B=%s]", processId, i2, cell->unkD8, _FlagsToBinaryString(cell->unkD8));
    _MESSAGE("| %s | %sunkE0 : [D=%d, B=%s]", processId, i2, cell->unkE0, _FlagsToBinaryString(cell->unkE0));
    _MESSAGE("| %s | %sunkEC : [D=%d, B=%s]", processId, i2, cell->unkEC, _FlagsToBinaryString(cell->unkEC));

    _TraceExtraDataList(processId, cell->extraDataList, indent + 1);

    if(indent == 0)
    {
        _MESSAGE("| %s | %s[ ============= ]", processId, i1);
    }
}

void _TraceTESObjectREFR(const char * processId, TESObjectREFR * ref, int indent)
{
    if(!ref)
    {
        return;
    }

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);

    NiPoint3 pos = ref->pos;

    _MESSAGE("| %s | %s[ TESObjectREFR ]", processId, i1);
    _MESSAGE("| %s | %sName  : %s", processId, i2, CALL_MEMBER_FN(ref, GetReferenceName)());
    _MESSAGE("| %s | %sID    : %08X", processId, i2, ref->formID);
    _MESSAGE("| %s | %sPos   : [X=%f, Y=%f, Z=%f]", processId, i2, pos.x, pos.y, pos.z);
    _MESSAGE("| %s | %sFlags : [D=%d, B=%s]", processId, i2, ref->flags, _FlagsToBinaryString(ref->flags));
    //_MESSAGE("| %s | %sHandle: %08X", processId, i2, ref->CreateRefHandle());

    _TraceTESForm(processId, ref->baseForm, indent + 1);

    TESObjectREFR::LoadedData * loadedData = ref->unkF0;
    if(loadedData && false)
    {
        const char * i3 = _MakeIndent(indent + 2);
        _MESSAGE("| %s | %s[ Loaded Data ]", processId, i2);
        _MESSAGE("| %s | %sFlags : [D=%d, B=%s]", processId, i3, loadedData->flags, _FlagsToBinaryString(loadedData->flags));
        _MESSAGE("| %s | %sunk00 : [D=%d, B=%s]", processId, i3, loadedData->unk00, _FlagsToBinaryString(loadedData->unk00));
        _MESSAGE("| %s | %sunk10 : [D=%d, B=%s]", processId, i3, loadedData->unk10, _FlagsToBinaryString(loadedData->unk10));
        _MESSAGE("| %s | %sunk18 : [D=%d, B=%s]", processId, i3, loadedData->unk18, _FlagsToBinaryString(loadedData->unk18));
    }

    _TraceExtraDataList(processId, ref->extraDataList, indent + 1);

    if(indent == 0)
    {
        _MESSAGE("| %s | %s[ ============= ]", processId, i1);
    }
}

void _TraceBGSInventoryItem(const char * processId, BGSInventoryItem * item, int indent)
{
    if(!item)
    {
        return;
    }

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);

    TESFullName * fullName = DYNAMIC_CAST(item->form, TESForm, TESFullName);

    _MESSAGE("| %s | %s[ BGSInventoryItem ]", processId, i1);

    _TraceTESForm(processId, item->form, indent + 1);

    const char * i3 = _MakeIndent(indent + 2);

    item->stack->Visit([&](BGSInventoryItem::Stack * stack) {
        _MESSAGE("| %s | %s[ Stack ]", processId, i2);
        _MESSAGE("| %s | %sCount    : %d", processId, i3, stack->count);
        _MESSAGE("| %s | %sRefCount : %d", processId, i3, stack->m_refCount);
        _MESSAGE("| %s | %sFlags    : [D=%d, B=%s]", processId, i3, stack->flags, _FlagsToBinaryString(stack->flags));

        _TraceExtraDataList(processId, stack->extraData, indent + 2);

        return true;
    });

    if(indent == 0)
    {
        _MESSAGE("| %s | %s[ ================ ]", processId, i1);
    }
}

std::map<UInt32, UInt32> _referenceIdByFlags;

void _TraceReferenceFlags(const char * processId, TESObjectREFR * ref, int indent)
{
    if(!ref)
    {
        return;
    }

    UInt32 referenceId = ref->formID;
    UInt32 flags = ref->flags;

    if(_referenceIdByFlags[referenceId] == flags)
    {
        return;
    }

    _referenceIdByFlags[referenceId] = flags;

    const char * i1 = _MakeIndent(indent);
    const char * i2 = _MakeIndent(indent + 1);
    const char * i3 = _MakeIndent(indent + 2);

    _MESSAGE("| %s | %s[ Reference Flags ]", processId, i1);

    _MESSAGE("| %s | %sFlags      : %s", processId, i2, _FlagsToBinaryString(flags));

    _MESSAGE("| %s | %s[ Digits ]", processId, i2);
    _MESSAGE("| %s | %sDigit  1 : %d", processId, i3, (flags & 1 << 0) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  2 : %d", processId, i3, (flags & 1 << 1) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  3 : %d", processId, i3, (flags & 1 << 2) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  4 : %d", processId, i3, (flags & 1 << 3) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  5 : %d", processId, i3, (flags & 1 << 4) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  6 : %d", processId, i3, (flags & 1 << 5) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  7 : %d", processId, i3, (flags & 1 << 6) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  8 : %d", processId, i3, (flags & 1 << 7) ? 1 : 0);
    _MESSAGE("| %s | %sDigit  9 : %d", processId, i3, (flags & 1 << 8) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 10 : %d", processId, i3, (flags & 1 << 9) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 11 : %d", processId, i3, (flags & 1 << 10) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 12 : %d", processId, i3, (flags & 1 << 11) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 13 : %d", processId, i3, (flags & 1 << 12) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 14 : %d", processId, i3, (flags & 1 << 13) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 15 : %d", processId, i3, (flags & 1 << 14) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 16 : %d", processId, i3, (flags & 1 << 15) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 17 : %d", processId, i3, (flags & 1 << 16) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 18 : %d", processId, i3, (flags & 1 << 17) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 19 : %d", processId, i3, (flags & 1 << 18) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 20 : %d", processId, i3, (flags & 1 << 19) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 21 : %d", processId, i3, (flags & 1 << 20) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 22 : %d", processId, i3, (flags & 1 << 21) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 23 : %d", processId, i3, (flags & 1 << 22) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 24 : %d", processId, i3, (flags & 1 << 23) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 25 : %d", processId, i3, (flags & 1 << 24) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 26 : %d", processId, i3, (flags & 1 << 25) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 27 : %d", processId, i3, (flags & 1 << 26) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 28 : %d", processId, i3, (flags & 1 << 27) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 29 : %d", processId, i3, (flags & 1 << 28) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 30 : %d", processId, i3, (flags & 1 << 29) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 31 : %d", processId, i3, (flags & 1 << 30) ? 1 : 0);
    _MESSAGE("| %s | %sDigit 32 : %d", processId, i3, (flags & 1 << 31) ? 1 : 0);

    if(indent == 0)
    {
        _MESSAGE("| %s | %s[ =============== ]", processId, i1);
    }
}

// Generate and return a random process ID (a 10-digit random hexadecimal string)
// Pages used for reference: https://stackoverflow.com/questions/12110209/how-to-fill-a-string-with-random-hex-characters#answer-12110369
const char * _GetRandomProcessID()
{
    const char * hex = "0123456789ABCDEF";
    std::random_device rand;

    std::stringstream ss;
    for(int i = 0 ; i < 10; i++)
    {
        ss << hex[rand() % 16];
    }

    return BSFixedString(ss.str().c_str());
}

#endif
