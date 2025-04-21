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
#if defined(MEDIAINFO_MACHO_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Archive/File_MachO.h"
#include <cmath>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
// https://github.com/opensource-apple/cctools/blob/master/include/mach/machine.h
const char* MachO_cputype(int32u cputype)
{
    switch (cputype)
    {
    case 0x00000001: return "VAX";
    case 0x00000002: return "ROMP";
    case 0x00000004: return "NS32032";
    case 0x00000005: return "NS32332";
    case 0x00000006: return "MC680x0";
    case 0x00000007: return "Intel i386";
    case 0x01000007: return "AMD x86-64";
    case 0x00000008: return "MIPS";
    case 0x00000009: return "NS32352";
    case 0x0000000B: return "HP-PA";
    case 0x0000000C: return "ARM";
    case 0x0100000C: return "ARM64";
    case 0x0000000D: return "MC88000";
    case 0x0000000E: return "SPARC";
    case 0x0000000F: return "Intel i860 (big-endian)";
    case 0x00000010: return "Intel i860 (little-endian)";
    case 0x00000011: return "RS/6000";
    case 0x00000012: return "PowerPC / MC98000";
    case 0x01000012: return "PowerPC 64-bit";
    default: return "";
    }
}

// CPU type and binary size
//---------------------------------------------------------------------------
struct BinaryInfo {
    int32u cputype;
    int64u size;
    int32u align;
};

//***************************************************************************
// Static stuff
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MachO::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<4)
        return false; //Must wait for more data

    if (!((Buffer[0] == 0xCE && Buffer[1] == 0xFA && Buffer[2] == 0xED && Buffer[3] == 0xFE) || //feedface - 32-bit Mach-O (little-endian)
          (Buffer[0] == 0xCF && Buffer[1] == 0xFA && Buffer[2] == 0xED && Buffer[3] == 0xFE) || //feedfacf - 64-bit Mach-O (little-endian)
          (Buffer[0] == 0xFE && Buffer[1] == 0xED && Buffer[2] == 0xFA && Buffer[3] == 0xCE) || //feedface - 32-bit Mach-O (big-endian)
          (Buffer[0] == 0xFE && Buffer[1] == 0xED && Buffer[2] == 0xFA && Buffer[3] == 0xCF) || //feedfacf - 64-bit Mach-O (big-endian)
          (Buffer[0] == 0xCA && Buffer[1] == 0xFE && Buffer[2] == 0xBA && Buffer[3] == 0xBE) || //cafebabe - 32-bit Universal fat binary
          (Buffer[0] == 0xCA && Buffer[1] == 0xFE && Buffer[2] == 0xBA && Buffer[3] == 0xBF)))  //cafebabf - 64-bit Universal fat binary
    {
        Reject("Mach-O");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_MachO::Read_Buffer_Continue()
{
    //Parsing
    bool isFat{};
    int32u magic, cputype{}, nfat_arch{};
    std::vector<BinaryInfo> binaries;
    Peek_B4(magic);
    if (magic == 0xcefaedfe || magic == 0xcffaedfe) {
        isFat = false;
        Element_Begin1("Mach-O");
        Skip_L4(                                                "magic");
        Get_L4(cputype,                                         "cputype"); Param_Info1(MachO_cputype(cputype));
        Skip_L4(                                                "cpusubtype");
        Skip_L4(                                                "filetype");
        Skip_L4(                                                "ncmds");
        Skip_L4(                                                "sizeofcmds");
        Skip_L4(                                                "flags");
        if (magic == 0xcffaedfe)
            Skip_L4(                                            "reserved");
        Element_End0();
    }
    if (magic == 0xfeedface || magic == 0xfeedfacf) {
        isFat = false;
        Element_Begin1("Mach-O");
        Skip_B4(                                                "magic");
        Get_B4(cputype,                                         "cputype"); Param_Info1(MachO_cputype(cputype));
        Skip_B4(                                                "cpusubtype");
        Skip_B4(                                                "filetype");
        Skip_B4(                                                "ncmds");
        Skip_B4(                                                "sizeofcmds");
        Skip_B4(                                                "flags");
        if (magic == 0xfeedfacf)
            Skip_B4(                                            "reserved");
        Element_End0();
    }
    if (magic == 0xcafebabe || magic == 0xcafebabf) {
        isFat = true;
        Element_Begin1("Universal Binary");
        Skip_B4(                                                "magic");
        Get_B4(nfat_arch,                                       "nfat_arch");
        for (int32u i = 0; i < nfat_arch; ++i) {
            Element_Begin1("Binary");
            BinaryInfo binary{};
            Get_B4(binary.cputype,                              "cputype"); Param_Info1(MachO_cputype(binary.cputype));
            Skip_B4(                                            "cpusubtype");
            if (magic == 0xcafebabe) {
                int32u size32{};
                Skip_B4(                                        "offset");
                Get_B4(size32,                                  "size");
                Get_B4(binary.align,                            "align"); Param_Info1(std::pow(2, binary.align));
                binary.size = size32;
            } else {
                int64u size64{};
                Skip_B8(                                        "offset");
                Get_B8(size64,                                  "size");
                Get_B4(binary.align,                            "align"); Param_Info1(std::pow(2, binary.align));
                Skip_B4(                                        "reserved");
                binary.size = size64;
            }
            binaries.push_back(binary);
            Element_End0();
        }
        Element_End0();
    }

    FILLING_BEGIN();
        Accept("Mach-O");
        if (!isFat) {
            Fill(Stream_General, 0, General_Format, "Mach-O");
            Fill(Stream_General, 0, General_Format_Profile, "Mach Object");
            Fill(Stream_General, 0, General_Format_Profile, MachO_cputype(cputype));
        }
        else {
            Fill(Stream_General, 0, General_Format, "Universal Binary");
            for (size_t i = 0; i < binaries.size(); ++i) {
                Stream_Prepare(Stream_Other);
                Fill(Stream_Other, i, Other_Type, "Binary");
                Fill(Stream_Other, i, Other_Format, "Mach-O");
                Fill(Stream_Other, i, Other_Format_Profile, "Mach Object");
                Fill(Stream_Other, i, Other_Format_Profile, MachO_cputype(binaries[i].cputype));
                Fill(Stream_Other, i, Other_StreamSize, binaries[i].size);
                Fill(Stream_Other, i, "Alignment/String", Ztring::ToZtring(std::pow(2, binaries[i].align) / 1024, 0) + __T(" KiB"));
            }
        }
        Finish("Mach-O");
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_MACHO_YES
