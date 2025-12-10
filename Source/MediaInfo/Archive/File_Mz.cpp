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
#if defined(MEDIAINFO_MZ_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Archive/File_Mz.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
struct mz_machine_data 
{
    int16u ID;
    const char* Name;
};
mz_machine_data Mz_Machine_Data[] =
{
    { 0x0000, "" },
    { 0x014C, "Intel i386" },
    { 0x014D, "Intel i860" },
    { 0x0162, "MIPS R3000" },
    { 0x0166, "MIPS R4000" },
    { 0x0168, "MIPS R10000" },
    { 0x0169, "MIPS WCE v2" },
    { 0x0183, "DEC Alpha" },
    { 0x0184, "DEC Alpha AXP" },
    { 0x01A2, "Hitachi SH3" },
    { 0x01A3, "Hitachi SH3 DSP" },
    { 0x01A6, "Hitachi SH4" },
    { 0x01A8, "Hitachi SH5" },
    { 0x01C0, "ARM" },
    { 0x01C2, "ARM Thumb" },
    { 0x01C4, "ARM Thumb-2" },
    { 0x01D3, "Matsushita AM33" },
    { 0x01F0, "Power PC" },
    { 0x01F1, "Power PC with FP" },
    { 0x0200, "Intel IA64" },
    { 0x0266, "MIPS16" },
    { 0x0284, "DEC Alpha 64" },
    { 0x0366, "MIPS with FPU" },
    { 0x0466, "MIPS16 with FPU" },
    { 0x0EBC, "EFI" },
    { 0x5032, "RISC-V 32-bit address space" },
    { 0x5064, "RISC-V 64-bit address space" },
    { 0x5128, "RISC-V 128-bit address space" },
    { 0x6232, "LoongArch 32-bit" },
    { 0x6264, "LoongArch 64-bit" },
    { 0x8664, "AMD x86-64" },
    { 0x9041, "Mitsubishi M32R" },
    { 0xAA64, "ARM64" },
    { 0xA641, "ARM64EC" },
    { 0xA64E, "ARM64X" },
};
static string Mz_Machine(int16u Machine)
{
    for (const auto& Item : Mz_Machine_Data)
        if (Item.ID == Machine)
            return Item.Name;
    return "0x" + Ztring().From_CC2(Machine).To_UTF8();
}

const char* Mz_Windows_Subsystem[]{
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows CUI",
    "",
    "OS2 CUI",
    "",
    "POSIX CUI",
    "Native Windows",
    "Windows CE GUI",
    "EFI Application",
    "EFI Boot Service Driver",
    "EFI Runtime Driver",
    "EFI ROM",
    "XBOX",
    "",
    "Windows Boot Application"
};

static string Mz_DLL_Characteristics(int16u DllCharacteristics)
{
    const char* DllCharacteristics_List[]{
        "ProcessInit",
        "ProcessTerm",
        "ThreadInit",
        "ThreadTerm",
        "",
        "High Entropy VA",
        "Dynamic Base",
        "Force Integrity",
        "NX Compat",
        "No Isolation",
        "No SEH",
        "No Bind",
        "AppContainer",
        "WDM Drive",
        "Control Flow Guard",
        "Terminal Server Aware"
    };
    int DllCharacteristics_List_Sz = sizeof(DllCharacteristics_List) / sizeof(DllCharacteristics_List[0]);

    string to_return;
    for (int i = 0; i < DllCharacteristics_List_Sz; ++i) {
        if (DllCharacteristics & (1U << i)) {
            if (!to_return.empty())
                to_return += ", ";
            to_return += DllCharacteristics_List[i];
        }
    }
    return to_return;
}

const char* Mz_Directories[]{
    "Export Table",
    "Import Table",
    "Resource Table",
    "Exception Table",
    "Certificate Table",
    "Base Relocation Table",
    "Debug",
    "Architecture",
    "Global Ptr",
    "TLS Table",
    "Load Config Table",
    "Bound Import",
    "Import Address Table",
    "Delay Import Descriptor",
    "CLR Runtime Header",
    "Reserved"
};

//***************************************************************************
// Static stuff
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mz::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<2)
        return false; //Must wait for more data

    if (Buffer[0]!=0x4D //"MZ"
     || Buffer[1]!=0x5A)
    {
        Reject("MZ");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mz::Read_Buffer_Continue()
{
    if (rsrc_offset) {
        Parse_Resources();       

        Finish("MZ");
        return;
    }

    //Parsing
    int32u lfanew;
    Element_Begin1("MZ");
    Skip_C2(                                                    "magic");
    Skip_L2(                                                    "cblp");
    Skip_L2(                                                    "cp");
    Skip_L2(                                                    "crlc");
    Skip_L2(                                                    "cparhdr");
    Skip_L2(                                                    "minalloc");
    Skip_L2(                                                    "maxalloc");
    Skip_L2(                                                    "ss");
    Skip_L2(                                                    "sp");
    Skip_L2(                                                    "csum");
    Skip_L2(                                                    "ip");
    Skip_L2(                                                    "cs");
    Skip_L2(                                                    "lsarlc");
    Skip_L2(                                                    "ovno");
    Skip_L2(                                                    "res");
    Skip_L2(                                                    "res");
    Skip_L2(                                                    "res");
    Skip_L2(                                                    "res");
    Skip_L2(                                                    "oemid");
    Skip_L2(                                                    "oeminfo");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Skip_L2(                                                    "res2");
    Get_L4 (lfanew,                                             "lfanew");

    //Computing
    if (lfanew>Element_Offset)
    {
        Skip_XX(lfanew-Element_Offset,                          "MZ data");
        Element_End0();
    }
    if (Element_Offset>lfanew)
    {
        Element_End0();
        Element_Offset=lfanew; //Multi usage off the first bytes
    }

    //Parsing
    int32u Signature, TimeDateStamp=0;
    int16u Machine{}, NumberOfSections{}, SizeOfOptionalHeader{}, Characteristics{}, Subsystem{}, DllCharacteristics{}, MajorSubsystemVersion{}, MinorSubsystemVersion{};
    int8u MajorLinkerVersion{}, MinorLinkerVersion{};
    Peek_B4(Signature);
    if (Signature==0x50450000) //"PE"
    {
        Element_Begin1("PE");
        Skip_C4(                                                "Signature");
        Element_Begin1("COFF File Header");
        Get_L2 (Machine,                                        "Machine"); Param_Info1(Mz_Machine(Machine));
        Get_L2 (NumberOfSections,                               "NumberOfSections");
        Get_L4 (TimeDateStamp,                                  "TimeDateStamp"); Param_Info1(Ztring().Date_From_Seconds_1970(TimeDateStamp));
        Skip_L4(                                                "PointerToSymbolTable");
        Skip_L4(                                                "NumberOfSymbols");
        Get_L2 (SizeOfOptionalHeader,                           "SizeOfOptionalHeader");
        Get_L2 (Characteristics,                                "Characteristics");
        Element_End0();
        if (SizeOfOptionalHeader >= 20) {
            Element_Begin1("Optional Header");
            int16u Magic;
            Get_L2(Magic,                                       "Magic"); Param_Info1(Magic == 0x10B ? "PE32" : Magic == 0x20B ? "PE32+" : Magic == 0x107 ? "ROM" : "");
            Get_L1 (MajorLinkerVersion,                         "MajorLinkerVersion");
            Get_L1 (MinorLinkerVersion,                         "MinorLinkerVersion");
            Skip_L4(                                            "SizeOfCode");
            Skip_L4(                                            "SizeOfInitializedData");
            Skip_L4(                                            "SizeOfUninitializedData");
            Skip_L4(                                            "AddressOfEntryPoint");
            Skip_L4(                                            "BaseOfCode");
            if (Magic == 0x10B)
                Skip_L4(                                        "BaseOfData");
            if (SizeOfOptionalHeader > 24) {
                int32u NumberOfRvaAndSizes;
                if (Magic == 0x10B)
                    Skip_L4(                                    "ImageBase");
                if (Magic == 0x20B)
                    Skip_L8(                                    "ImageBase");
                Skip_L4(                                        "SectionAlignment");
                Skip_L4(                                        "FileAlignment");
                Skip_L2(                                        "MajorOperatingSystemVersion");
                Skip_L2(                                        "MinorOperatingSystemVersion");
                Skip_L2(                                        "MajorImageVersion");
                Skip_L2(                                        "MinorImageVersion");
                Get_L2 (MajorSubsystemVersion,                  "MajorSubsystemVersion");
                Get_L2 (MinorSubsystemVersion,                  "MinorSubsystemVersion");
                Skip_L4(                                        "Win32VersionValue");
                Skip_L4(                                        "SizeOfImage");
                Skip_L4(                                        "SizeOfHeaders");
                Skip_L4(                                        "CheckSum");
                Get_L2 (Subsystem,                              "Subsystem");
                if (Subsystem < sizeof(Mz_Windows_Subsystem) / sizeof(Mz_Windows_Subsystem[0])) Param_Info1(Mz_Windows_Subsystem[Subsystem]);
                Get_L2 (DllCharacteristics,                     "DllCharacteristics"); Param_Info1(Mz_DLL_Characteristics(DllCharacteristics));
                if (Magic == 0x10B) {
                    Skip_L4(                                    "SizeOfStackReserve");
                    Skip_L4(                                    "SizeOfStackCommit");
                    Skip_L4(                                    "SizeOfHeapReserve");
                    Skip_L4(                                    "SizeOfHeapCommit");
                }
                if (Magic == 0x20B) {
                    Skip_L8(                                    "SizeOfStackReserve");
                    Skip_L8(                                    "SizeOfStackCommit");
                    Skip_L8(                                    "SizeOfHeapReserve");
                    Skip_L8(                                    "SizeOfHeapCommit");
                }
                Skip_L4(                                        "LoaderFlags");
                Get_L4 (NumberOfRvaAndSizes,                    "NumberOfRvaAndSizes");
                for (int32u i = 0; i < NumberOfRvaAndSizes; ++i) {
                    Element_Begin1("Data Directory");
                    if (i < sizeof(Mz_Directories) / sizeof(Mz_Directories[0]))
                        Element_Info1(Mz_Directories[i]);
                    Skip_L4(                                    "VirtualAddress");
                    Skip_L4(                                    "Size");
                    Element_End0();
                }
            }
            Element_End0();
        }
        if (SizeOfOptionalHeader > 24) {
            for (int8u i = 0; i < NumberOfSections; ++i) {
                Element_Begin1("Section Header");
                int64u Name;
                int32u VirtualSize, VirtualAddress, PointerToRawData;
                Get_C8 (Name,                                   "Name"); Element_Info1(Ztring::ToZtring_From_CC4(Name >> 32) + Ztring::ToZtring_From_CC4(Name & 0xFFFFFFFF));
                Get_L4 (VirtualSize,                            "VirtualSize");
                Get_L4 (VirtualAddress,                         "VirtualAddress");
                Skip_L4(                                        "SizeOfRawData");
                Get_L4 (PointerToRawData,                       "PointerToRawData");
                Skip_L4(                                        "PointerToRelocations");
                Skip_L4(                                        "PointerToLinenumbers");
                Skip_L2(                                        "NumberOfRelocations");
                Skip_L2(                                        "NumberOfLinenumbers");
                Skip_L4(                                        "Characteristics");
                if (Name == 0x2E72737263000000) { // .rsrc
                    rsrc_size = VirtualSize;
                    rsrc_virtual_addr = VirtualAddress;
                    rsrc_offset = PointerToRawData;
                }
                Element_End0();
            }
        }
        Element_End0();
    }

    FILLING_BEGIN();
        Accept("MZ");

        Fill(Stream_General, 0, General_Format, "MZ");
        if (Characteristics&0x2000)
            Fill(Stream_General, 0, General_Format_Profile, "DLL");
        else if (Characteristics&0x0002)
            Fill(Stream_General, 0, General_Format_Profile, "Executable");
        Fill(Stream_General, 0, General_Format_Profile, Mz_Machine(Machine));
        if (TimeDateStamp)
        {
            Ztring Time=Ztring().Date_From_Seconds_1970(TimeDateStamp);
            if (!Time.empty())
            {
                Time.FindAndReplace(__T("UTC "), __T(""));
                Time+=__T(" UTC");
            }
            Fill(Stream_General, 0, General_Encoded_Date, Time);
        }
        if (MajorLinkerVersion)
            Fill(Stream_General, 0, "Linker_Version", std::to_string(MajorLinkerVersion) + "." + std::to_string(MinorLinkerVersion));
        if (Subsystem < sizeof(Mz_Windows_Subsystem) / sizeof(Mz_Windows_Subsystem[0]))
            Fill(Stream_General, 0, "Subsystem_Name", Mz_Windows_Subsystem[Subsystem]);
        if (MajorSubsystemVersion)
            Fill(Stream_General, 0, "Subsystem_Version", std::to_string(MajorSubsystemVersion) + "." + std::to_string(MinorSubsystemVersion));
        Fill(Stream_General, 0, "Format_Settings", Mz_DLL_Characteristics(DllCharacteristics));

        if (rsrc_offset) {
            GoTo(rsrc_offset);
        }
        else {
            //No more need data
            Finish("MZ");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mz::Parse_Resources() {
    int16u num_name, num_ID;
    vector<int32u> NameOffsets;
    vector<int32u> OffsetToDirectories;
    vector<int32u> OffsetToData;
    auto resource_name = Named_Resource.find((int32u)Element_Offset);
    auto resource_id = Resource.find((int32u)Element_Offset);
    Element_Begin1("Resource Directory Table");
    Skip_L4(                                                    "Characteristics");
    Skip_L4(                                                    "TimeDateStamp");
    Skip_L2(                                                    "MajorVersion");
    Skip_L2(                                                    "MinorVersion");
    Get_L2 (num_name,                                           "NumberOfNamedEntries");
    Get_L2 (num_ID,                                             "NumberOfIdEntries");
    Element_End0();
    for (int16u i = 0; i < num_name; ++i) {
        Element_Begin1("Resource Directory Entry");
        int32u NameOffset, OffsetToDirectory;
        Ztring name;
        Get_L4(NameOffset,                                      "NameOffset");
        Get_L4(OffsetToDirectory,                               "OffsetToDirectory");
        bool DataIsDirectory = OffsetToDirectory & (1U << 31);
        if (DataIsDirectory)
            OffsetToDirectories.push_back(OffsetToDirectory & 0x7FFFFFFF);
        else
            OffsetToData.push_back(OffsetToDirectory);
        bool NameIsString = NameOffset & (1U << 31);
        Element_End0();
        if (NameIsString) {
            auto Element_Offset_Save = Element_Offset;
            Element_Offset = NameOffset & 0x7FFFFFFF;
            Element_Begin1("Resource Directory String");
            int16u Length;
            Get_L2(Length,                                      "Length");
            Get_UTF16L(Length * 2LL, name,                      "Unicode String");
            Element_End0();
            Element_Offset = Element_Offset_Save;
        }
        Named_Resource.insert({ OffsetToDirectory & 0x7FFFFFFF, name });
    }
    for (int16u i = 0; i < num_ID; ++i) {
        Element_Begin1("Resource Directory Entry");
        int32u Id, OffsetToDirectory;
        Get_L4(Id,                                              "Id");
        Get_L4(OffsetToDirectory,                               "OffsetToDirectory");
        bool DataIsDirectory = OffsetToDirectory & (1U << 31);
        if (DataIsDirectory)
            OffsetToDirectories.push_back(OffsetToDirectory & 0x7FFFFFFF);
        else
            OffsetToData.push_back(OffsetToDirectory);
        if (resource_name != Named_Resource.end()) {
            Named_Resource.insert({ OffsetToDirectory & 0x7FFFFFFF, resource_name->second });
        }
        if (resource_id != Resource.end()) {
            Resource.insert({ OffsetToDirectory & 0x7FFFFFFF, resource_id->second });
        }
        else {
            Resource.insert({ OffsetToDirectory & 0x7FFFFFFF, Id });
        }
        Element_End0();
        
    }
    for (const int& directory_offset : OffsetToDirectories) {
        Element_Offset = directory_offset;
        Parse_Resources();
    }
    for (const int& data_offset : OffsetToData) {
        Element_Offset = data_offset;
        auto resource_name = Named_Resource.find((int32u)Element_Offset);
        auto resource_id = Resource.find((int32u)Element_Offset);
        Element_Begin1("Resource Data Entry");
        int32u OffsetToData;
        Get_L4 (OffsetToData,                                   "OffsetToData");
        Skip_L4(                                                "Size");
        Skip_L4(                                                "CodePage");
        Skip_L4(                                                "Reserved");
        Element_End0();
        if (resource_name != Named_Resource.end() && resource_name->second == __T("BOOTMGRSECURITYVERSIONNUMBER")) {
            if (OffsetToData > rsrc_virtual_addr && OffsetToData < rsrc_virtual_addr + rsrc_size) {
                Element_Offset = (OffsetToData - rsrc_virtual_addr + rsrc_offset) - Buffer_Offset - File_Offset;
                Element_Begin1("BOOTMGRSECURITYVERSIONNUMBER");
                int16u minorver, majorver;
                Get_L2(minorver,                                "MinorVersion");
                Get_L2(majorver,                                "MajorVersion");

                FILLING_BEGIN();
                Fill(Stream_General, 0, "BootMgrSecurity_Version", std::to_string(majorver) + "." + std::to_string(minorver));
                FILLING_END();
                Element_End0();
            }
        }
        if (resource_id != Resource.end() && resource_id->second == 16) { // VS_VERSIONINFO
            if (OffsetToData > rsrc_virtual_addr && OffsetToData < rsrc_virtual_addr + rsrc_size) {
                Element_Offset = (OffsetToData - rsrc_virtual_addr + rsrc_offset) - Buffer_Offset - File_Offset;
                Element_Begin1("VS_VERSIONINFO");
                Ztring szKey;
                Skip_L2(                                        "wLength");
                Skip_L2(                                        "wValueLength");
                Skip_L2(                                        "wType");
                Get_UTF16L(32, szKey,                           "szKey");
                if (szKey == __T("VS_VERSION_INFO")) {
                    Skip_XX((4 - (File_Offset + Buffer_Offset + Element_Offset) & 3) & 3, "Padding1");
                    Element_Begin1("VS_FIXEDFILEINFO");
                    Skip_L4(                                    "dwSignature");
                    Skip_L4(                                    "dwStrucVersion");
                    Skip_L4(                                    "dwFileVersionMS");
                    Skip_L4(                                    "dwFileVersionLS");
                    Skip_L4(                                    "dwProductVersionMS");
                    Skip_L4(                                    "dwProductVersionLS");
                    Skip_L4(                                    "dwFileFlagsMask");
                    Skip_L4(                                    "dwFileFlags");
                    Skip_L4(                                    "dwFileOS");
                    Skip_L4(                                    "dwFileType");
                    Skip_L4(                                    "dwFileSubtype");
                    Skip_L4(                                    "dwFileDateMS");
                    Skip_L4(                                    "dwFileDateLS");
                    Element_End0();
                    Skip_XX((4 - (File_Offset + Buffer_Offset + Element_Offset) & 3) & 3, "Padding2");
                    Element_Begin0();
                    Parse_StringFileInfo();
                    Element_End0();
                }
                Element_End0();
            }
        }
    }
}

//---------------------------------------------------------------------------
bool File_Mz::Parse_StringFileInfo(int8u level) {
    auto SizeUpTo0_16 = [&]() -> int64u {
            auto Buffer_Begin = Buffer + Buffer_Offset + (size_t)Element_Offset;
            auto Buffer_Current = Buffer_Begin;
            auto Remaining = (size_t)(Element_Size - Element_Offset);
            auto Buffer_End = Buffer_Begin + (128 > Remaining ? Remaining : 128);
            while (Buffer_Current < Buffer_End && (Buffer_Current[0] || Buffer_Current[1]))
                Buffer_Current += 2;
            return Buffer_Current - Buffer_Begin;
        };
    int16u wLength, wValueLength;
    Ztring szKey, Value;
    Get_L2 (wLength,                                            "wLength");
    auto Offset_End = Element_Offset + wLength - 2;
    Get_L2 (wValueLength,                                       "wValueLength");
    Skip_L2(                                                    "wType");
    Get_UTF16L(SizeUpTo0_16(), szKey,                           "szKey");
    Element_Offset += 2;
    Skip_XX((4 - (File_Offset + Buffer_Offset + Element_Offset) & 3) & 3, "Padding");
    if (szKey == __T("StringFileInfo")) {
        Element_Name("StringFileInfo");
        Element_Begin1("StringTable");
        Parse_StringFileInfo(1);
        Element_End0();
    } else if (szKey.size() == 8 && level == 1) {
        while (Element_Offset + 6 < Offset_End) {
            Element_Begin1("String");
            Parse_StringFileInfo(2);
            Element_End0();
        }
    } else if (level == 2) {
        Get_UTF16L(wValueLength * 2LL, Value,                   "Value");
    } else {
        return 0;
    }
    Skip_XX((4 - (File_Offset + Buffer_Offset + Element_Offset) & 3) & 3, "Padding");

    FILLING_BEGIN();
    if (level == 2) {
        string Key;
        string szKey_utf8{ szKey.To_UTF8() };
        if (szKey_utf8 == "CompanyName") Key = "Software_CompanyName";
        else if (szKey_utf8 == "FileDescription") Key = "Software_Description";
        else if (szKey_utf8 == "FileVersion") Key = "Software_Version";
        else if (szKey_utf8 == "LegalCopyright") Key = "Copyright";
        else if (szKey_utf8 == "ProductName") Key = "Software_Name";
        else if (szKey_utf8 == "ProductVersion") Key = "Software_Version";
        else Key = szKey_utf8;
        Fill(Stream_General, 0, Key.c_str(), Value);
    }
    FILLING_END();

    return true;
}

} //NameSpace

#endif //MEDIAINFO_MZ_YES
