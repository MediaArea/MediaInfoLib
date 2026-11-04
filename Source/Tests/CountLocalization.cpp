/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/CountPlural.h"
#include "ZenLib/ZtringListList.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

using namespace MediaInfoLib;
using namespace ZenLib;

namespace
{
size_t Checks=0;
size_t Failures=0;

Ztring Utf8(const std::string& Value)
{
    return Ztring().From_UTF8(Value);
}

void Equal(const char* Label, const Ztring& Actual, const Ztring& Expected)
{
    ++Checks;
    if (Actual==Expected)
        return;
    ++Failures;
    std::cerr << Label << ": expected [" << Expected.To_UTF8()
              << "], got [" << Actual.To_UTF8() << "]\n";
}

void Check(const char* Label, bool Value)
{
    Equal(Label, Value ? __T("true") : __T("false"), __T("true"));
}

Ztring Catalog(const std::string& Directory, const char* Locale)
{
    std::ifstream Input((Directory+"/"+Locale+".csv").c_str(), std::ios::binary);
    Check("catalog available", Input.good());
    return Utf8(std::string(std::istreambuf_iterator<char>(Input), std::istreambuf_iterator<char>()));
}

void Load(const Ztring& Catalog)
{
    MediaInfo::Option_Static(__T("Language"), Catalog);
}

Ztring Message(const char* Name, const Ztring& Count, const char* Kind="Audio", const Ztring& Formats=__T("AAC"))
{
    ZtringListList Request;
    Request(0, 0)=__T("Message"); Request(0, 1)=Utf8(Name);
    Request(1, 0)=__T("Count"); Request(1, 1)=Count;
    if (std::string(Name)!="FileCount")
    {
        Request(2, 0)=__T("Kind"); Request(2, 1)=Utf8(Kind);
        if (std::string(Name)=="StreamSummary")
        {
            Request(3, 0)=__T("Formats"); Request(3, 1)=Formats;
        }
    }
    return MediaInfo::Option_Static(__T("Language_Format"), Request.Read());
}

Ztring Unit(const Ztring& Count, const Ztring& Name=__T(" channel"), bool Invariant=false)
{
    return Config.Language_Get(Count, Name, Invariant);
}

void Czech(const Ztring& Cs)
{
    Load(Cs);
    static const unsigned Counts[]={0,1,2,3,4,5,6,10,11,12,14,21,22,23,24,25,101,102};
    for (size_t I=0; I<sizeof(Counts)/sizeof(*Counts); ++I)
    {
        const unsigned N=Counts[I];
        const Ztring Number=Ztring::ToZtring(N);
        const size_t Form=N==1 ? 0 : N>=2 && N<=4 ? 1 : 2;
        static const Char* Audio[]={__T(" zvukov\u00FD stream"), __T(" zvukov\u00E9 streamy"), __T(" zvukov\u00FDch stream\u016F")};
        static const Char* Text[]={__T(" textov\u00FD stream"), __T(" textov\u00E9 streamy"), __T(" textov\u00FDch stream\u016F")};
        static const Char* Files[]={__T(" soubor"), __T(" soubory"), __T(" soubor\u016F")};
        static const Char* Channels[]={__T(" kan\u00E1l"), __T(" kan\u00E1ly"), __T(" kan\u00E1l\u016F")};
        Equal("Czech audio stream count", Message("StreamCount", Number), Number+Audio[Form]);
        Equal("Czech text stream count", Message("StreamCount", Number, "Text"), Number+Text[Form]);
        Equal("Czech file count", Message("FileCount", Number), Number+Files[Form]);
        Equal("Czech channel count", Unit(Number), Number+Channels[Form]);
    }
    Equal("Czech full summary", Message("StreamSummary", __T("6")), __T("6 zvukov\u00FDch stream\u016F: AAC"));
    Equal("empty formats", Message("StreamSummary", __T("6"), "Audio", Ztring()), __T("6 zvukov\u00FDch stream\u016F: "));
    Equal("Czech negative zero", Unit(__T("-0")), __T("-0 kan\u00E1l\u016F"));
    Equal("Czech decimal falls back as whole English unit", Unit(__T("1.5")), __T("1.5 channels"));
    Equal("Czech visible fraction digits", Unit(__T("1.0")), __T("1.0 channels"));
    Equal("invalid numeric text", Unit(__T("unknown")), __T("unknown"));
    Equal("invalid numeric exponent", Unit(__T("1e2")), __T("1e2"));
    Equal("legacy expression inflected unit", Unit(__T("2 / 3")), __T("2 / 3"));
    Equal("invariant expression is literal", Unit(__T("2 / 3"), __T(" dB")), __T("2 / 3 dB"));
    Equal("explicit zero message", Unit(__T("0"), __T(" dB")), __T("0 dB"));
    Equal("whole zero message", Unit(__T("0"), __T(" warppoint")), __T("\u017D\u00E1dn\u00E9 body deformace"));
    Equal("frame rate expression", Unit(__T("29.97 (30000/1001)"), __T(" fps")), __T("29.97 (30000/1001) FPS"));
    Equal("legacy literal unit", Unit(__T("6"), __T(" unknown-unit")), __T("6 unknown-unit"));
    Equal("uncatalogued invariant TiB", Unit(__T("6"), __T(" TiB"), true), __T("6 TiB"));
    Equal("uncatalogued luminance unit", Unit(__T("6"), __T(" cd/m2")), __T("6 cd/m2"));
}

void Categories(const Ztring& Catalog, const char* Locale, const unsigned* Counts, const char* const* Expected, size_t Size)
{
    // Marker patterns test the production lookup and selector without inventing translations.
    ZtringListList Marked(Catalog);
    static const char* Categories[]={"zero", "one", "two", "few", "many", "other"};
    for (size_t I=0; I<6; ++I)
        Marked(Utf8(std::string("FileCount.")+Categories[I]))=Utf8(std::string("{count} ")+Categories[I]);
    Load(Marked.Read());
    for (size_t I=0; I<Size; ++I)
    {
        const Ztring Count=Ztring::ToZtring(Counts[I]);
        Equal(Locale, Message("FileCount", Count), Count+__T(" ")+Utf8(Expected[I]));
    }
}

Ztring MarkedUnits(const Ztring& Text)
{
    ZtringListList Marked(Text);
    for (size_t I=0; I<6; ++I)
    {
        const char* Category=CountPlural::Name((CountPlural::Category)I);
        Marked(Utf8(std::string(" channel.")+Category))=Utf8(std::string("{count} ")+Category);
    }
    Marked(__T("  Config_Text_ThousandsSeparator"))=__T("");
    Marked(__T("  Config_Text_FloatSeparator"))=__T(".");
    // A missing or invalid catalog rule must fail instead of using the locale fallback.
    Marked(__T("  Language_ISO639"))=__T("zz");
    return Marked.Read();
}

void Selector(const std::string& Directory)
{
    using namespace CountPlural;
    struct Case { const char* Locale; const char* Count; Category Expected; };
    // Fixed examples exercise boundaries, visible decimals and language differences.
    static const Case Cases[]={
        {"en","0",Other}, {"en","1",One}, {"en","2",Other},
        {"en","1.0",Other}, {"en","0001",One}, {"en","+1",One}, {"en","-1",One},
        {"de","1.00",Other}, {"gl","1.0",Other}, {"nl","1",One}, {"sv","1.0",Other},
        {"bg","1.0",One}, {"el","1.00",One}, {"eu","1.5",Other},
        {"hu","1.0",One}, {"ka","1",One}, {"sq","1.0",One}, {"tr","1.0",One},
        {"cs","0",Other}, {"cs","1",One}, {"cs","4",Few}, {"cs","5",Other},
        {"cs","21",Other}, {"cs","22",Other}, {"cs","1.0",Many}, {"cs","2.50",Many},
        {"sk","2",Few}, {"sk","5.0",Many},
        {"pl","1",One}, {"pl","12",Many}, {"pl","14",Many}, {"pl","21",Many},
        {"pl","22",Few}, {"pl","24",Few}, {"pl","25",Many}, {"pl","101",Many},
        {"pl","102",Few}, {"pl","1.0",Other},
        {"ru","11",Many}, {"ru","21",One}, {"ru","22",Few}, {"ru","25",Many},
        {"ru","101",One}, {"ru","102",Few}, {"ru","-21",One}, {"ru","1.0",Other},
        {"ru","18446744073709551621",One}, {"ru","18446744073709551622",Few},
        {"ru","18446744073709551611",Many}, {"uk","21",One}, {"uk","22",Few},
        {"be","1.0",One}, {"be","2.0",Few}, {"be","11.0",Many}, {"be","1.1",Other},
        {"hr","1",One}, {"hr","2",Few}, {"hr","11",Other}, {"hr","21",One},
        {"hr","1.1",One}, {"hr","1.2",Few}, {"hr","1.11",Other},
        {"hr","1.20",Other}, {"hr","2.0",Other}, {"hr","0.21",One},
        {"lt","1",One}, {"lt","9",Few}, {"lt","10",Other}, {"lt","11",Other},
        {"lt","19",Other}, {"lt","21",One}, {"lt","22",Few},
        {"lt","1.0",One}, {"lt","1.5",Many}, {"lt","0.01",Many},
        {"ro","0",Few}, {"ro","1",One}, {"ro","19",Few}, {"ro","20",Other},
        {"ro","100",Other}, {"ro","101",Few}, {"ro","119",Few}, {"ro","120",Other},
        {"ro","1.0",Few}, {"ro","2.5",Few},
        {"ar","0",Zero}, {"ar","1",One}, {"ar","2",Two}, {"ar","3",Few},
        {"ar","10",Few}, {"ar","11",Many}, {"ar","99",Many}, {"ar","100",Other},
        {"ar","102",Other}, {"ar","103",Few}, {"ar","111",Many},
        {"ar","1.0",One}, {"ar","3.0",Few}, {"ar","3.1",Other}, {"ar","-0",Zero},
        {"da","0",Other}, {"da","0.5",One}, {"da","1.0",One},
        {"da","1.5",One}, {"da","2.5",Other},
        {"fa","0",One}, {"fa","0.5",One}, {"fa","1.0",One}, {"fa","1.5",Other},
        {"hy","0",One}, {"hy","1.5",One}, {"hy","2",Other},
        {"fr","0",One}, {"fr","1.5",One}, {"fr","2",Other}, {"fr","1000000",Many},
        {"fr","1000001",Other}, {"fr","1000000.0",Other},
        {"es","0",Other}, {"es","1.0",One}, {"es","1000000",Many},
        {"ca","1.0",Other}, {"ca","1000000",Many}, {"it","1.0",Other}, {"it","2000000",Many},
        {"pt-BR","0",One}, {"pt-BR","1.5",One}, {"pt-BR","1000000",Many},
        {"pt-PT","0",Other}, {"pt-PT","1",One}, {"pt-PT","1.0",Other},
        {"pt-Latn-PT","0",Other}, {"pt-Latn-BR","0",One}, {"pt","0",Other},
        {"pt-PT","1000000",Many}, {"pt-PT","1000000.0",Other},
        {"id","1",Other}, {"ja","1",Other}, {"ko","1",Other}, {"th","1",Other}, {"zh","1",Other},
        {"CS_cz","3",Few}, {"RU-ru","21",One}, {"zh-Hant-TW","1",Other},
        {"PT_pt","0",Other}, {"pt-AO","0",Other},
        {"hr","0.0000000000000000000000000000000000000000000000021",One},
        {"hr","0.0000000000000000000000000000000000000000000000011",Other},
        {"lt","0.0000000000000000000000000000000000000000000000001",Many},
        {"en","999999999999999999999999999999999999999999999999999",Other},
        {"en","1.0000000000000000000000000000000000000000000000000",Other}
    };
    std::map<std::string, Ztring> Catalogs;
    for (size_t I=0; I<sizeof(Cases)/sizeof(*Cases); ++I)
    {
        const Case& C=Cases[I];
        const Result Actual=Select(C.Locale, C.Count);
        const std::string Label=std::string("selector ")+C.Locale+" "+C.Count;
        Equal(Label.c_str(), Utf8(!Actual.Valid ? "invalid" : !Actual.KnownLocale ? "unknown locale" : Name(Actual.Value)), Utf8(Name(C.Expected)));

        std::string File=C.Locale;
        for (size_t J=0; J<File.size(); ++J)
        {
            if (File[J]>='A' && File[J]<='Z') File[J]+='a'-'A';
            if (File[J]=='_') File[J]='-';
        }
        if (File.substr(0, 2)=="pt") File=File.find("-br")!=std::string::npos ? "pt-BR" : "pt";
        else if (File.substr(0, 2)=="zh") File="zh-TW";
        else File=File.substr(0, File.find('-'));
        if (File=="el") File="gr";
        if (Catalogs.find(File)==Catalogs.end())
            Catalogs[File]=MarkedUnits(Catalog(Directory, File.c_str()));
        Load(Catalogs[File]);
        Equal(("catalog "+Label).c_str(), Unit(Utf8(C.Count)), Utf8(std::string(C.Count)+" "+Name(C.Expected)));
    }
    static const char* Invalid[]={"", "+", "-", ".1", "1.", " 1", "1 ", "1e2", "1,5", "1/2", "--1", "1.2.3", "NaN", "inf"};
    for (size_t I=0; I<sizeof(Invalid)/sizeof(*Invalid); ++I)
    {
        const Result Actual=Select("en", Invalid[I]);
        const std::string Label=std::string("invalid count [")+Invalid[I]+"]";
        Check(Label.c_str(), !Actual.Valid && Actual.KnownLocale && Actual.Value==Other);
    }
    static const char* Unknown[]={"", "zz", "cy", "en@US", "en US", "en.US", "-en", "en-", "en--US"};
    for (size_t I=0; I<sizeof(Unknown)/sizeof(*Unknown); ++I)
    {
        const Result Actual=Select(Unknown[I], "1");
        const std::string Label=std::string("unsupported locale [")+Unknown[I]+"]";
        Check(Label.c_str(), Actual.Valid && !Actual.KnownLocale && Actual.Value==Other && !KnownLocale(Unknown[I]));
    }
}

void ShippedCatalogs(const std::string& Directory)
{
    struct Case { const char* Locale; const char* Six; };
    static const Case Cases[]={
        {"ar","few"}, {"be","many"}, {"bg","other"}, {"ca","other"}, {"cs","other"},
        {"da","other"}, {"de","other"}, {"en","other"}, {"es","other"}, {"eu","other"},
        {"fa","other"}, {"fr","other"}, {"gl","other"}, {"gr","other"}, {"hr","other"},
        {"hu","other"}, {"hy","other"}, {"id","other"}, {"it","other"}, {"ja","other"},
        {"ka","other"}, {"ko","other"}, {"lt","few"}, {"nl","other"}, {"pl","many"},
        {"pt-BR","other"}, {"pt","other"}, {"ro","few"}, {"ru","many"}, {"sk","other"},
        {"sq","other"}, {"sv","other"}, {"th","other"}, {"tr","other"}, {"uk","many"},
        {"zh-CN","other"}, {"zh-HK","other"}, {"zh-TW","other"}
    };
    // Read actual catalog metadata, including historical gr and pt identifiers.
    const unsigned Counts[]={6};
    for (size_t I=0; I<sizeof(Cases)/sizeof(*Cases); ++I)
    {
        const char* Expected[]={Cases[I].Six};
        const Ztring Text=Catalog(Directory, Cases[I].Locale);
        ZtringListList Metadata(Text);
        Equal("shipped catalog declares plural rules", Metadata(__T("  Config_Text_PluralRules")), __T("1"));
        Categories(Text, Cases[I].Locale, Counts, Expected, 1);
        Load(MarkedUnits(Text));
        std::string Locale=Cases[I].Locale;
        if (Locale=="gr") Locale="el";
        for (unsigned N=0; N<=125; ++N)
        {
            const Ztring Count=Ztring::ToZtring(N);
            const std::string Label="catalog migration "+Locale+" "+Count.To_UTF8();
            const CountPlural::Result Previous=CountPlural::Select(Locale, Count.To_UTF8());
            Equal(Label.c_str(), Unit(Count), Count+__T(" ")+Utf8(CountPlural::Name(Previous.Value)));
        }
        static const char* Edges[]={"0.0", "1.0", "1.1", "1.20", "0.21", "2.5", "1000000", "1000000.0",
            "2000000", "18446744073709551621", "18446744073709551622", "18446744073709551611"};
        for (size_t J=0; J<sizeof(Edges)/sizeof(*Edges); ++J)
        {
            const std::string Label="catalog migration "+Locale+" "+Edges[J];
            const CountPlural::Result Previous=CountPlural::Select(Locale, Edges[J]);
            Equal(Label.c_str(), Unit(Utf8(Edges[J])), Utf8(std::string(Edges[J])+" "+CountPlural::Name(Previous.Value)));
        }
    }
}


void RuleExpressions()
{
    using namespace CountPlural;
    Rules Integer;
    Check("compile integer rule", Integer.Set(One, "v == 0 && i == 1"));
    Check("integer rule singular", Integer.Select("1").Value==One);
    Check("integer rule visible zero", Integer.Select("1.0").Value==Other);
    Check("integer rule invalid count", !Integer.Select("1e2").Valid);
    Check("compile precedence rule", Integer.Set(Few, "i == 2 || i == 3 && v == 0"));
    Check("OR accepts decimal left operand", Integer.Select("2.5").Value==Few);
    Check("AND binds tighter than OR", Integer.Select("3.5").Value==Other);
    Check("AND accepts integer right operand", Integer.Select("3").Value==Few);
    Check("compile grouped rule", Integer.Set(One, "(i == 1 || i == 2) && v == 0"));
    Check("group accepts integer", Integer.Select("2").Value==One);
    Check("group rejects visible fraction", Integer.Select("1.0").Value==Other);
    Check("rules are independent of locale", Integer.Select("1").KnownLocale);

    Rules Priority;
    Check("compile always true", Priority.Set(One, "true"));
    Check("compile earlier category", Priority.Set(Zero, "true"));
    Check("categories have fixed priority", Priority.Select("6").Value==Zero);
    Check("compile always false", Priority.Set(Zero, "false"));
    Check("false category is skipped", Priority.Select("6").Value==One);
    Check("other has no explicit rule", !Priority.Set(Other, "true"));
    Check("invalid category is rejected", !Priority.Set((Category)99, "true"));
    Check("empty expression clears category", Priority.Set(One, ""));
    Check("cleared category defaults to other", Priority.Select("6").Value==Other);

    Rules Fractions;
    Check("compile fractional ending", Fractions.Set(One, "v > 0 && f % 10 == 1 && f % 100 != 11"));
    Check("compile fractional range", Fractions.Set(Few, "v >= 1 && f % 10 >= 2 && f % 10 <= 4 && (f % 100 < 12 || f % 100 > 14)"));
    Check("fraction ending selects one", Fractions.Select("1.21").Value==One);
    Check("fraction ending exception", Fractions.Select("1.11").Value==Other);
    Check("fraction visible trailing zero", Fractions.Select("1.20").Value==Other);
    Check("fraction ending selects few", Fractions.Select("1.22").Value==Few);
    Check("fraction range exception", Fractions.Select("1.12").Value==Other);
    Check("fraction beyond 64 bits", Fractions.Select("0.18446744073709551621").Value==One);
    Check("fraction leading zeros", Fractions.Select("0.0000000000000000000000000000000000000000000000021").Value==One);
    Check("signed fractional rule", Fractions.Select("-1.21").Value==One);
    Check("integer has no visible fraction", Fractions.Select("21").Value==Other);

    Rules Large;
    Check("compile largest allowed modulus", Large.Set(One, "i % 1000000 == 1 && i > 1000000"));
    Check("modulo beyond 64 bits", Large.Select("18446744073709551616000001").Value==One);
    Check("modulo beyond 64 bits mismatch", Large.Select("18446744073709551616000002").Value==Other);
    Check("leading zeros and explicit sign", Large.Select("+0001000001").Value==One);
    Check("modulo boundary", Large.Select("1").Value==Other);
    Check("compile operand comparisons", Large.Set(One, "i >= 1000000 && v <= 2 && f != 5"));
    Check("maximum allowed constant equality", Large.Select("1000000.00").Value==One);
    Check("fraction digit count comparison", Large.Select("1000000.005").Value==Other);
    Check("fraction integer comparison", Large.Select("1000000.05").Value==Other);
    Check("fraction zero without decimal", Large.Select("1000000").Value==One);

    static const char* Invalid[]={
        "i", "1", "i == -1", "i == 1000001", "i % 0 == 1", "i % 1000001 == 1",
        "i % == 1", "i / 2 == 1", "i + 1 == 2", "n == 1", "v = 0", "i == 1 &&",
        "i == 1 ||", "!true", "i == 1 & v == 0", "i == 1 | v == 0", "i == 1;true",
        "i == 1 trailing", "TRUE", "(i == 1", "i == 1)", "i == 1 ? true : false",
        "true false", "i == i", "i == 1 == 1", "i % 2 % 2 == 1",
        "true || invalid", "false && invalid", "true || i % 0 == 1", "i == 18446744073709551616"
    };
    for (size_t I=0; I<sizeof(Invalid)/sizeof(*Invalid); ++I)
    {
        Rules Replaced;
        Replaced.Set(One, "true");
        Replaced.Set(Few, "true");
        const std::string Label=std::string("invalid plural expression [")+Invalid[I]+"]";
        Check(Label.c_str(), !Replaced.Set(One, Invalid[I]));
        Check("invalid replacement clears only its own category", Replaced.Select("1").Value==Few);
    }
    Rules Limits;
    const std::string LongExpression="i == 1"+std::string(1018, ' ');
    Check("expression length limit accepted", Limits.Set(One, LongExpression));
    Check("expression length limit exceeded", !Limits.Set(One, LongExpression+" "));
    Check("parenthesis depth limit accepted", Limits.Set(One, std::string(16, '(')+"true"+std::string(16, ')')));
    Check("parenthesis depth limit exceeded", !Limits.Set(One, std::string(17, '(')+"true"+std::string(17, ')')));
    Check("empty rules default to other", Rules().Select("1").Value==Other);
}

Ztring Markers(const char* Locale, const Ztring& Metadata)
{
    ZtringListList Text;
    Text(__T("  Language_ISO639"))=Utf8(Locale);
    Text(__T("  Config_Text_ThousandsSeparator"))=__T("");
    Text(__T("  Config_Text_FloatSeparator"))=__T(".");
    for (size_t I=0; I<6; ++I)
    {
        const Ztring Category=Utf8(CountPlural::Name((CountPlural::Category)I));
        Text(__T("FileCount.")+Category)=__T("{count} ")+Category;
        Text(__T(" channel.")+Category)=__T("{count} ")+Category;
    }
    ZtringListList Entries(Metadata);
    for (size_t I=0; I<Entries.size(); ++I)
        if (!Entries[I].empty())
            Text(Entries[I][0])=Entries[I].size()>1?Entries[I][1]:Ztring();
    return Text.Read();
}

void CatalogRules()
{
    const Ztring SixIsOne=__T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6");
    Load(Markers("cs", SixIsOne));
    Equal("catalog rules override known locale", Message("FileCount", __T("6")), __T("6 one"));
    Equal("catalog rules replace all language categories", Message("FileCount", __T("2")), __T("2 other"));
    Load(Markers("zz", SixIsOne));
    Equal("catalog rules support unknown language", Message("FileCount", __T("6")), __T("6 one"));
    Equal("unknown language unit uses catalog rules", Unit(__T("6")), __T("6 one"));
    Load(Markers("zz", __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;v == 1 && f == 0\n  Config_Text_PluralRule.few;f % 10 == 2")));
    Equal("catalog visible decimal digits", Unit(__T("1.0")), __T("1.0 one"));
    Equal("catalog visible decimal zero count", Unit(__T("1.00")), __T("1.00 other"));
    Equal("catalog fractional operand", Unit(__T("1.22")), __T("1.22 few"));
    Equal("catalog fractional operand beyond 64 bits", Unit(__T("1.18446744073709551622")), __T("1.18446744073709551622 few"));
    Load(Markers("en", __T("  Config_Text_PluralRules;1")));
    Equal("empty valid rule set means other", Message("FileCount", __T("1")), __T("1 other"));
    Load(Markers("cs", __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;")));
    Equal("empty catalog category is disabled", Message("FileCount", __T("2")), __T("2 other"));

    Load(Markers("cs", SixIsOne));
    Equal("language switch loads first rule", Message("FileCount", __T("6")), __T("6 one"));
    Load(Markers("cs", __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 8")));
    Equal("reloading same locale replaces cached rules", Message("FileCount", __T("6")), __T("6 other"));
    Equal("reloading same locale compiles new rule", Message("FileCount", __T("8")), __T("8 one"));
    Load(Markers("cs", Ztring()));
    Equal("old catalog uses compatibility rules", Message("FileCount", __T("2")), __T("2 few"));
    Equal("old catalog discards prior metadata rules", Message("FileCount", __T("8")), __T("8 other"));
    Load(__T(""));
    Equal("default language clears cached rules", Message("FileCount", __T("1")), __T("1 file"));

    static const Char* Invalid[]={
        __T("  Config_Text_PluralRules;2\n  Config_Text_PluralRule.one;i == 6"),
        __T("  Config_Text_PluralRule.one;i == 6"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralRule.few;i % 0 == 1"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralLegacy;one=1"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralLegacy;one=4,other=3"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralLegacy;one=1,one=2,other=3"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralLegacy;bogus=1,other=3"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralLegacyDecimals;2"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralRule.other;true"),
        __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\n  Config_Text_PluralRule.bogus;true")
    };
    for (size_t I=0; I<sizeof(Invalid)/sizeof(*Invalid); ++I)
    {
        Load(Markers("cs", Invalid[I]));
        Equal("malformed metadata falls back as a whole", Message("FileCount", __T("6")), __T("6 other"));
        Equal("malformed metadata retains old categories", Message("FileCount", __T("2")), __T("2 few"));
    }
    Load(Markers("zz", __T("  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;invalid")));
    Equal("invalid unknown-language rules use English", Message("FileCount", __T("6")), __T("6 files"));

    const Ztring Legacy=__T("  Language_ISO639;zz\n  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;v == 0 && i == 1\n  Config_Text_PluralRule.few;i == 2 || v > 0\n channel1; first\n channel2; second\n channel3; third");
    Load(Legacy);
    Equal("metadata without legacy map uses English legacy family", Unit(__T("1")), __T("1 channel"));
    const Ztring Mapping=__T("\n  Config_Text_PluralLegacy;one=2,few=1,other=3");
    Load(Legacy+Mapping);
    Equal("catalog legacy one mapping", Unit(__T("1")), __T("1 second"));
    Equal("catalog legacy few mapping", Unit(__T("2")), __T("2 first"));
    Equal("catalog legacy other mapping", Unit(__T("6")), __T("6 third"));
    Equal("catalog legacy decimals disabled by default", Unit(__T("1.5")), __T("1.5 channels"));
    Load(Legacy+__T("\n  Config_Text_PluralLegacy;one=0,few=1,other=0"));
    Equal("zero legacy mapping disables singular form", Unit(__T("1")), __T("1 channel"));
    Equal("zero other mapping disables plural form", Unit(__T("6")), __T("6 channels"));
    Equal("zero mapping preserves other categories", Unit(__T("2")), __T("2 first"));
    Load(Legacy+Mapping+__T("\n  Config_Text_PluralLegacyDecimals;1"));
    Equal("catalog explicitly allows legacy decimals", Unit(__T("1.5")), __T("1.5 first"));
    Load(Legacy+Mapping+__T("\n  Config_Text_PluralRule.zero;i == 0"));
    Equal("unmapped legacy category uses other mapping", Unit(__T("0")), __T("0 third"));
    Load(Legacy+Mapping+__T("\n  Config_Text_PluralLegacyDecimals;0\n channel0;NONE"));
    Equal("catalog metadata preserves exact zero message", Unit(__T("0")), __T("NONE"));
    Load(Legacy+__T("\n dB; dB"));
    Equal("catalog metadata preserves invariant unit", Unit(__T("1.5"), __T(" dB")), __T("1.5 dB"));
    Load(__T("  Language_ISO639;zz\n  Config_Text_PluralRules;1\n  Config_Text_PluralRule.one;i == 6\nFileCount.one;{count} LOCAL"));
    Equal("missing message uses English selection", Message("FileCount", __T("6")), __T("6 files"));
    Equal("missing unit uses English selection", Unit(__T("6")), __T("6 channels"));
}

void OtherLanguages(const std::string& Directory)
{
    const Ztring En=Catalog(Directory, "en");
    Load(En);
    Equal("English singular", Unit(__T("1")), __T("1 channel"));
    Equal("English plural", Unit(__T("2")), __T("2 channels"));
    Equal("English full summary", Message("StreamSummary", __T("2")), __T("2 audio streams: AAC"));
    Load(Catalog(Directory, "fr"));
    Equal("French legacy summary punctuation", Message("StreamSummary", __T("6")), __T("6 flux audio : AAC"));
    Equal("French legacy Sheet punctuation", Message("StreamSummaryMore", __T("6")), __T("6 flux audio, Voir ci-dessous"));
    const Ztring Pl=Catalog(Directory, "pl");
    Load(Pl);
    Equal("Polish 12", Unit(__T("12")), __T("12 kana\u0142\u00F3w"));
    Equal("Polish 22", Unit(__T("22")), __T("22 kana\u0142y"));
    const Ztring Ru=Catalog(Directory, "ru");
    Load(Ru);
    Equal("Russian 11", Unit(__T("11")), __T("11 \u043A\u0430\u043D\u0430\u043B\u043E\u0432"));
    Equal("Russian 21", Unit(__T("21")), __T("21 \u043A\u0430\u043D\u0430\u043B"));
    const Ztring Ja=Catalog(Directory, "ja");
    Load(Ja);
    for (unsigned I=0; I<3; ++I)
        Equal("Japanese invariant channel", Unit(Ztring::ToZtring(I)), Ztring::ToZtring(I)+__T(" \u30C1\u30E3\u30F3\u30CD\u30EB"));
    const unsigned EnCounts[]={1,2}; const char* EnForms[]={"one","other"};
    const unsigned PlCounts[]={12,22}; const char* PlForms[]={"many","few"};
    const unsigned RuCounts[]={11,21}; const char* RuForms[]={"many","one"};
    const unsigned ArCounts[]={0,1,2,3,11,100}; const char* ArForms[]={"zero","one","two","few","many","other"};
    const unsigned JaCounts[]={0,1,2}; const char* JaForms[]={"other","other","other"};
    const Ztring Ar=Catalog(Directory, "ar");
    Load(Ar);
    Equal("legacy Arabic approximation singular", Unit(__T("1")), __T("1\u0642\u0646\u0627\u0629"));
    Equal("legacy Arabic approximation plural", Unit(__T("2")), __T("2\u0627\u0644\u0642\u0646\u0648\u0627\u062A"));
    Categories(En, "English", EnCounts, EnForms, 2);
    Categories(Pl, "Polish", PlCounts, PlForms, 2);
    Categories(Ru, "Russian", RuCounts, RuForms, 2);
    Categories(Ar, "Arabic", ArCounts, ArForms, 6);
    Categories(Ja, "Japanese", JaCounts, JaForms, 3);
}

void FallbackAndArguments(const Ztring& Cs)
{
    Load(__T("  Language_ISO639;cs\n channel1; LOCAL\nFileCount.one;{count} LOCAL"));
    Equal("missing Czech unit falls back to English plural", Unit(__T("22")), __T("22 channels"));
    Equal("missing Czech message falls back to English plural", Message("FileCount", __T("22")), __T("22 files"));
    Load(__T("  Language_ISO639;cs\n channel3; LOCAL"));
    Equal("partial catalog inherits default grouping", Unit(__T("1000")), __T("1 000 LOCAL"));
    Load(__T("  Language_ISO639;cs\n  Config_Text_ThousandsSeparator;\n channel3; LOCAL"));
    Equal("explicit empty grouping is preserved", Unit(__T("1000")), __T("1000 LOCAL"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;{bogus}"));
    Equal("unknown placeholder fallback", Message("FileCount", __T("22")), __T("22 files"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;{count"));
    Equal("unclosed placeholder fallback", Message("FileCount", __T("22")), __T("22 files"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;missing count"));
    Equal("missing count placeholder fallback", Message("FileCount", __T("22")), __T("22 files"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;{count} {count}"));
    Equal("repeated placeholder fallback", Message("FileCount", __T("22")), __T("22 files"));
    Load(__T("  Language_ISO639;cs\nStreamSummary.Audio.other;{count} streams"));
    Equal("missing formats placeholder fallback", Message("StreamSummary", __T("22")), __T("22 audio streams: AAC"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;{count} generic"));
    Equal("missing category uses translated other", Message("FileCount", __T("2")), __T("2 generic"));
    Load(__T("  Language_ISO639;cs\nFileCount.other;{{files}} {count}"));
    Equal("escaped braces", Message("FileCount", __T("2")), __T("{files} 2"));
    Equal("invalid negative file count", Message("FileCount", __T("-1")), Ztring());
    Equal("invalid decimal stream count", Message("StreamCount", __T("1.5")), Ztring());
    Equal("unknown stream kind", Message("StreamCount", __T("6"), "Bogus"), Ztring());
    Equal("duplicate argument", MediaInfo::Option_Static(__T("Language_Format"), __T("Message;FileCount\nCount;6\nCount;2")), Ztring());
    Equal("unexpected argument", MediaInfo::Option_Static(__T("Language_Format"), __T("Message;FileCount\nCount;6\nUnused;2")), Ztring());
    Load(__T("  Language_ISO639;zz\n channel1; LOCAL\n channel2; LOCAL\nFileCount.one;{count} LOCAL\nFileCount.other;{count} LOCAL"));
    Equal("unknown language ignores singular pattern", Message("FileCount", __T("1")), __T("1 file"));
    Equal("unknown language ignores plural pattern", Message("FileCount", __T("22")), __T("22 files"));
    Equal("unknown language ignores inflected unit", Unit(__T("22")), __T("22 channels"));
    Load(__T("  Language_ISO639;cy\nFileCount.one;{count} LOCAL\nFileCount.other;{count} LOCAL"));
    Equal("unsupported language falls back to English", Message("FileCount", __T("1")), __T("1 file"));
    Load(Cs);
    const Ztring Literal=__T("AAC; \"{count}\"\n{formats} %s");
    Equal("metadata remains literal", Message("StreamSummary", __T("6"), "Audio", Literal), __T("6 zvukov\u00FDch stream\u016F: ")+Literal);
    Load(__T(""));
    Equal("switch to English", Message("FileCount", __T("6")), __T("6 files"));
    Load(Cs);
    Equal("switch back to Czech", Message("FileCount", __T("6")), __T("6 soubor\u016F"));
    Load(__T("  Language_ISO639;cs\n channel.other;{count} kan\u00E1l\u016F\n channel.many;{count} kan\u00E1lu"));
    Equal("explicit Czech decimal pattern", Unit(__T("1.50")), __T("1.50 kan\u00E1lu"));
    Load(__T("  Language_ISO639;ru\n  Config_Text_ThousandsSeparator;\n channel1; one\n channel2; few\n channel3; many"));
    Equal("count beyond 32 bit", Unit(__T("4294967321")), __T("4294967321 one"));
    Equal("count beyond 64 bit", Unit(__T("18446744073709551621")), __T("18446744073709551621 one"));
    Equal("signed count", Unit(__T("-21")), __T("-21 one"));
    Equal("explicit positive count", Unit(__T("+21")), __T("+21 one"));
    Load(__T("  Language_ISO639;CS-CZ\n channel1; one\n channel2; few\n channel3; other"));
    Equal("uppercase locale identifier", Unit(__T("22")), __T("22 other"));
    Load(__T("  Language_ISO639;pt\nFileCount.one;{count} one\nFileCount.other;{count} other"));
    Equal("legacy pt means Portugal", Message("FileCount", __T("0")), __T("0 other"));
    Load(__T("  Language_ISO639;pt-BR\nFileCount.one;{count} one\nFileCount.other;{count} other"));
    Equal("Brazilian Portuguese zero", Message("FileCount", __T("0")), __T("0 one"));
    Load(__T("  Language_ISO639;gr\nFileCount.one;{count} one\nFileCount.other;{count} other"));
    Equal("legacy Greek identifier", Message("FileCount", __T("1")), __T("1 one"));
}

void Le(std::vector<int8u>& Bytes, size_t Offset, unsigned Value, size_t Width)
{
    for (size_t I=0; I<Width; ++I)
        Bytes[Offset+I]=(int8u)(Value>>(I*8));
}

std::vector<int8u> Wave()
{
    const unsigned DataSize=22*2*100;
    std::vector<int8u> Bytes(44+DataSize, 0);
    const char* Riff="RIFF"; const char* Wave="WAVEfmt "; const char* Data="data";
    for (size_t I=0; I<4; ++I) { Bytes[I]=Riff[I]; Bytes[36+I]=Data[I]; }
    for (size_t I=0; I<8; ++I) Bytes[8+I]=Wave[I];
    Le(Bytes, 4, 36+DataSize, 4); Le(Bytes, 16, 16, 4); Le(Bytes, 20, 1, 2);
    Le(Bytes, 22, 22, 2); Le(Bytes, 24, 48000, 4); Le(Bytes, 28, 48000*22*2, 4);
    Le(Bytes, 32, 22*2, 2); Le(Bytes, 34, 16, 2); Le(Bytes, 40, DataSize, 4);
    return Bytes;
}

void MediaIntegration(const Ztring& Cs)
{
    const std::vector<int8u> Bytes=Wave();
    MediaInfo Info;
    Load(Cs);
    Info.Open_Buffer_Init(Bytes.size());
    Info.Open_Buffer_Continue(&Bytes[0], Bytes.size());
    Info.Open_Buffer_Finalize();
    Check("WAV has one audio stream (not 22 streams)", Info.Count_Get(Stream_Audio)==1);
    Equal("WAV raw channel count", Info.Get(Stream_Audio, 0, __T("Channel(s)")), __T("22"));
    Equal("WAV production formatted channels", Info.Get(Stream_Audio, 0, __T("Channel(s)/String")), __T("22 kan\u00E1l\u016F"));
    Info.Option(__T("Inform"), __T("JSON"));
    const Ztring JsonCs=Info.Inform();
    Check("JSON raw numeric channel field", JsonCs.find(__T("\"Channels\":\"22\""))!=Ztring::npos);
    Info.Option(__T("Inform"), __T("XML"));
    const Ztring XmlCs=Info.Inform();
    Check("XML raw numeric channel field", XmlCs.find(__T("<Channels>22</Channels>"))!=Ztring::npos);
    Load(__T(""));
    Info.Option(__T("Inform"), __T("JSON"));
    Equal("JSON is unchanged by language switch", Info.Inform(), JsonCs);
    Info.Option(__T("Inform"), __T("XML"));
    Equal("XML is unchanged by language switch", Info.Inform(), XmlCs);
    Info.Close();
}
}

int main(int Argc, char** Argv)
{
    if (Argc!=2)
    {
        std::cerr << "Usage: count_localization <MediaInfo CSV catalog directory>\n";
        return 2;
    }
    MediaInfo::Option_Static(__T("Info_Version"));
    Equal("public formatter capability", MediaInfo::Option_Static(__T("Language_Format")), __T("1"));
    const Ztring Cs=Catalog(Argv[1], "cs");
    Czech(Cs);
    Selector(Argv[1]);
    RuleExpressions();
    CatalogRules();
    ShippedCatalogs(Argv[1]);
    OtherLanguages(Argv[1]);
    FallbackAndArguments(Cs);
    MediaIntegration(Cs);
    std::cout << (Failures ? "FAIL: " : "PASS: ") << Checks << " assertions, " << Failures << " failures\n";
    return Failures ? 1 : 0;
}
