/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef MediaInfo_CountMessagesH
#define MediaInfo_CountMessagesH

#include "MediaInfo/CountPlural.h"
#include "ZenLib/Translation.h"
#include "ZenLib/ZtringListList.h"

namespace MediaInfoLib
{
namespace CountMessages
{
using ZenLib::Ztring;
using ZenLib::Translation;

inline Ztring Lookup(const Translation& Catalog, const Ztring& Key)
{
    Translation::const_iterator It=Catalog.find(Key);
    return It==Catalog.end()?Ztring():It->second;
}

inline std::string Locale(const Translation& Catalog)
{
    std::string Result=Lookup(Catalog, __T("  Language_ISO639")).To_UTF8();
    for (size_t Pos=0; Pos<Result.size(); ++Pos)
    {
        if (Result[Pos]>='A' && Result[Pos]<='Z') Result[Pos]+='a'-'A';
        if (Result[Pos]=='_') Result[Pos]='-';
    }
    // Historical MediaInfo catalog identifiers.
    if (Result=="gr") Result="el";
    if (Result=="pt") Result="pt-PT";
    return Result;
}

// Catalog rules are compiled when the language is loaded, before formatting counts.
struct CatalogRules
{
    CountPlural::Rules Plurals;
    bool HasRules;
    bool LegacyDecimals;
    int Legacy[6];

    CatalogRules() : HasRules(false), LegacyDecimals(false)
    {
        for (size_t Pos=0; Pos<6; ++Pos) Legacy[Pos]=0;
    }

    void Load(const Translation& Catalog)
    {
        *this=CatalogRules();
        if (Lookup(Catalog, __T("  Config_Text_PluralRules"))!=__T("1")) return;
        const Ztring Prefix=__T("  Config_Text_PluralRule.");
        for (Translation::const_iterator It=Catalog.begin(); It!=Catalog.end(); ++It)
            if (It->first.find(Prefix)==0)
            {
                bool Known=false;
                for (size_t Pos=0; Pos<5; ++Pos)
                    if (It->first==Prefix+Ztring().From_UTF8(CountPlural::Name((CountPlural::Category)Pos))) Known=true;
                if (!Known) return;
            }
        for (size_t Pos=0; Pos<5; ++Pos)
        {
            CountPlural::Category Category=(CountPlural::Category)Pos;
            if (!Plurals.Set(Category, Lookup(Catalog, Prefix+Ztring().From_UTF8(CountPlural::Name(Category))).To_UTF8())) return;
        }
        const std::string Mapping=Lookup(Catalog, __T("  Config_Text_PluralLegacy")).To_UTF8();
        if (!Mapping.empty())
        {
            unsigned Seen=0;
            for (size_t Start=0;;)
            {
                size_t End=Mapping.find(',', Start);
                const std::string Item=Mapping.substr(Start, End==std::string::npos?End:End-Start);
                const size_t Equal=Item.find('=');
                if (Equal==std::string::npos || Equal+2!=Item.size() || Item[Equal+1]<'0' || Item[Equal+1]>'3') return;
                size_t Category=0;
                while (Category<6 && Item.substr(0, Equal)!=CountPlural::Name((CountPlural::Category)Category)) ++Category;
                if (Category==6 || (Seen&(1U<<Category))) return;
                Seen|=1U<<Category;
                Legacy[Category]=Item[Equal+1]-'0';
                if (End==std::string::npos) break;
                Start=End+1;
            }
            if (!(Seen&(1U<<CountPlural::Other))) return;
            for (size_t Pos=0; Pos<6; ++Pos)
                if (!(Seen&(1U<<Pos))) Legacy[Pos]=Legacy[CountPlural::Other];
        }
        const Ztring Decimals=Lookup(Catalog, __T("  Config_Text_PluralLegacyDecimals"));
        if (!Decimals.empty() && Decimals!=__T("0") && Decimals!=__T("1")) return;
        LegacyDecimals=Decimals==__T("1");
        HasRules=true;
    }

    CountPlural::Result Select(const std::string& Locale, const std::string& Count) const
    {
        return HasRules?Plurals.Select(Count):CountPlural::Select(Locale, Count);
    }
};

// Only named arguments are interpreted. Inserted metadata is never parsed again.
inline bool Expand(const Ztring& Pattern, const Ztring& Count, const Ztring& Formats, bool WithFormats, Ztring& Result)
{
    Result.clear();
    unsigned Counts=0, FormatLists=0;
    for (size_t Pos=0; Pos<Pattern.size();)
    {
        ZenLib::Char C=Pattern[Pos++];
        if (C==__T('{') || C==__T('}'))
        {
            if (Pos<Pattern.size() && Pattern[Pos]==C)
            {
                Result+=C;
                ++Pos;
            }
            else if (C==__T('{'))
            {
                size_t End=Pattern.find(__T('}'), Pos);
                if (End==Ztring::npos) return false;
                Ztring Name=Pattern.substr(Pos, End-Pos);
                if (Name==__T("count")) { Result+=Count; ++Counts; }
                else if (WithFormats && Name==__T("formats")) { Result+=Formats; ++FormatLists; }
                else return false;
                Pos=End+1;
            }
            else return false;
        }
        else Result+=C;
    }
    return Counts==1 && FormatLists==(WithFormats?1U:0U);
}

inline Ztring Number(const Ztring& Count, const Translation& Catalog)
{
    Ztring Result=Count;
    size_t Dot=Result.find(__T('.'));
    if (Dot==Ztring::npos) Dot=Result.size();
    else
    {
        Ztring Separator=Lookup(Catalog, __T("  Config_Text_FloatSeparator"));
        if (!Separator.empty()) Result.replace(Dot, 1, Separator);
    }
    // Keep the existing number-presentation policy; plural selection precedes it.
    if (Dot>3 && Result[0]==__T('-')) --Dot;
    if (Dot>3)
    {
        Translation::const_iterator Separator=Catalog.find(__T("  Config_Text_ThousandsSeparator"));
        Result.insert(Dot-3, Separator==Catalog.end()?__T(" "):Separator->second);
    }
    return Result;
}

inline Ztring Invariant(const Translation& Catalog, const Ztring& Unit)
{
    Ztring Text=Lookup(Catalog, Unit);
    if (!Text.empty()) return Text;
    Text=Lookup(Catalog, Unit+__T('1'));
    if (!Text.empty() && Text==Lookup(Catalog, Unit+__T('2')) && Text==Lookup(Catalog, Unit+__T('3')))
        return Text;
    return Ztring();
}

inline int LegacyForm(const std::string& Locale, CountPlural::Category Category)
{
    std::string Base=Locale.substr(0, Locale.find_first_of("-_"));
    if (Base=="cs" || Base=="sk" || Base=="pl" || Base=="ru" || Base=="be" || Base=="uk" || Base=="hr" || Base=="lt" || Base=="ro")
    {
        // These catalogs distinguish one, few and the remaining integer forms.
        if (Category==CountPlural::One) return 1;
        if (Category==CountPlural::Few) return 2;
        return 3;
    }
    // Other legacy catalogs provide a binary distinction (or invariant forms).
    return Category==CountPlural::One?1:2;
}

inline bool Pattern(const Translation& Catalog, const Ztring& Key, CountPlural::Category Category,
                    const Ztring& Count, const Ztring& Formats, bool WithFormats, Ztring& Result)
{
    Ztring Other=Lookup(Catalog, Key+__T(".other"));
    if (Other.empty()) return false;
    Ztring Text=Lookup(Catalog, Key+__T('.')+Ztring().From_UTF8(CountPlural::Name(Category)));
    if (Text.empty()) Text=Other;
    return Expand(Text, Number(Count, Catalog), Formats, WithFormats, Result);
}

inline bool Unit(const Translation& Catalog, const CatalogRules& Rules, const std::string& Locale, const Ztring& Count,
                 const Ztring& Name, bool AlwaysSame, Ztring& Result)
{
    CountPlural::Result Plural=Rules.Select(Locale, Count.To_UTF8());
    Ztring Same=Invariant(Catalog, Name);
    if (!Plural.Valid)
    {
        // Legacy frame-rate expressions are labels, not numbers to evaluate.
        if (Count.empty() || Count.find_first_not_of(__T("0123456789.+-/*() "))!=Ztring::npos || Same.empty())
            return false;
        Result=Count+Same;
        return true;
    }
    if (AlwaysSame)
    {
        Ztring Text=Lookup(Catalog, Name);
        if (Text.empty()) Text=Same;
        if (Text.empty()) return false;
        Result=Number(Count, Catalog)+Text;
        return true;
    }
    Ztring Zero=Lookup(Catalog, Name+__T('0'));
    if (Count==__T("0") && !Zero.empty())
    {
        Result=Zero;
        return true;
    }
    if (Plural.KnownLocale && Pattern(Catalog, Name, Plural.Value, Count, Ztring(), false, Result)) return true;
    if (!Same.empty())
    {
        Result=Number(Count, Catalog)+Same;
        return true;
    }
    if (!Plural.KnownLocale) return false;
    std::string Base=Locale.substr(0, Locale.find_first_of("-_"));
    // These legacy triplets have no fractional form. A named message is needed.
    if (Count.find(__T('.'))!=Ztring::npos &&
        (Rules.HasRules?!Rules.LegacyDecimals:
        (Base=="cs" || Base=="sk" || Base=="pl" || Base=="ru" || Base=="be" || Base=="uk" || Base=="hr")))
        return false;
    const int Form=Rules.HasRules?Rules.Legacy[Plural.Value]:LegacyForm(Locale, Plural.Value);
    if (!Form) return false;
    Ztring Text=Lookup(Catalog, Name+ZenLib::Char(__T('0')+Form));
    if (Text.empty()) return false;
    Result=Number(Count, Catalog)+Text;
    return true;
}

inline Ztring FormatUnit(const Translation& Selected, const Translation& English,
                         const CatalogRules& SelectedRules, const CatalogRules& EnglishRules, const Ztring& Count, const Ztring& Name, bool AlwaysSame)
{
    Ztring Result;
    if (Unit(Selected, SelectedRules, Locale(Selected), Count, Name, AlwaysSame, Result) ||
        Unit(English, EnglishRules, "en", Count, Name, AlwaysSame, Result))
        return Result;
    // Measures may be literal symbols absent from catalogs (e.g. TiB or cd/m2).
    if (AlwaysSame || (Lookup(Selected, Name+__T('1')).empty() && Lookup(English, Name+__T('1')).empty()))
        return (CountPlural::Select("en", Count.To_UTF8()).Valid?Number(Count, Selected):Count)+Name;
    return Count;
}

inline Ztring Format(const Translation& Selected, const Translation& English,
                     const CatalogRules& SelectedRules, const CatalogRules& EnglishRules, const ZenLib::ZtringListList& Request)
{
    Ztring Message, Count, Kind, Formats;
    unsigned Seen=0;
    for (size_t Pos=0; Pos<Request.size(); ++Pos)
    {
        if (Request[Pos].empty() || Request[Pos].size()>2) return Ztring();
        const Ztring& Key=Request[Pos][0];
        Ztring Value=Request[Pos].size()>1?Request[Pos][1]:Ztring();
        unsigned Bit=0;
        if (Key==__T("Message")) { Message=Value; Bit=1; }
        else if (Key==__T("Count")) { Count=Value; Bit=2; }
        else if (Key==__T("Kind")) { Kind=Value; Bit=4; }
        else if (Key==__T("Formats")) { Formats=Value; Bit=8; }
        else return Ztring();
        if (Seen&Bit) return Ztring();
        Seen|=Bit;
    }
    bool IsFile=Message==__T("FileCount");
    bool WithFormats=Message==__T("StreamSummary");
    bool More=Message==__T("StreamSummaryMore");
    if (!IsFile && !WithFormats && !More && Message!=__T("StreamCount")) return Ztring();
    if (Seen!=(IsFile?3U:WithFormats?15U:7U) || Count.empty() || Count.find_first_not_of(__T("0123456789"))!=Ztring::npos)
        return Ztring();

    Ztring Noun;
    if (IsFile) Noun=__T(" file");
    else if (Kind==__T("Audio")) Noun=__T(" audio stream");
    else if (Kind==__T("Video")) Noun=__T(" video stream");
    else if (Kind==__T("Text")) Noun=__T(" text stream");
    else if (Kind==__T("Image")) Noun=__T(" image stream");
    else if (Kind==__T("Other")) Noun=__T(" other stream");
    else if (Kind==__T("Menu")) Noun=__T(" menu stream");
    else return Ztring();

    std::string SelectedLocale=Locale(Selected);
    CountPlural::Result Plural=SelectedRules.Select(SelectedLocale, Count.To_UTF8());
    Ztring Key=Message+(IsFile?Ztring():__T(".")+Kind);
    Ztring Result;
    if (Plural.KnownLocale && Pattern(Selected, Key, Plural.Value, Count, Formats, WithFormats, Result))
        return Result;

    // Legacy fragments are resolved as a family, without borrowing English suffixes.
    bool Local=Unit(Selected, SelectedRules, SelectedLocale, Count, Noun, false, Result);
    Ztring Below=Local?Lookup(Selected, __T("see below")):Ztring();
    if (More && Below.empty()) Local=false;
    if (!Local)
    {
        CountPlural::Result En=EnglishRules.Select("en", Count.To_UTF8());
        if (Pattern(English, Key, En.Value, Count, Formats, WithFormats, Result)) return Result;
        if (!Unit(English, EnglishRules, "en", Count, Noun, false, Result))
            Result=Number(Count, English)+Noun+(En.Value==CountPlural::One?Ztring():__T("s"));
        Below=__T("see below");
    }
    if (WithFormats || More)
    {
        Ztring Key=WithFormats?__T(": "):__T(", ");
        Ztring Separator=Local?Lookup(Selected, Key):Ztring();
        if (Separator.empty()) Separator=Key;
        Result+=Separator+(WithFormats?Formats:Below);
    }
    return Result;
}
}
}
#endif
