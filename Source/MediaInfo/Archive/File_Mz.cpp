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
};
string Mz_Machine(int16u Machine)
{
    for (const auto& Item : Mz_Machine_Data)
        if (Item.ID == Machine)
            return Item.Name;
    return "0x" + Ztring().From_CC2(Machine).To_UTF8();
}

static const char* Mz_Windows_Subsystem(int16u Subsystem)
{
    switch (Subsystem) {
    case 0: return "Unknown";
    case 1: return "Native";
    case 2: return "Windows GUI";
    case 3: return "Windows CUI";
    case 5: return "OS2 CUI";
    case 7: return "POSIX CUI";
    case 8: return "Native Windows";
    case 9: return "Windows CE GUI";
    case 10: return "EFI Application";
    case 11: return "EFI Boot Service Driver";
    case 12: return "EFI Runtime Driver";
    case 13: return "EFI ROM";
    case 14: return "XBOX";
    case 16: return "Windows Boot Application";
    default: return "";
    }
}

struct mz_dllcharacteristics_data
{
    int16u Value;
    const char* Characteristic;
};
mz_dllcharacteristics_data Mz_DLLCharacteristics_Data[] =
{
    { 0x0020, "High Entropy VA" },
    { 0x0040, "Dynamic Base" },
    { 0x0080, "Force Integrity" },
    { 0x0100, "NX Compat" },
    { 0x0200, "No Isolation" },
    { 0x0400, "No SEH" },
    { 0x0800, "No Bind" },
    { 0x1000, "AppContainer" },
    { 0x2000, "WDM Driver" },
    { 0x4000, "Guard CF" },
    { 0x8000, "Terminal Server Aware" },
};
static string Mz_DLL_Characteristics(int16u DllCharacteristics)
{
    string Characteristics;
    for (const auto& Item : Mz_DLLCharacteristics_Data)
        if (Item.Value & DllCharacteristics) {
            if (!Characteristics.empty())
                Characteristics += ", ";
            Characteristics += Item.Characteristic;
        }
    return Characteristics;
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
                Get_L2 (Subsystem,                              "Subsystem"); Param_Info1(Mz_Windows_Subsystem(Subsystem));
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
                Get_C8 (Name,                                   "Name"); Element_Info1(Ztring::ToZtring_From_CC4(Name >> 32) + Ztring::ToZtring_From_CC4(Name));
                Skip_L4(                                        "VirtualSize");
                Skip_L4(                                        "VirtualAddress");
                Skip_L4(                                        "SizeOfRawData");
                Skip_L4(                                        "PointerToRawData");
                Skip_L4(                                        "PointerToRelocations");
                Skip_L4(                                        "PointerToLinenumbers");
                Skip_L2(                                        "NumberOfRelocations");
                Skip_L2(                                        "NumberOfLinenumbers");
                Skip_L4(                                        "Characteristics");
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
        Fill(Stream_General, 0, "Windows_Subsystem", Mz_Windows_Subsystem(Subsystem));
        if (MajorSubsystemVersion)
            Fill(Stream_General, 0, "Subsystem_Version", std::to_string(MajorSubsystemVersion) + "." + std::to_string(MinorSubsystemVersion));
        Fill(Stream_General, 0, "Dll_Characteristics", Mz_DLL_Characteristics(DllCharacteristics));

        //No more need data
        Finish("MZ");
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_MZ_YES
