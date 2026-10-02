/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_TAR_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Archive/File_Tar.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Helpers
//***************************************************************************

const char* Names[] =
{
    "Header",
    "Payload",
    "Data",
    "End of archive",
};

static std::string TrimNUL(std::string s)
{
    while (!s.empty() && s.back() == 0x00) s.pop_back();
    return s;
}

static int64u ParseOct(int128u z, size_t ValidBytes)
{
    int64u v = 0;
    for (size_t i = 0; i < ValidBytes; i++)
    {
        int64u s = (z >> ((ValidBytes - 1 - i) * 8)).lo & 0xFF;
        if (s == ' ' || s == 0)
            break;
        if (s < '0' || s > '7')
            return 0;
        v = (v << 3) | (int64u)(s - '0');
    }
    return v;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Tar::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "TAR");
}

//---------------------------------------------------------------------------
void File_Tar::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Tar::Header_Parse()
{
    int64u ContentSize = 0;

    switch (Type)
    {
    case Type_Header:
    {
        const auto BufferO = Buffer + Buffer_Offset;

        // Is the current block all zero?
        bool CurrentAllZero = true;
        size_t MaxSize = (size_t)std::min(Element_Size, 1024ull);
        for (size_t i = 0; i < MaxSize; i++)
        {
            if (BufferO[i] != 0x00) { CurrentAllZero = false; break; }
        }

        if (CurrentAllZero)
        {
            if (MaxSize < 1024 && File_Offset + Buffer_Size < File_Size)
            {
                Element_WaitForMoreData();
                return;
            }
            Type = Type_EndOfFile;
            ContentSize = MaxSize;
            break;
        }

        ContentSize = 512;
        break;
    }

    case Type_Payload:
        ContentSize = (Size + 511) / 512 * 512;
        break;

    case Type_Data:
        ContentSize = 512;
        break;
    case Type_EndOfFile: break; // Never happens
    }

    Header_Fill_Code(0, Names[Type]);
    Header_Fill_Size(ContentSize);
}

//---------------------------------------------------------------------------
void File_Tar::Data_Parse()
{
    switch (Type)
    {
    case Type_Header: Header(); break;
    case Type_Payload: Payload(); break;
    case Type_Data: Data(); break;
    case Type_EndOfFile: EndOfFile(); break;
    }
}

//---------------------------------------------------------------------------
void File_Tar::Header()
{
    const auto BufferO = Buffer + Buffer_Offset;

    // Magic / version (strict, but tolerant of real-world version padding) ---
    bool IsUstarMagic =
        (BufferO[257] == 'u' && BufferO[258] == 's' &&
            BufferO[259] == 't' && BufferO[260] == 'a' &&
            BufferO[261] == 'r');

    bool IsNullMagic =
        (BufferO[257] == 0x00 && BufferO[258] == 0x00 &&
            BufferO[259] == 0x00 && BufferO[260] == 0x00 &&
            BufferO[261] == 0x00 && BufferO[262] == 0x00 &&
            BufferO[263] == 0x00 && BufferO[264] == 0x00);

    if (IsUstarMagic)
    {
        // Offset 262: NUL (POSIX) or space (old GNU)
        if (!(BufferO[262] == 0x00 || BufferO[262] == ' '))
        {
            Reject();
            return;
        }

        // Offsets 263..264: any combination of '0', space, or NUL
        auto IsVersionByte = [](int8u c) -> bool {
            return c == '0' || c == ' ' || c == 0x00;
            };
        if (!(IsVersionByte(BufferO[263]) && IsVersionByte(BufferO[264])))
        {
            Reject();
            return;
        }
    }
    else if (!IsNullMagic)
    {
        Reject();
        return;
    }

    // Typeflag 
    TypeFlag = (int8u)BufferO[0x9C];
    if (IsUstarMagic)
    {
        switch (TypeFlag)
        {
            case 0x00: case '0': case '1': case '2':
            case '3':  case '4': case '5': case '6': case '7':
                Type = Type_Data;
                break;
            case 'x':  case 'g': case 'L': case 'K':
                Type = Type_Payload;
                break;
            default:
                Reject();
                return;
        }
    }
    else // IsV7
    {
        switch (TypeFlag)
        {
            case 0x00: case '0': case '1': case '2':
            case '3':  case '4': case '5': case '6': case '7':
                Type = Type_Data;
                break;
            default:
                Reject();
                return;
        }
    }

    // Name must not start with NUL
    if (BufferO[0] == 0x00)
    {
        if (!IsUstarMagic || (TypeFlag != 'x' && TypeFlag != 'g'))
        {
            Reject();
            return;
        }
    }

    // Numeric field sanity
    {
        bool NumNOk = false;
        for (size_t p = 100; p < 156; p++)
        {
            int8u c = (int8u)BufferO[p];
            if (!((c >= '0' && c <= '7') || c == 0x00 || c == ' '))
            { NumNOk = true; break; }
        }
        if (NumNOk) { Reject(); return; }
    }

    // Checksum
    {
        int32u Checksum = 0;
        size_t b = 148, e = 156;
        while (b < e && (BufferO[b] == ' ' || BufferO[b] == 0x00)) b++;
        while (e > b && (BufferO[e-1] == ' ' || BufferO[e-1] == 0x00)) e--;
        if (b == e) { Reject(); return; }
        bool CkOk = true;
        for (size_t i = b; i < e; i++)
        {
            int8u c = (int8u)BufferO[i];
            if (c < '0' || c > '7') { CkOk = false; break; }
            Checksum = (Checksum << 3) | (int32u)(c - '0');
        }
        if (!CkOk) { Reject(); return; }

        int32u CU = 0, CS = 0;
        for (size_t p = 0; p < 512; p++)
        {
            if (p == 148) { CU += 32*8; CS += 32*8; p += 7; continue; }
            CU += (int8u)BufferO[p];
            CS += (int8s)BufferO[p];
        }
        if (CU != Checksum && CS != Checksum) { Reject(); return; }
    }

    // Read all fields
    Ztring Name_S, LinkName_S, UserName_S, GroupName_S, Prefix_S;
    int128u Size128 = 0;
    Get_UTF8 (100, Name_S,                                      "File name");
    Skip_C8  (                                                  "File mode");
    Skip_C8  (                                                  "Owner's numeric user ID");
    Skip_C8  (                                                  "Group's numeric user ID");
    Get_C12  (     Size128,                                     "File size in bytes"); Size = ParseOct(Size128, 12); Param_Info1(Size);
    Skip_UTF8( 12,                                              "Last modification time");
    Skip_C8  (                                                  "Checksum");
    Skip_C1  (                                                  "Link indicator");
    Get_UTF8 (100, LinkName_S,                                  "Name of linked file");
    Skip_String(6,                                              "Magic");
    Skip_String(2,                                              "Version");
    Get_UTF8 ( 32, UserName_S,                                  "Owner user name");
    Get_UTF8 ( 32, GroupName_S,                                 "Owner group name");
    Skip_C8  (                                                  "Device major number");
    Skip_C8  (                                                  "Device minor number");
    Get_UTF8 (155, Prefix_S,                                    "Prefix");
    Skip_XX  ( 12,                                              "Padding");

    FILLING_BEGIN()
        if (!Status[IsAccepted])
            Accept();

        //Build the full path: prefix + '/' + name
        std::string Name      = TrimNUL(Name_S.To_UTF8());
        std::string Prefix    = TrimNUL(Prefix_S.To_UTF8());
        std::string FullName;
        if (!Prefix.empty()) { FullName = Prefix; FullName += '/'; }
        FullName += Name;

        std::string LinkName  = TrimNUL(LinkName_S.To_UTF8());
        std::string UserName  = TrimNUL(UserName_S.To_UTF8());
        std::string GroupName = TrimNUL(GroupName_S.To_UTF8());

        //Apply PAX overrides (per-file takes precedence over global)
        auto GetBoth = [&](const char* k) -> const std::string* {
            auto it = Pending_PAXOverrides.find(k);
            if (it != Pending_PAXOverrides.end()) return &it->second;
            it = Global_PAXOverrides.find(k);
            if (it != Global_PAXOverrides.end())  return &it->second;
            return nullptr;
        };
        if (const std::string* v = GetBoth("path"))     FullName  = *v;
        if (const std::string* v = GetBoth("linkpath")) LinkName  = *v;
        if (const std::string* v = GetBoth("uname"))    UserName  = *v;
        if (const std::string* v = GetBoth("gname"))    GroupName = *v;
        if (const std::string* v = GetBoth("size"))
        {
            char* End = nullptr;
            auto Value = strtoul(v->c_str(), &End, 10);
            if (End != v->c_str() && *End == '\0')
                Size = Value;
        }

        // Apply GNU long-name / long-link overrides
        if (!Pending_GNULongName.empty()) FullName = Pending_GNULongName;
        if (!Pending_GNULongLink.empty()) LinkName = Pending_GNULongLink;

        // Consume per-file pending state; globals stay
        Pending_PAXOverrides.clear();
        Pending_GNULongName.clear();
        Pending_GNULongLink.clear();

        Element_Info1C(Type == Type_Data, FullName);
        Element_Info1C(!LinkName.empty(), LinkName);
    FILLING_END();

    //For zero-length files there is no data block
    if (Size == 0)
        Type = Type_Header;
}

//---------------------------------------------------------------------------
void File_Tar::Payload()
{
    bool IsPAX   = (TypeFlag == 'x' || TypeFlag == 'g');
    bool IsGNU_L = (TypeFlag == 'L');
    bool IsGNU_K = (TypeFlag == 'K');

    // GNU long name / long link: payload is a single NUL-terminated
    // string, then padding.
    if (IsGNU_L || IsGNU_K)
    {
        Ztring Value;
        Get_UTF8(Size, Value,                                   "Data");
        Skip_XX (Element_Size - Size,                           "Padding");

        std::string S = TrimNUL(Value.To_UTF8());
        if (IsGNU_L) Pending_GNULongName = S;
        if (IsGNU_K) Pending_GNULongLink = S;

        Type = Type_Header;
        return;
    }

    // PAX records: "<len> <key>=<value>\n" repeated
    const int8u* P   = (const int8u*)Buffer + Buffer_Offset;
    size_t       End = (size_t)Size;
    size_t       Pos = 0;

    while (Pos < End)
    {
        Element_Begin1("Tag");

        size_t RecStart = Pos;
        size_t Len      = 0;
        bool   LenOk    = false;
        while (Pos < End && P[Pos] >= '0' && P[Pos] <= '9')
        {
            Len = Len * 10 + (size_t)(P[Pos] - '0');
            Pos++;
            LenOk = true;
        }

        // A valid record needs at least "NN x=y\n"
        if (!LenOk || Pos >= End || P[Pos] != ' ' ||
            Len < Pos - RecStart + 4 ||              // digits + space + key + '=' + '\n'
            RecStart + Len > End)
        {
            // Malformed: skip the rest of the payload to stay aligned
            Skip_XX(End - Pos + (Pos - RecStart),               "(Malformed)");
            Element_End0();
            break;
        }

        bool HasEOL = (P[RecStart + Len - 1] == 0x0A);

        Skip_String(Pos - RecStart,                             "Size");
        Skip_B1  (                                              "Separator");

        size_t CS_ = Pos + 1;
        size_t CE  = RecStart + Len - (HasEOL ? 1 : 0);
        size_t Eq  = CS_;
        while (Eq < CE && P[Eq] != '=') Eq++;

        if (Eq < CE)
        {
            std::string Key, Value;
            Get_String(Eq - CS_,     Key,                       "Key");
            Skip_C1(                                            "Separator");
            Get_String(CE - Eq - 1,  Value,                     "Value");

            if (!Key.empty())
            {
                if (TypeFlag == 'g') Global_PAXOverrides[Key]  = Value;
                else                 Pending_PAXOverrides[Key] = Value;
            }
        }
        else
        {
            // No '=', skip the record content to stay aligned
            Skip_XX(CE - CS_,                                   "(Malformed)");
        }
        if (HasEOL)
            Skip_B1(                                            "Separator");

        Pos = RecStart + Len;
        Element_End0();
    }

    Skip_XX(Element_Size - Size,                                "Padding");

    Type = Type_Header;
}

//---------------------------------------------------------------------------
void File_Tar::Data()
{
    int64u CurrentSize;
    if (Size > 512)
    {
        CurrentSize = 512;
        Size -= 512;
    }
    else
    {
        CurrentSize = Size;
        Size = 0;
        Type = Type_Header;
    }

    Skip_XX(CurrentSize,                                        "File data");
    Skip_XX(512 - CurrentSize,                                  "Padding");

    Finish(); // We stop there for the moment
}

//---------------------------------------------------------------------------
void File_Tar::EndOfFile()
{
    Skip_XX(Element_Size,                                       "End of archive block");
    Finish();
}
//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_TAR_YES
