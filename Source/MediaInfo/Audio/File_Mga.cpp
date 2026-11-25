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
#if defined(MEDIAINFO_MGA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Mga.h"
#if defined(MEDIAINFO_ADM_YES)
    #include "MediaInfo/Audio/File_Adm.h"
#endif
#include "MediaInfo/File_Unknown.h"
#include <zlib.h>
#include <algorithm>
#include <string>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Mga_Section_Names[]=
{
    "Audio Essence",
    "Audio Metadata Pack",
    "Audio Metadata Payload",
    "UL-defined metadata",
};
static const size_t Mga_Section_Names_Size=sizeof(Mga_Section_Names)/sizeof(*Mga_Section_Names);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mga::File_Mga()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=5; // Needed for catching 1.001 frame rates in ADM

    //Temp
    Parser=nullptr;
}

//---------------------------------------------------------------------------
File_Mga::~File_Mga()
{
    delete Parser;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mga::Streams_Accept()
{
    //Filling
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "MGA");
}

void File_Mga::Streams_Finish()
{
    //Filling
    if (Parser)
    {
        Finish(Parser);
        Merge(*Parser, Stream_Audio, 0, 0);
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Mga::Read_Buffer_Unsynched()
{
    if (Checks.Frame_Count)
        Checks.IsStopped=true;
}
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mga::Header_Parse()
{
    //Temp
    Accept();

    //Parsing
    int8u SectionCount;
    Get_B1 (SectionCount,                                       "Section Count");
    for (int8u i=0; i<SectionCount; i++)
    {
        if (Element_Offset>Element_Size || Element_Size-Element_Offset<6)
        {
            Element_WaitForMoreData();
            return;
        }
        Element_Offset+=2;
        Element_Offset+=4+BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    }

    //Filling
    Header_Fill_Size(Element_Offset);
    Header_Fill_Code(SectionCount, "MGA Frame");
    Element_Offset=1;
}

//---------------------------------------------------------------------------
void File_Mga::Data_Parse()
{
    int8u SectionCount=(int8u)Element_Code;
    auto Metadata_IsPresent=false;
    auto Metadata_IsEmpty=false;

    #if MEDIAINFO_CONFORMANCE
        std::map<int64u, int64u> Sections;
        int8u Index_Expected=1;
        if (SectionCount<2 && !Checks.IsStopped)
            Checks.Errors.insert("Section Count is less than 2");
    #endif //MEDIAINFO_CONFORMANCE

    //Parsing
    for (int8u i=0; i<SectionCount; i++)
    {
        Element_Begin1("Section");
        if (Element_Offset>Element_Size || Element_Size-Element_Offset<6)
        {
            Trusted_IsNot("Truncated section header");
            Element_End0();
            return;
        }
        Element_Begin1("Header");
            int32u Length;
            int8u Index, Identifier;
            Get_B1 (Index,                                      "Index");
            Get_B1 (Identifier,                                 "Identifier");
            Get_B4 (Length,                                     "Length");
        Element_End0();
        Element_Info1(Identifier<Mga_Section_Names_Size?Mga_Section_Names[Identifier]:(Identifier==0xFF?"Fill":to_string(Identifier).c_str()));
        if (!Trusted_Get() || Element_IsWaitingForMoreData()
         || Element_Offset>Element_Size || Length>Element_Size-Element_Offset
         || Buffer_Offset>Buffer_Size || Element_Offset>Buffer_Size-Buffer_Offset
         || Length>Buffer_Size-Buffer_Offset-Element_Offset)
        {
            Trusted_IsNot("Invalid section length");
            Element_End0();
            return;
        }
        auto End=Element_Offset+Length;
        auto Element_Size_Save=Element_Size;
        Element_Size=End;
        #if MEDIAINFO_CONFORMANCE
            if (Checks.IsStopped)
                ;
            else if (!i)
            {
                if (Index || Identifier)
                    Checks.Errors.insert("first section is not the Audio Essence one");
                if (Checks.AudioEssence_Lengths.size()<16)
                    Checks.AudioEssence_Lengths.insert(Length);
            }
            else if (Index==0xFF || Identifier==0xFF)
            {
                if (Index!=Identifier || i+1!=SectionCount)
                    Checks.Errors.insert("fill section is not valid or not the last section");
                else if (Index==0xFF)
                {
                    const auto Fill_Begin=Buffer+Buffer_Offset+(size_t)Element_Offset;
                    if (any_of(Fill_Begin, Fill_Begin+Length, [](int8u Value) { return Value!=0; }))
                        Checks.Errors.insert("fill section contains bytes which are not 0");
                }
            }
            else
            {
                if (Index!=Index_Expected++)
                    Checks.Errors.insert("indexes of the Audio Metadata sections are not incremented from 1");
                if (!Identifier || Identifier>3)
                    Checks.Errors.insert("Identifier of an Audio Metadata section is not 1 to 3");
                Sections[Index]=Identifier;
                if (Identifier==2 && Length)
                {
                    const auto Tag=Buffer[Buffer_Offset+(size_t)Element_Offset];
                    if (Tag<0x80)
                        Checks.Payloads.emplace(Index, Tag);
                }
            }
        #endif //MEDIAINFO_CONFORMANCE
        if (i && Identifier!=0xFF)
            (Length?Metadata_IsPresent:Metadata_IsEmpty)=true;
        switch (Identifier)
        {
            case 0x00 : Skip_XX(Length,                         "PCM data");
                        break;
            case 0x02 : if (Length)
                            AudioMetadataPayload();
                        break;
        }
        if (Element_Offset<End)
            Skip_XX(End-Element_Offset,                         "(Unknown)");
        Element_Size=Element_Size_Save;
        Element_End0();
    }

    FILLING_BEGIN()
        #if MEDIAINFO_CONFORMANCE
            if (!Checks.IsStopped)
            {
                if (Sections.empty())
                    Checks.Errors.insert("no Audio Metadata section");
                if (!Checks.Frame_Count)
                    Checks.Sections=Sections;
                else if (Checks.Sections.size()!=Sections.size())
                    Checks.Errors.insert("count of Audio Metadata sections is not constant");
                Checks.Frame_Count++;
            }
        #endif //MEDIAINFO_CONFORMANCE
        if (Metadata_IsPresent || !Metadata_IsEmpty) //Metadata is in next frames if it is empty in this one
            Frame_Count++;
        if (Frame_Count>=Frame_Count_Valid)
        {
            Fill();
            Finish();
        }
    FILLING_END()
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mga::AudioMetadataPayload()
{
    //Parsing
    Element_Begin1("Audio Metadata Payload");
    Element_Begin1("Header");
    int64u Tag, Length;
    Get_BER(Tag,                                                "Tag");
    Get_BER(Length,                                             "Length");
    Element_End0();
    if (!Trusted_Get() || Element_IsWaitingForMoreData() || Tag==(int64u)-1
     || Element_Offset>Element_Size || Length>Element_Size-Element_Offset
     || (Tag==0x12 && Length<2))
    {
        Trusted_IsNot("Invalid metadata length");
        Element_End0();
        return;
    }
    auto End=Element_Offset+Length;
    switch (Tag)
    {
        case 0x12 : SerialAudioDefinitionModelMetadataPayload(Length); break;
        default   : Element_Name(to_string(Tag).c_str()); break;
    }
    if (Element_Offset<End)
        Skip_XX(End-Element_Offset, "(Unknown)");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mga::SerialAudioDefinitionModelMetadataPayload(int64u Length)
{
    Element_Begin1("Serial Audio Definition Model Metadata Payload");

    //Parsing
    Element_Begin1("Header");
    int8u Version, Format;
    Get_B1(Version,                                             "Version");
    Get_B1(Format,                                              "Format");
    Element_End0();
    #if MEDIAINFO_CONFORMANCE
        if (!Checks.IsStopped && Trusted_Get())
        {
            if (Version)
                Checks.Errors.insert("Version of the S-ADM payload is not 0");
            if (Format>1)
                Checks.Errors.insert("Format of the S-ADM payload is not 0 or 1");
        }
    #endif //MEDIAINFO_CONFORMANCE
    if (Format>1)
    {
        Element_End0();
        return;
    }
    if (Format==1 && Length-2>(int64u)((uInt)-1))
    {
        Trusted_IsNot("Metadata is too large");
        Element_End0();
        return;
    }

    int8u* UncompressedData=NULL;
    size_t UncompressedData_Size=0;
    if (Format==1)
    {
        //Uncompress init
        z_stream strm;
        strm.next_in=(Bytef*)Buffer+Buffer_Offset+(size_t)Element_Offset;
        strm.avail_in=(uInt)(Length-2);
        strm.next_out=NULL;
        strm.avail_out=0;
        strm.total_out=0;
        strm.zalloc=Z_NULL;
        strm.zfree=Z_NULL;
        inflateInit2(&strm, 15+16); // 15 + 16 are magic values for gzip

        //Prepare out
        strm.avail_out=0x10000; //Blocks of 64 KiB, arbitrary chosen, as a begin
        strm.next_out=(Bytef*)new Bytef[strm.avail_out];

        //Parse compressed data, with handling of the case the output buffer is not big enough
        for (;;)
        {
            //inflate
            int inflate_Result=inflate(&strm, Z_NO_FLUSH);
            if (inflate_Result<0)
                break;

            //Check if we need to stop
            if (strm.avail_out || inflate_Result || strm.total_out>=0x4000000) //Arbitrary limit of 64 MiB
                break;

            //Need to increase buffer
            size_t UncompressedData_NewMaxSize=strm.total_out*4;
            int8u* UncompressedData_New=new int8u[UncompressedData_NewMaxSize];
            memcpy(UncompressedData_New, strm.next_out-strm.total_out, strm.total_out);
            delete[](strm.next_out - strm.total_out); strm.next_out=UncompressedData_New;
            strm.next_out=strm.next_out+strm.total_out;
            strm.avail_out=UncompressedData_NewMaxSize-strm.total_out;
        }
        UncompressedData=strm.next_out-strm.total_out;
        UncompressedData_Size=strm.total_out;
        inflateEnd(&strm);
    }

    #if defined(MEDIAINFO_ADM_YES)
    {
    if (UncompressedData || Element_Offset<Element_Size)
    {
        if (!Parser)
        {
            Parser=new File_Adm();
            ((File_Adm*)Parser)->MuxingMode="SMPTE ST 2127-1 / SMPTE ST 2109 / SMPTE ST 2127-10";
            Open_Buffer_Init(Parser);
        }
    }
    }
    #else
    {
        //Filling
        if (!Parser)
        {
            Parser=new File_Unknown();
            Open_Buffer_Init(Parser);
            Parser->Accept();
            Parser->Stream_Prepare(Stream_Audio);
            Parser->Fill(Stream_Audio, 0, "Metadata_Format", "ADM");
            Parser->Finish();
        }
    }
    #endif

    if (Parser)
    {
        if (UncompressedData)
        {
            Open_Buffer_Continue(Parser, UncompressedData, UncompressedData_Size);
            delete[] UncompressedData;
        }
        else            
        {
            Open_Buffer_Continue(Parser, Buffer+Buffer_Offset+(size_t)Element_Offset, (size_t)(Length-2));
        }
        #if MEDIAINFO_CONFORMANCE && defined(MEDIAINFO_ADM_YES)
            //The ADM parser keeps only the content of the last frame
            const auto AdmContent=(File_Adm*)Parser;
            for (const auto& ID : AdmContent->ID_List("audioProgramme"))
                if (Checks.Programmes.size()<0x1000 && Checks.Programmes.find(ID)==Checks.Programmes.end()) //Arbitrary limit
                {
                    auto& Programme=Checks.Programmes[ID];
                    AdmContent->audioProgramme_Get(ID, Programme.Name, Programme.Label);
                }
            const auto FrameType=AdmContent->FrameType_Get();
            if (!FrameType.empty() && Checks.Adm_FrameTypes.size()<16)
                Checks.Adm_FrameTypes.insert(FrameType);
        #endif //MEDIAINFO_CONFORMANCE && defined(MEDIAINFO_ADM_YES)
    }

    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mga::Get_BER(int64u &Value, const char* Name)
{
    int8u Length;
    Get_B1(Length,                                              Name);
    if (Length<0x80)
    {
        Value=Length; //1-byte
        return;
    }

    Length&=0x7F;
    switch (Length)
    {
        case 1 :
                {
                int8u  Length1;
                Get_B1(Length1,                                 Name);
                Value=Length1;
                break;
                }
        case 2 :
                {
                int16u Length2;
                Get_B2(Length2,                                 Name);
                Value=Length2;
                break;
                }
        case 3 :
                {
                int32u Length3;
                Get_B3(Length3,                                 Name);
                Value=Length3;
                break;
                }
        case 4 :
                {
                int32u Length4;
                Get_B4(Length4,                                 Name);
                Value=Length4;
                break;
                }
        case 5 :
                {
                int64u Length5;
                Get_B5(Length5,                                 Name);
                Value=Length5;
                break;
                }
        case 6 :
                {
                int64u Length6;
                Get_B6(Length6,                                 Name);
                Value=Length6;
                break;
                }
        case 7 :
                {
                int64u Length7;
                Get_B7(Length7,                                 Name);
                Value=Length7;
                break;
                }
        case 8 :
                {
                int64u Length8;
                Get_B8(Length8,                                 Name);
                Value=Length8;
                break;
                }
        default:Value=(int64u)-1; //Problem
    }
}

} //NameSpace

#endif //MEDIAINFO_MGA_YES
