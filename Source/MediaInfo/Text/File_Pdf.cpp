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
#if defined(MEDIAINFO_PDF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Pdf.h"
#if defined(MEDIAINFO_XMP_YES)
    #include "MediaInfo/Tag/File_Xmp.h"
#endif
#include <zlib.h>
#include <cstdlib>
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static inline bool Pdf_IsSpace(int8u Value)
{
    return Value==' ' || Value=='\r' || Value=='\n' || Value=='\t' || Value=='\f' || Value=='\0';
}

//---------------------------------------------------------------------------
static inline bool Pdf_IsDelimiter(int8u Value)
{
    return Value=='/' || Value=='(' || Value==')' || Value=='<' || Value=='>' || Value=='[' || Value==']' || Value=='{' || Value=='}' || Value=='%';
}

//---------------------------------------------------------------------------
static size_t Pdf_String_End(const int8u* Data, size_t Size, size_t Pos)
{
    //Position after the literal string beginning at Pos
    size_t Level=0;
    for (; Pos<Size; Pos++)
        switch (Data[Pos])
        {
            case '\\': Pos++; break; //Escaped character
            case '(' : Level++; break;
            case ')' : if (!--Level) return Pos+1; break;
            default  : ;
        }
    return Size;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Pdf::File_Pdf()
:File__Analyze()
{
    IsEncrypted=false;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pdf::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "PDF");

    Stream_Prepare(Stream_Text);
    Fill(Stream_Text, 0, "Format", "PDF");
}

//---------------------------------------------------------------------------
void File_Pdf::Streams_Finish()
{
    Fill(Stream_General, 0, General_Encoded_Date, Info_ModDate.empty()?Info_CreationDate:Info_ModDate);
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Pdf::FileHeader_Begin()
{
    //Synchro
    if (5>Buffer_Size)
        return false;
    if (Buffer[0]!=0x25 //"%PDF-"
     || Buffer[1]!=0x50
     || Buffer[2]!=0x44
     || Buffer[3]!=0x46
     || Buffer[4]!=0x2D)
    {
        Reject();
        return false;
    }

    Accept();

    //Temp
    Catalog_Level=0;
    Offsets_Max=0;
    Objects_Current=Objects.end();

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Pdf::FileHeader_Parse()
{
    string PdfHeader;
    Get_String(SizeOfLine(), PdfHeader,                         "Header");
    for (;;)
    {
        int64u CommentSize=SizeOfLine();
        if (Buffer_Offset+Element_Offset>=Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }
        if (Buffer[Buffer_Offset+Element_Offset]!='%')
            break;
        Skip_String(CommentSize,                                "Comment");
    }

    //Filling
    Fill(Stream_General, 0, General_Format_Version, PdfHeader.substr(5));

    GoToFromEnd(1024); //"startxref", the offset of the cross-reference data and "%%EOF" are at the end of the file, some bytes after them are accepted
    State=State_Parsing_startxref;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pdf::Read_Buffer_Continue()
{
    switch (State)
    {
        case State_Parsing_xref         : xref(); break;
        case State_Parsing_startxref    : startxref(); break;
        case State_Parsing_object       : break; //Using elements
        default                         : Finish();
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Pdf::Header_Begin()
{
    //Offsets_Current=Offsets.find(Objects_Current->second.Offset);
    //offsets::iterator Offsets_Next=Offsets_Current;
    //Offsets_Next++;
    //if (Offsets_Next!=Offsets.end() && Offsets_Next->first>File_Offset+Buffer_Size)
    //{
    //    Element_WaitForMoreData();
    //    return false;
    //}

    return true;
}

//---------------------------------------------------------------------------
void File_Pdf::Header_Parse()
{
    offsets::iterator Offsets_Next=upper_bound(Offsets.begin(), Offsets.end(), File_Offset+Buffer_Offset);
    if (Offsets_Next!=Offsets.end() && *Offsets_Next>File_Offset+Buffer_Size)
    {
        Element_WaitForMoreData();
        return;
    }

    int64u Size;
    //if (Offsets_Current==Offsets.end())
    //    Size=Offsets_Max-(File_Offset+Buffer_Offset);
    //else
    //    Size=Offsets_Current->first-(File_Offset+Buffer_Offset);
    if (Offsets_Next==Offsets.end())
        Size=Offsets_Max-(File_Offset+Buffer_Offset);
    else
        Size=*Offsets_Next-(File_Offset+Buffer_Offset);

    Header_Fill_Size(Size);
}

//---------------------------------------------------------------------------
void File_Pdf::Data_Parse()
{
    Element_Name("Object");

    string Line;
    const int64u ObjectHeader_Size=SizeOfObjectHeader();
    Get_String(ObjectHeader_Size?ObjectHeader_Size:SizeOfLine(), Line, "Header");

    size_t Space_Pos=Line.find(' ');
    int32u ObjectNumber=Ztring().From_UTF8(Line.substr(0, Space_Pos)).To_int32u();
    Element_Info1(ObjectNumber);
    objects::iterator Object=Objects.find(ObjectNumber);
    if (Objects_Current!=Objects.end() && Objects_Current->second.ObjectStream!=(int32u)-1 && Objects_Current->second.ObjectStream==ObjectNumber)
        Object_ObjStm();
    else if (Object==Objects.end())
        Skip_XX(Element_Size-Element_Offset,                    "Data");
    else
        switch(Object->second.Type)
        {
            case Type_Root      : Object_Root(); break;
            case Type_Info      : Object_Info(); break;
            case Type_Metadata  : Object_Metadata(); break;
            case Type_InfoValue : Object_InfoValue(ObjectNumber); break;
            default             : Skip_XX(Element_Size-Element_Offset, "Data");
        }

    Objects_Next();
}

//---------------------------------------------------------------------------
void File_Pdf::Objects_Next()
{
    for (;;)
    {
        if (Objects_Current==Objects.end())
            break;

        Objects_Current->second.BottomPos++;
        if (Objects_Current->second.BottomPos>=Objects_Current->second.Bottoms.size())
        {
            if (Objects_Current->first==(int32u)-1)
            {
                //No more to parse
                Objects_Current=Objects.end();
                Objects.clear();
                Finish();
                break;
            }

            Objects_Current=Objects.find(Objects_Current->second.TopObject);
            continue;
        }

        Objects_Current=Objects.find(Objects_Current->second.Bottoms[Objects_Current->second.BottomPos]);

        //Offset of the object, or of the object stream it is in
        int64u Offset=Objects_Current->second.Offset;
        if (Objects_Current->second.ObjectStream!=(int32u)-1)
        {
            objects::iterator ObjectStream=Objects.find(Objects_Current->second.ObjectStream);
            Offset=ObjectStream==Objects.end()?(int64u)-1:ObjectStream->second.Offset;
        }
        if (Offset>=File_Size)
            continue; //Not in the file
        GoTo(Offset);
        break;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pdf::xref()
{
    //Cross-reference stream, which is an object
    if (SizeOfObjectHeader())
    {
        xref_Stream();
        return;
    }

    //Parsing
    Element_Begin1("Cross-Reference Table");

    Element_Begin1("Cross-Reference Section");

    string FirstLine;
    Get_String(SizeOfLine(), FirstLine,                             "Object name");
    if (FirstLine!="xref")
    {
        //Problem
        Skip_XX(Element_Size-Element_Offset,                        "(Problem)");
        Element_End0();
        Element_End0();
        trailer();
        return;
    }
    const size_t Offsets_Size=Offsets.size();
    for (;;)
    {
        //A section has 1 or more subsections, then the trailer
        Peek_String(SizeOfLine(), FirstLine);
        if (FirstLine.empty() || FirstLine[0]<'0' || FirstLine[0]>'9')
            break;

        Element_Begin1("Cross-Reference SubSection");
        Skip_String(FirstLine.size(),                               "Header");
        size_t FirstLine_Space=FirstLine.find(' ');
        int32u Base=atoi((const char*)FirstLine.c_str());
        int32u Count=0;
        if (FirstLine_Space!=string::npos)
            Count=atoi((const char*)FirstLine.c_str()+FirstLine_Space+1);

        while (Element_Offset<Element_Size && (Buffer[Buffer_Offset+(size_t)Element_Offset]=='\r' || Buffer[Buffer_Offset+(size_t)Element_Offset]=='\n'))
            Element_Offset++;
        if (Count>(Element_Size-Element_Offset)/20)
        {
            if (File_Offset+Buffer_Size<File_Size)
            {
                // We wait for more data
                Offsets.resize(Offsets_Size);
                Buffer_Offset=0;
                Element_Offset=0;
                Element_DoNotShow();
                Element_WaitForMoreData();
            }
            else
                Skip_XX(Element_Size-Element_Offset,                "(Problem)");
            Element_End0();
            Element_End0();
            Element_End0();
            if (!Element_IsWaitingForMoreData())
                trailer();
            return;
        }

        const int8u* Buffer_Temp=Buffer+Buffer_Offset+(size_t)Element_Offset+17;
        for (int32u Pos=0; Pos<Count; ++Pos)
        {
            if (*Buffer_Temp=='n')
            {
                char atoi_buf[18];
                atoi_buf[17]=0;
                memcpy(atoi_buf,(const char*)Buffer_Temp-17,17);
                const int64u Offset=strtoull(atoi_buf, NULL, 10);
                object& Object=Objects[Base+Pos];
                if (!Object.IsSet()) //The sections are parsed from the last update of the file to the first one, the last content of an object is the one to parse
                    Object.Offset=Offset;
                //Offsets[Offset]=Base+Pos;
                Offsets.push_back(Offset);
            }
            Buffer_Temp+=20;
            if (Pos>100)
                Element_Offset+=20;
            else
            {
                Skip_String(18,                                 "Entry"); Param_Info1(Base+Pos);
                Element_Offset+=2; //Skipping spaces at end and line return
            }
        }
        Element_End0();
    }

    //The trailer must be in the buffer too, it is followed by the offset of the cross-reference data
    static const char Keyword[]="startxref";
    const int8u* Trailer=Buffer+Buffer_Offset+(size_t)Element_Offset;
    if (File_Offset+Buffer_Size<File_Size && Buffer+Buffer_Size-Trailer<0x10000 && search(Trailer, Buffer+Buffer_Size, Keyword, Keyword+9)==Buffer+Buffer_Size)
    {
        // We wait for more data
        Offsets.resize(Offsets_Size);
        Buffer_Offset=0;
        Element_Offset=0;
        Element_DoNotShow();
        Element_WaitForMoreData();
        Element_End0();
        Element_End0();
        return;
    }

    Element_End0();

    Element_End0();

    if (File_Offset+Buffer_Offset>Offsets_Max)
        Offsets_Max=File_Offset+Buffer_Offset;

    trailer();
}

//---------------------------------------------------------------------------
static vector<int64u> Pdf_Numbers(const Ztring& Array)
{
    vector<int64u> Numbers;
    const string Text=Array.To_UTF8();
    for (size_t Pos=0; Pos<Text.size(); Pos++)
    {
        if (Text[Pos]<'0' || Text[Pos]>'9')
            continue;
        int64u Number=0;
        for (; Pos<Text.size() && Text[Pos]>='0' && Text[Pos]<='9'; Pos++)
            Number=Number*10+(Text[Pos]-'0');
        Numbers.push_back(Number);
    }
    return Numbers;
}

//---------------------------------------------------------------------------
void File_Pdf::xref_Stream()
{
    Element_Begin1("Cross-Reference Stream");

    //Parsing
    Skip_String(SizeOfObjectHeader(),                               "Header");
    stream_data Stream;
    const bool IsStream=Get_Stream(Stream);
    if (Stream.IsTruncated && File_Offset+Buffer_Size<File_Size)
    {
        // We wait for more data
        Element_Offset=0;
        Element_DoNotShow();
        Element_WaitForMoreData();
        Element_End0();
        return;
    }

    int64u Prev=(int64u)-1;
    Ztring Root, Info;
    if (!IsStream)
        Skip_XX(Element_Size-Element_Offset,                        "(Problem)");
    else
    {
        Skip_Stream(Stream);

        //Entries
        if (Stream.IsTruncated)
            Trusted_IsNot("Stream is not complete");
        else if (Get_Stream_Decoded(Stream))
        {
            const vector<int64u> W=Pdf_Numbers(Stream.Dictionary["W"]);
            vector<int64u> Index=Pdf_Numbers(Stream.Dictionary["Index"]);
            if (W.size()!=3 || W[0]>8 || !W[1] || W[1]>8 || W[2]>8)
                Trusted_IsNot("Sizes of the fields of the entries are not valid");
            else
            {
                if (Index.empty())
                {
                    Index.push_back(0);
                    Index.push_back(Stream.Dictionary["Size"].To_int64u());
                }
                const int64u Entry_Size=W[0]+W[1]+W[2];
                const int64u Objects_Max=File_Size/16<0x10000?0x10000:File_Size/16; //Arbitrary limit on the count of objects kept
                const int8u* Entry=Stream.Data;
                int64u Remaining=Stream.Size;
                for (size_t Pos=0; Pos+1<Index.size(); Pos+=2)
                {
                    int64u Count=Index[Pos+1];
                    Loop_CheckValue(Count, Remaining, Entry_Size, "Count");
                    Remaining-=Count*Entry_Size;
                    for (int64u Number=Index[Pos]; Count; Number++, Count--)
                    {
                        int64u Type=W[0]?0:1; //In use and not in an object stream when there is no such field
                        for (int64u Byte=0; Byte<W[0]; Byte++)
                            Type=(Type<<8)|*Entry++;
                        int64u Value=0;
                        for (int64u Byte=0; Byte<W[1]; Byte++)
                            Value=(Value<<8)|*Entry++;
                        Entry+=W[2];
                        if ((Type!=1 && Type!=2) || Value>=(Type==1?(int64u)-1:(int32u)-1) || Objects.size()>=Objects_Max)
                            continue; //Free, or not kept
                        if (Type==1)
                            Offsets.push_back(Value);
                        object& Object=Objects[(int32u)Number];
                        if (Object.IsSet())
                            continue; //The sections are parsed from the last update of the file to the first one, the last content of an object is the one to parse
                        if (Type==1)
                            Object.Offset=Value;
                        else
                            Object.ObjectStream=(int32u)Value;
                    }
                }
            }
        }

        //The dictionary of the stream has the content of a trailer
        Root=Stream.Dictionary["Root"];
        Info=Stream.Dictionary["Info"];
        if (Stream.Dictionary.find("Encrypt")!=Stream.Dictionary.end())
            IsEncrypted=true;
        const Ztring& Previous=Stream.Dictionary["Prev"];
        if (!Previous.empty())
            Prev=Previous.To_int64u();
        if (Prev!=(int64u)-1 && find(xref_Offsets.begin(), xref_Offsets.end(), Prev)!=xref_Offsets.end())
        {
            Trusted_IsNot("Cross-reference section already parsed");
            Prev=(int64u)-1;
        }
    }

    Element_End0();

    if (File_Offset+Buffer_Offset>Offsets_Max)
        Offsets_Max=File_Offset+Buffer_Offset;

    xref_End(Prev, Root, Info);
}

//---------------------------------------------------------------------------
void File_Pdf::xref_End(int64u Prev, const Ztring &Root, const Ztring &Info)
{
    //Catalog and document information, the ones of the last version of the file (its section is the first one parsed)
    if (!Root.empty() && Objects.find((int32u)-1)==Objects.end())
        for (size_t Pos=0; Pos<2; Pos++)
        {
            const Ztring& Value=Pos?Info:Root;
            if (Value.empty())
                continue;
            const int32u ObjectNumber=Value.To_int32u();
            Objects[ObjectNumber].Type=Pos?Type_Info:Type_Root;
            Objects[ObjectNumber].TopObject=(int32u)-1;
            Objects[(int32u)-1].Bottoms.push_back(ObjectNumber);
        }

    //Previous Cross-Reference Table
    if (Prev!=(int64u)-1)
    {
        xref_Offsets.push_back(Prev);
        GoTo(Prev);
        return;
    }

    objects::iterator Object_Top=Objects.find((int32u)-1);
    if (Offsets.empty() || Object_Top==Objects.end())
    {
        ForceFinish();
        return;
    }

    sort(Offsets.begin(), Offsets.end());

    //Offsets_Current=Offsets.end(); //No more used for the moment
    State=State_Parsing_object;
    Objects_Current=Object_Top;
    Objects_Next();
}

//---------------------------------------------------------------------------
void File_Pdf::trailer()
{
    Element_Begin1("Trailer");

    //Parsing
    int64u Prev=(int64u)-1;
    string Key;
    Ztring Value, Root, Info;
    Get_String(SizeOfLine(), Key,                                   "Object name");
    if (Key!="trailer")
    {
        //Problem
        Skip_XX(Element_Size-Element_Offset,                        "(Problem)");
    }
    Skip_String(SizeOfLine(),                                       "Object name");
    while (Element_Offset<Element_Size)
    {
        if (Get_Next(Key, Value))
        {
            for (;;)
            {
                Get_Next(Key, Value);
                if (Key.empty())
                    break;
                else if (Key=="Root")
                {
                    Root=Value;
                    Param_Info1(__T("Document Catalog is at offset 0x"+Ztring().From_Number(Objects[Value.To_int32u()].Offset, 16)));
                }
                else if (Key=="Info")
                {
                    Info=Value;
                    Param_Info1(__T("Info is at offset 0x"+Ztring().From_Number(Objects[Value.To_int32u()].Offset, 16)));
                }
                else if (Key=="Encrypt")
                    IsEncrypted=true;
                else if (Key=="Prev")
                {
                    Prev=Value.To_int64u();
                    Param_Info1(__T("Previous Cross-Reference Table is at offset 0x"+Ztring().From_Number(Prev, 16)));
                }
            }

            continue;
        }

        if (Key.empty())
            break;
    }

    if (Prev!=(int64u)-1 && find(xref_Offsets.begin(), xref_Offsets.end(), Prev)!=xref_Offsets.end())
    {
        Trusted_IsNot("Cross-reference section already parsed");
        Prev=(int64u)-1;
    }

    Element_End0();

    xref_End(Prev, Root, Info);
}

//---------------------------------------------------------------------------
void File_Pdf::startxref()
{
    if (File_Size!=(int64u)-1 && File_Offset+Buffer_Size<File_Size)
    {
        Element_WaitForMoreData();
        return;
    }

    //The offset of the cross-reference data is after the last "startxref" of the file
    static const char Keyword[]="startxref";
    const int8u* Begin=find_end(Buffer+Buffer_Offset, Buffer+Buffer_Size, Keyword, Keyword+9);
    if (Begin==Buffer+Buffer_Size)
    {
        Trusted_IsNot("Offset of the cross-reference data not found");
        ForceFinish();
        return;
    }
    Buffer_Offset=Begin-Buffer;
    Element_Size=Buffer_Size-Buffer_Offset;

    //Parsing
    Element_Begin1("Cross-Reference Table Offset");
    string xrefOffsetS;
    Skip_String(9,                                              "Object name");
    Get_String (SizeOfLine(), xrefOffsetS,                      "xref Offset");
    Element_End0();
    Element_Begin1("End Of File");
    Skip_String(SizeOfLine(),                                   "Object name");
    Element_End0();
    const int64u xref_Offset=strtoull(xrefOffsetS.c_str(), NULL, 10);

    //Going to xref
    xref_Offsets.push_back(xref_Offset);
    if (xref_Offset>Offsets_Max)
        Offsets_Max=xref_Offset;
    GoTo (xref_Offset);
    State=State_Parsing_xref;
}

//---------------------------------------------------------------------------
void File_Pdf::Object_Root()
{
    Element_Info1("Document Catalog");

    //Parsing
    string Key;
    Ztring Value;
    while (Element_Offset<Element_Size)
    {
        if (Get_Next(Key, Value))
        {
            for (;;)
            {
                Get_Next(Key, Value);
                if (Key.empty())
                    break;
                else if (Key=="Metadata")
                {
                    int32u ObjectNumber=Value.To_int32u();
                    object& Metadata=Objects[ObjectNumber];
                    if (Metadata.Type==Type_Root || Metadata.Type==Type_Info)
                    {
                        Trusted_IsNot("Metadata is the document catalog or the document information");
                        return;
                    }
                    if (Metadata.Type==Type_Other && ObjectNumber!=(int32u)-1)
                    {
                        Metadata.Type=Type_Metadata;
                        Metadata.TopObject=Objects_Current->first;
                        Objects_Current->second.Bottoms.push_back(ObjectNumber);
                    }
                    Param_Info1(__T("Metadata is at offset 0x"+Ztring().From_Number(Objects[ObjectNumber].Offset)));
                }
            }

            continue;
        }

        if (Key.empty())
            break;
    }
}

//---------------------------------------------------------------------------
static Ztring Pdf_String(const string& Raw)
{
    string Bytes;
    if (Raw.size()>=2 && Raw[0]=='(')
    {
        //Literal string
        size_t Level=1;
        for (size_t Pos=1; Pos<Raw.size(); Pos++)
        {
            char Byte=Raw[Pos];
            if (Byte=='\\')
            {
                if (++Pos>=Raw.size())
                    break;
                Byte=Raw[Pos];
                switch (Byte)
                {
                    case 'n' : Byte='\n'; break;
                    case 'r' : Byte='\r'; break;
                    case 't' : Byte='\t'; break;
                    case 'b' : Byte='\b'; break;
                    case 'f' : Byte='\f'; break;
                    default   :
                                if (Byte>='0' && Byte<='7')
                                {
                                    //Up to 3 octal digits
                                    int8u Code=(int8u)(Byte-'0');
                                    for (size_t Digits=1; Digits<3 && Pos+1<Raw.size() && Raw[Pos+1]>='0' && Raw[Pos+1]<='7'; Digits++)
                                        Code=(int8u)((Code<<3)|(Raw[++Pos]-'0'));
                                    Byte=(char)Code;
                                }
                }
            }
            else if (Byte=='(')
                Level++;
            else if (Byte==')' && !--Level)
                break;
            Bytes+=Byte;
        }
    }
    else if (Raw.size()>=2 && Raw[0]=='<')
    {
        //Hexadecimal string
        bool HasHigh=false;
        int8u High=0;
        for (size_t Pos=1; Pos<Raw.size() && Raw[Pos]!='>'; Pos++)
        {
            const char Digit=Raw[Pos];
            int8u Value;
                 if (Digit>='0' && Digit<='9') Value=(int8u)(Digit-'0');
            else if (Digit>='A' && Digit<='F') Value=(int8u)(Digit-'A'+10);
            else if (Digit>='a' && Digit<='f') Value=(int8u)(Digit-'a'+10);
            else
                continue;
            if (HasHigh)
                Bytes+=(char)((High<<4)|Value);
            else
                High=Value;
            HasHigh=!HasHigh;
        }
        if (HasHigh)
            Bytes+=(char)(High<<4);
    }

    //Encoding
    Ztring Text;
    if (Bytes.size()>=2 && (int8u)Bytes[0]==0xFE && (int8u)Bytes[1]==0xFF)
        Text.From_UTF16BE(Bytes.c_str()+2, Bytes.size()-2);
    else if (Bytes.size()>=3 && (int8u)Bytes[0]==0xEF && (int8u)Bytes[1]==0xBB && (int8u)Bytes[2]==0xBF)
        Text.From_UTF8(Bytes.c_str()+3, Bytes.size()-3);
    else
        Text.From_ISO_8859_1(Bytes.c_str(), Bytes.size()); //PDFDocEncoding, which is ISO 8859-1 for most of the characters
    return Text;
}

//---------------------------------------------------------------------------
static Ztring Pdf_Date(const Ztring& Value)
{
    //"D:YYYYMMDDHHmmSS" then "Z" or the offset from UTC as +HH'mm', everything after the year may be missing
    const string Date=Value.To_UTF8();
    size_t Pos=Date.compare(0, 2, "D:")?0:2;
    const size_t Digits=min(Date.find_first_not_of("0123456789", Pos), Date.size())-Pos;
    if (Digits<4 || Digits>14 || Digits%2)
        return Value;
    static const char Separators[]="-- ::";
    string Result=Date.substr(Pos, 4);
    for (size_t Part=0; 4+Part*2<Digits; Part++)
    {
        Result+=Separators[Part];
        Result+=Date.substr(Pos+4+Part*2, 2);
    }
    Pos+=Digits;
    if (Digits==14 && Pos<Date.size())
    {
        if (Date[Pos]=='Z')
            Result+=" UTC";
        else if ((Date[Pos]=='+' || Date[Pos]=='-') && Pos+3<=Date.size() && Date.find_first_not_of("0123456789", Pos+1)>=Pos+3)
        {
            Result+=Date.substr(Pos, 3);
            Result+=':';
            Pos+=3;
            if (Pos<Date.size() && Date[Pos]=='\'')
                Pos++;
            if (Pos+2<=Date.size() && Date.find_first_not_of("0123456789", Pos)>=Pos+2)
                Result+=Date.substr(Pos, 2);
            else
                Result+="00";
        }
    }
    return Ztring().From_UTF8(Result);
}

//---------------------------------------------------------------------------
static size_t Pdf_Info_Parameter(const string& Key)
{
    //Same parameters as the ones filled from the XMP metadata
    if (Key=="Title")
        return General_Title;
    if (Key=="Author")
        return General_Producer;
    if (Key=="Subject")
        return General_Description;
    if (Key=="Keywords")
        return General_Keywords;
    if (Key=="Creator")
        return General_Encoded_Application;
    if (Key=="Producer")
        return General_Encoded_Library;
    if (Key=="CreationDate" || Key=="ModDate")
        return General_Encoded_Date;
    return (size_t)-1;
}

//---------------------------------------------------------------------------
static bool Pdf_Reference(const string& Raw, int32u& ObjectNumber)
{
    //Object number, generation number and "R"
    static const char* const Spaces=" \r\n\t\f";
    const size_t Number_End=Raw.find_first_not_of("0123456789");
    if (!Number_End || Number_End==string::npos)
        return false;
    const size_t Generation=Raw.find_first_not_of(Spaces, Number_End);
    if (Generation==Number_End || Generation==string::npos)
        return false;
    const size_t Generation_End=Raw.find_first_not_of("0123456789", Generation);
    if (Generation_End==Generation || Generation_End==string::npos)
        return false;
    const size_t Keyword=Raw.find_first_not_of(Spaces, Generation_End);
    if (Keyword==Generation_End || Keyword==string::npos || Raw[Keyword]!='R' || Keyword+1!=Raw.size())
        return false;
    ObjectNumber=(int32u)strtoul(Raw.c_str(), NULL, 10);
    return true;
}

//---------------------------------------------------------------------------
void File_Pdf::Object_Info()
{
    Element_Info1("Info");

    //Parsing
    string Key, Raw;
    Ztring Value;
    while (Element_Offset<Element_Size)
    {
        if (Get_Next(Key, Value, &Raw))
        {
            for (;;)
            {
                Get_Next(Key, Value, &Raw);
                if (Key.empty())
                    break;
                if (IsEncrypted || Pdf_Info_Parameter(Key)==(size_t)-1) //The strings of an encrypted file are encrypted
                    continue;
                int32u ObjectNumber;
                if (!Pdf_Reference(Raw, ObjectNumber))
                {
                    Object_Info_Fill(Key, Raw);
                    continue;
                }

                //The value is in another object
                object& Object=Objects[ObjectNumber];
                if (Object.Type==Type_Other && ObjectNumber!=(int32u)-1)
                {
                    Object.Type=Type_InfoValue;
                    Object.TopObject=Objects_Current->first;
                    Objects_Current->second.Bottoms.push_back(ObjectNumber);
                }
                if (Object.Type==Type_InfoValue)
                    Info_Values[ObjectNumber].push_back(Key);
            }

            continue;
        }

        if (Key.empty())
            break;
    }
}

//---------------------------------------------------------------------------
void File_Pdf::Object_Info_Fill(const string &Key, const string &Raw)
{
    const size_t Parameter=Pdf_Info_Parameter(Key);
    const Ztring Text=Pdf_String(Raw);
    if (Parameter!=General_Encoded_Date)
        Fill(Stream_General, 0, Parameter, Text);
    else if (Key=="ModDate")
        Info_ModDate=Pdf_Date(Text);
    else
        Info_CreationDate=Pdf_Date(Text);
}

//---------------------------------------------------------------------------
void File_Pdf::Object_InfoValue(int32u ObjectNumber)
{
    Element_Info1("Info value");

    //Parsing
    while (Element_Offset<Element_Size && Pdf_IsSpace(Buffer[Buffer_Offset+(size_t)Element_Offset]))
        Element_Offset++;
    const int8u* Data=Buffer+Buffer_Offset+(size_t)Element_Offset;
    const size_t Size=(size_t)(Element_Size-Element_Offset);
    size_t End=0;
    if (Size && Data[0]=='(')
        End=Pdf_String_End(Data, Size, 0);
    else if (Size>1 && Data[0]=='<' && Data[1]!='<')
    {
        End=find(Data, Data+Size, '>')-Data;
        if (End<Size)
            End++;
    }
    string Raw;
    Get_String(End, Raw,                                        "Value");

    const std::map<int32u, std::vector<std::string> >::iterator Keys=Info_Values.find(ObjectNumber);
    if (Keys!=Info_Values.end())
        for (size_t Pos=0; Pos<Keys->second.size(); Pos++)
            Object_Info_Fill(Keys->second[Pos], Raw);
}

//---------------------------------------------------------------------------
void File_Pdf::Object_Metadata()
{
    Element_Info1("Metadata");

    //Parsing
    stream_data Stream;
    if (!Get_Stream(Stream))
    {
        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }
    Skip_Stream(Stream);
    if (Stream.IsTruncated)
    {
        Trusted_IsNot("Invalid metadata length");
        return;
    }

    #if defined(MEDIAINFO_XMP_YES)
    if (!Get_Stream_Decoded(Stream))
        return;
    File_Xmp MI;
    Open_Buffer_Init(&MI, Stream.Size);
    Open_Buffer_Continue(&MI, Stream.Data, Stream.Size);
    Open_Buffer_Finalize(&MI);
    Merge_Tags(MI, Tags_Xmp);
    #endif
}

//---------------------------------------------------------------------------
void File_Pdf::Object_ObjStm()
{
    Element_Info1("Object stream");

    //Parsing
    stream_data Stream;
    if (IsEncrypted || !Get_Stream(Stream)) //The content of the object streams of an encrypted file is encrypted
    {
        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }
    Skip_Stream(Stream);
    if (Stream.IsTruncated)
    {
        Trusted_IsNot("Stream is not complete");
        return;
    }
    if (!Get_Stream_Decoded(Stream))
        return;

    //Place of the object in the stream, which begins with the number of each object and its offset from the first object
    int64u First=Stream.Dictionary["First"].To_int64u();
    if (First>Stream.Size)
    {
        Trusted_IsNot("Offset of the first object is after the end of the stream");
        return;
    }
    int64u Count=Stream.Dictionary["N"].To_int64u();
    Loop_CheckValue(Count, First, 3, "N"); //At least 1 digit for each number and 1 space between 2 numbers
    size_t Pos=0;
    const auto Number=[&]() -> int64u
    {
        while (Pos<First && (Stream.Data[Pos]<'0' || Stream.Data[Pos]>'9'))
            Pos++;
        int64u Value=0;
        for (; Pos<First && Stream.Data[Pos]>='0' && Stream.Data[Pos]<='9'; Pos++)
            Value=Value*10+(Stream.Data[Pos]-'0');
        return Value;
    };
    int64u Begin=(int64u)-1;
    for (int64u Remaining=Count; Remaining && Begin==(int64u)-1; Remaining--)
    {
        const int64u ObjectNumber=Number();
        const int64u Offset=Number();
        if (ObjectNumber==Objects_Current->first)
            Begin=Offset;
    }
    if (Begin>=Stream.Size-First)
    {
        Trusted_IsNot("Object is not in the stream");
        return;
    }
    int64u End=Stream.Size-First; //Up to the next object
    Pos=0;
    for (; Count; Count--)
    {
        Number();
        const int64u Offset=Number();
        if (Offset>Begin && Offset<End)
            End=Offset;
    }

    //Parsing the object in the content of the stream
    const int8u* Save_Buffer=Buffer;
    const size_t Save_Buffer_Offset=Buffer_Offset;
    const size_t Save_Buffer_Size=Buffer_Size;
    const int64u Save_Element_Size=Element_Size;
    File_Offset+=Buffer_Offset+Stream.Offset;
    Buffer=Stream.Data;
    Buffer_Offset=0;
    Buffer_Size=(size_t)(First+End); //The bytes after the object are not readable
    Element_Offset=First+Begin;
    Element_Size=First+End;
    switch (Objects_Current->second.Type)
    {
        case Type_Root      : Object_Root(); break;
        case Type_Info      : Object_Info(); break;
        case Type_InfoValue : Object_InfoValue(Objects_Current->first); break;
        default             : ;
    }
    Buffer=Save_Buffer;
    Buffer_Offset=Save_Buffer_Offset;
    Buffer_Size=Save_Buffer_Size;
    Element_Size=Save_Element_Size;
    Element_Offset=Element_Size;
    File_Offset-=Buffer_Offset+Stream.Offset;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
int64u File_Pdf::SizeOfLine()
{
    //while (Element_Offset<Element_Size && (Buffer[Buffer_Offset+(size_t)Element_Offset]=='\r' || Buffer[Buffer_Offset+(size_t)Element_Offset]=='\n' || Buffer[Buffer_Offset+(size_t)Element_Offset]=='<' || Buffer[Buffer_Offset+(size_t)Element_Offset]=='>'))
    while (Element_Offset<Element_Size && (Buffer[Buffer_Offset+(size_t)Element_Offset]=='\r' || Buffer[Buffer_Offset+(size_t)Element_Offset]=='\n' || Buffer[Buffer_Offset+(size_t)Element_Offset]==' '))
        Element_Offset++;
    size_t End=Buffer_Offset+(size_t)Element_Offset;
    while (End<Buffer_Size && Buffer[End]!='\r' && Buffer[End]!='\n' && !(End+1<Buffer_Size && Buffer[End]=='<' && Buffer[End+1]=='<') && !(End+1<Buffer_Size && Buffer[End]=='>' && Buffer[End+1]=='>'))
        End++;
    return End-(Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
int64u File_Pdf::SizeOfObjectHeader()
{
    while (Element_Offset<Element_Size && Pdf_IsSpace(Buffer[Buffer_Offset+(size_t)Element_Offset]))
        Element_Offset++;

    //Object number, generation number and "obj", with spaces or ends of line between them
    const size_t Begin=Buffer_Offset+(size_t)Element_Offset;
    size_t End=Begin;
    for (size_t Number=0; Number<2; Number++)
    {
        const size_t Digits=End;
        while (End<Buffer_Size && Buffer[End]>='0' && Buffer[End]<='9')
            End++;
        const size_t Spaces=End;
        while (End<Buffer_Size && Pdf_IsSpace(Buffer[End]))
            End++;
        if (Spaces==Digits || End==Spaces)
            return 0;
    }
    if (End+3>Buffer_Size || Buffer[End]!='o' || Buffer[End+1]!='b' || Buffer[End+2]!='j' || End+3-Begin>Element_Size-Element_Offset)
        return 0;
    return End+3-Begin;
}

//---------------------------------------------------------------------------
bool File_Pdf::Get_Next(string &Key, Ztring &Value, string* Raw)
{
    Key.clear();
    Value.clear();
    if (Raw)
        Raw->clear();

    string Line;

    //Removig end of lines
    while (Element_Offset<Element_Size && Pdf_IsSpace(Buffer[Buffer_Offset+(size_t)Element_Offset]))
        Element_Offset++;

    //End
    if (Element_Offset>=Element_Size)
        return true;

    //Testing Catalog
    Peek_String (2, Line);
    if (Line=="<<")
    {
        Element_Offset+=2;
        Catalog_Level++;
        return true;
    }
    else if (Line==">>")
    {
        Element_Offset+=2;
        Catalog_Level--;
        return true;
    }

    //Getting a complete line
    Peek_String (SizeOfLine(), Line);

    //Testing Catalog
    size_t Catalog_End=Line.find(">>");
    if (Catalog_End!=String::npos)
        Line.resize(Catalog_End);

    //Testing stream
    if (Line=="stream")
    {
        Skip_String(Line.size(),                                "Stream, Header");
        Key=Line;
        return false;
    }
    if (Line=="endstream")
    {
        Skip_String(Line.size(),                                "Stream, Footer");
        Key=Line;
        return false;
    }

    //Testing object
    if (Line=="endobj")
    {
        Skip_String(Line.size(),                                "Footer");
        Key=Line;
        return false;
    }

    //Testing next key
    const size_t Begin=Buffer_Offset+(size_t)Element_Offset;
    if (Begin>=Buffer_Size || Buffer[Begin]!='/')
        return false;
    const int8u* Data=Buffer+Begin;
    size_t Size=Buffer_Size-Begin;
    if (Size>Element_Size-Element_Offset)
        Size=(size_t)(Element_Size-Element_Offset);

    //Key
    size_t Key_End=1;
    while (Key_End<Size && !Pdf_IsSpace(Data[Key_End]) && !Pdf_IsDelimiter(Data[Key_End]))
        Key_End++;

    //Value, with or without space before it, which can be on several lines
    size_t Value_Begin=Key_End;
    while (Value_Begin<Size && Pdf_IsSpace(Data[Value_Begin]))
        Value_Begin++;
    size_t Value_End=Value_Begin;
    if (Value_Begin<Size)
        switch (Data[Value_Begin])
        {
            case '/' : //Name
                        do
                            Value_End++;
                        while (Value_End<Size && !Pdf_IsSpace(Data[Value_End]) && !Pdf_IsDelimiter(Data[Value_End]));
                        break;
            case '(' : //Literal string
                        Value_End=Pdf_String_End(Data, Size, Value_Begin);
                        break;
            case '[' : //Array
                        {
                        size_t Level=0;
                        do
                        {
                            switch (Data[Value_End])
                            {
                                case '[' : Level++; break;
                                case ']' : Level--; break;
                                case '(' : Value_End=Pdf_String_End(Data, Size, Value_End)-1; break;
                                default  : ;
                            }
                            Value_End++;
                        }
                        while (Level && Value_End<Size);
                        }
                        break;
            case '<' :
                        if (Value_Begin+1<Size && Data[Value_Begin+1]=='<')
                            break; //Dictionary, its content is in the next items

                        //Hexadecimal string
                        while (Value_End<Size && Data[Value_End]!='>')
                            Value_End++;
                        if (Value_End<Size)
                            Value_End++;
                        break;
            default  : //Number, boolean, null or reference to an object, up to the next key or the end of the dictionary
                        while (Value_End<Size && !Pdf_IsDelimiter(Data[Value_End]))
                            Value_End++;
                        while (Value_End>Value_Begin && Pdf_IsSpace(Data[Value_End-1]))
                            Value_End--;
        }
    if (Value_End==Value_Begin)
        Value_Begin=Value_End=Key_End; //No value

    Key.assign((const char*)Data+1, Key_End-1);
    if (Raw)
        Raw->assign((const char*)Data+Value_Begin, Value_End-Value_Begin);
    Value.From_UTF8((const char*)Data+Value_Begin, Value_End-Value_Begin);
    Skip_String(Value_End,                                      "Key-Value");
    return false;
}

//---------------------------------------------------------------------------
bool File_Pdf::Get_Stream(stream_data &Stream)
{
    //Dictionary
    const size_t Catalog_Level_Base=Catalog_Level;
    string Key, Parent;
    Ztring Value;
    for (;;)
    {
        if (Element_Offset>=Element_Size)
        {
            Stream.IsTruncated=true;
            return false;
        }
        if (Get_Next(Key, Value))
            continue;
        if (Key=="stream")
            break;
        if (Key.empty() || Key=="endstream" || Key=="endobj")
        {
            Stream.IsTruncated=Key.empty() && Element_Offset+SizeOfLine()>=Element_Size; //The end of the line may be after the available data
            return false;
        }
        switch (Catalog_Level-Catalog_Level_Base)
        {
            case 1 : Parent=Key; Stream.Dictionary[Key]=Value; break;
            case 2 : Stream.Dictionary[Parent+'/'+Key]=Value; break;
            default: ;
        }
    }

    //Data, up to the end of the stream when its length is in another object or is not valid
    if (Element_Offset<Element_Size && Buffer[Buffer_Offset+(size_t)Element_Offset]=='\r')
        Element_Offset++;
    if (Element_Offset<Element_Size && Buffer[Buffer_Offset+(size_t)Element_Offset]=='\n')
        Element_Offset++;
    Stream.Offset=Element_Offset;
    Stream.Data=Buffer+Buffer_Offset+(size_t)Element_Offset;
    const size_t Size_Max=(size_t)(Element_Size-Element_Offset);
    const Ztring& Length=Stream.Dictionary["Length"];
    if (!Length.empty() && Length.find_first_not_of(__T("0123456789"))==string::npos && Length.To_int64u()<=Size_Max)
        Stream.Size=(size_t)Length.To_int64u();
    else
    {
        static const char EndStream[]="endstream";
        Stream.Size=search(Stream.Data, Stream.Data+Size_Max, EndStream, EndStream+9)-Stream.Data;
        if (Stream.Size==Size_Max)
            Stream.IsTruncated=true;
        else
        {
            //Without the end of line before the keyword
            if (Stream.Size && Stream.Data[Stream.Size-1]=='\n')
                Stream.Size--;
            if (Stream.Size && Stream.Data[Stream.Size-1]=='\r')
                Stream.Size--;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
void File_Pdf::Skip_Stream(const stream_data &Stream)
{
    Skip_XX(Stream.Size,                                        "Stream, Data");

    //End of the stream and of the object
    string Key;
    Ztring Value;
    while (Element_Offset<Element_Size && !Get_Next(Key, Value) && !Key.empty());
}

//---------------------------------------------------------------------------
static const size_t Pdf_Decoded_Max=0x4000000; //Arbitrary limit of 64 MiB

//---------------------------------------------------------------------------
static bool Pdf_Inflate(const int8u* Data, size_t Size, vector<int8u>& Decoded)
{
    z_stream strm{};
    if (inflateInit(&strm)!=Z_OK)
        return false;
    strm.next_in=(Bytef*)Data;
    strm.avail_in=(uInt)Size;
    const size_t Decoded_Max=Pdf_Decoded_Max;
    int Result=Z_OK;
    while (Result==Z_OK && strm.total_out<Decoded_Max)
    {
        const size_t Decoded_Size=(size_t)strm.total_out;
        size_t Decoded_NewSize=Decoded_Size?Decoded_Size*2:(Size<0x400?0x1000:Size*4);
        if (Decoded_NewSize>Decoded_Max)
            Decoded_NewSize=Decoded_Max;
        Decoded.resize(Decoded_NewSize);
        strm.next_out=Decoded.data()+Decoded_Size;
        strm.avail_out=(uInt)(Decoded_NewSize-Decoded_Size);
        Result=inflate(&strm, Z_NO_FLUSH);
        if (strm.avail_out)
            break; //No more data to decode
    }
    Decoded.resize((size_t)strm.total_out);
    inflateEnd(&strm);
    return Result==Z_OK || Result==Z_STREAM_END || Result==Z_BUF_ERROR; //The end of the data may be missing
}

//---------------------------------------------------------------------------
static bool Pdf_Predictor(vector<int8u>& Data, int32u Predictor, int32u Colors, int32u BitsPerComponent, int32u Columns)
{
    if (Predictor<2)
        return true;
    if (!Colors || Colors>32 || !Columns || Columns>0x100000 || (BitsPerComponent!=1 && BitsPerComponent!=2 && BitsPerComponent!=4 && BitsPerComponent!=8 && BitsPerComponent!=16))
        return false;
    const size_t Pixel_Size=(Colors*BitsPerComponent+7)/8;
    const size_t Row_Size=((size_t)Colors*BitsPerComponent*Columns+7)/8;
    if (Predictor==2)
    {
        //TIFF predictor, with components of 1 byte only
        if (BitsPerComponent!=8)
            return false;
        for (size_t Row=0; Row+Row_Size<=Data.size(); Row+=Row_Size)
            for (size_t Pos=Pixel_Size; Pos<Row_Size; Pos++)
                Data[Row+Pos]+=Data[Row+Pos-Pixel_Size];
        return true;
    }
    if (Predictor<10 || Predictor>15)
        return false;

    //PNG predictors, the first byte of each row is the kind of filter of the row
    size_t Out=0;
    for (size_t In=0; In+1+Row_Size<=Data.size(); In+=1+Row_Size, Out+=Row_Size)
    {
        const int8u Filter=Data[In];
        if (Filter>4)
            return false;
        for (size_t Pos=0; Pos<Row_Size; Pos++)
        {
            const int Left=Pos<Pixel_Size?0:Data[Out+Pos-Pixel_Size];
            const int Up=Out?Data[Out+Pos-Row_Size]:0;
            const int UpLeft=(Out && Pos>=Pixel_Size)?Data[Out+Pos-Row_Size-Pixel_Size]:0;
            int Predicted=0;
            switch (Filter)
            {
                case 1 : Predicted=Left; break;
                case 2 : Predicted=Up; break;
                case 3 : Predicted=(Left+Up)/2; break;
                case 4 : {
                         //Paeth
                         const int FromLeft=abs(Up-UpLeft);
                         const int FromUp=abs(Left-UpLeft);
                         const int FromUpLeft=abs(Left+Up-2*UpLeft);
                         Predicted=(FromLeft<=FromUp && FromLeft<=FromUpLeft)?Left:(FromUp<=FromUpLeft?Up:UpLeft);
                         }
                         break;
                default: ;
            }
            Data[Out+Pos]=(int8u)(Data[In+1+Pos]+Predicted);
        }
    }
    Data.resize(Out);
    return true;
}

//---------------------------------------------------------------------------
static bool Pdf_AsciiHex(const int8u* Data, size_t Size, vector<int8u>& Decoded)
{
    Decoded.reserve(Size/2);
    int High=-1;
    for (size_t Pos=0; Pos<Size; Pos++)
    {
        const int8u Value=Data[Pos];
        int Digit;
        if (Value>='0' && Value<='9')
            Digit=Value-'0';
        else if (Value>='A' && Value<='F')
            Digit=Value-'A'+10;
        else if (Value>='a' && Value<='f')
            Digit=Value-'a'+10;
        else if (Value=='>')
            break; //End of data
        else if (Pdf_IsSpace(Value))
            continue;
        else
            return false;
        if (High<0)
            High=Digit;
        else
        {
            Decoded.push_back((int8u)((High<<4)|Digit));
            High=-1;
        }
    }
    if (High>=0)
        Decoded.push_back((int8u)(High<<4)); //A last digit alone is followed by 0
    return true;
}

//---------------------------------------------------------------------------
static bool Pdf_Ascii85(const int8u* Data, size_t Size, vector<int8u>& Decoded)
{
    Decoded.reserve(Size/5*4);
    int64u Group=0;
    size_t Group_Size=0;
    for (size_t Pos=0; Pos<Size; Pos++)
    {
        const int8u Value=Data[Pos];
        if (Value=='~')
            break; //End of data
        if (Pdf_IsSpace(Value))
            continue;
        if (Value=='z' && !Group_Size)
        {
            Decoded.insert(Decoded.end(), 4, 0x00); //4 bytes of value 0
            continue;
        }
        if (Value<'!' || Value>'u')
            return false;
        Group=Group*85+(Value-'!');
        Group_Size++;
        if (Group_Size==5)
        {
            if (Group>0xFFFFFFFF)
                return false;
            for (int Shift=24; Shift>=0; Shift-=8)
                Decoded.push_back((int8u)(Group>>Shift));
            Group=0;
            Group_Size=0;
        }
    }
    if (Group_Size==1)
        return false; //A group has at least 2 characters
    if (Group_Size)
    {
        //Last group, with 1 byte less than its count of characters
        for (size_t Pos=Group_Size; Pos<5; Pos++)
            Group=Group*85+84;
        if (Group>0xFFFFFFFF)
            return false;
        for (size_t Pos=0; Pos+1<Group_Size; Pos++)
            Decoded.push_back((int8u)(Group>>(24-8*Pos)));
    }
    return true;
}

//---------------------------------------------------------------------------
static bool Pdf_RunLength(const int8u* Data, size_t Size, vector<int8u>& Decoded)
{
    for (size_t Pos=0; Pos<Size;)
    {
        const int8u Length=Data[Pos++];
        if (Length==128)
            break; //End of data
        if (Length<128)
        {
            //Copy of the next bytes
            const size_t Count=min((size_t)Length+1, Size-Pos);
            Decoded.insert(Decoded.end(), Data+Pos, Data+Pos+Count);
            Pos+=Count;
        }
        else if (Pos<Size)
            Decoded.insert(Decoded.end(), (size_t)257-Length, Data[Pos++]); //Next byte repeated
        if (Decoded.size()>Pdf_Decoded_Max)
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
static bool Pdf_Lzw(const int8u* Data, size_t Size, int32u EarlyChange, vector<int8u>& Decoded)
{
    //Codes of 9 to 12 bits, most significant bit first: 0-255 = a byte, 256 = table cleared, 257 = end of data, then a previous content with 1 more byte
    vector<int16u> Prefix(4096);
    vector<int8u> Suffix(4096), Content(4096);
    const int32u NoCode=(int32u)-1;
    int32u Next=258, Bits=9, Previous=NoCode;
    int32u Pending=0, Pending_Bits=0;
    if (EarlyChange>1)
        EarlyChange=1;
    for (size_t Pos=0;;)
    {
        while (Pending_Bits<Bits && Pos<Size)
        {
            Pending=(Pending<<8)|Data[Pos++];
            Pending_Bits+=8;
        }
        if (Pending_Bits<Bits)
            break; //The end of the data may be missing
        const int32u Code=(Pending>>(Pending_Bits-Bits))&((1<<Bits)-1);
        Pending_Bits-=Bits;
        if (Code==256)
        {
            Next=258;
            Bits=9;
            Previous=NoCode;
            continue;
        }
        if (Code==257)
            break; //End of data
        if (Code>Next || (Code==Next && Previous==NoCode))
            return false;

        //Content of the code (from its last byte to its first one), a code not in the table yet being the previous content with its first byte again
        size_t Content_Size=0;
        for (int32u Item=Code==Next?Previous:Code;;)
        {
            if (Item<256)
            {
                Content[Content_Size++]=(int8u)Item;
                break;
            }
            Content[Content_Size++]=Suffix[Item];
            Item=Prefix[Item];
        }
        const int8u First=Content[Content_Size-1];
        if (Decoded.size()+Content_Size+1>Pdf_Decoded_Max)
            return false;
        for (size_t Content_Pos=Content_Size; Content_Pos; Content_Pos--)
            Decoded.push_back(Content[Content_Pos-1]);
        if (Code==Next)
            Decoded.push_back(First);

        //New item of the table
        if (Previous!=NoCode && Next<4096)
        {
            Prefix[Next]=(int16u)Previous;
            Suffix[Next]=First;
            Next++;
            if (Next+EarlyChange>=((int32u)1<<Bits) && Bits<12)
                Bits++;
        }
        Previous=Code;
    }
    return true;
}

//---------------------------------------------------------------------------
bool File_Pdf::Get_Stream_Decoded(stream_data &Stream)
{
    //Filters, alone or in an array, in the order of the decoding
    const string Filter=Stream.Dictionary["Filter"].To_UTF8();
    vector<string> Filters;
    for (size_t Pos=Filter.find('/'); Pos!=string::npos;)
    {
        const size_t End=Filter.find_first_of("/] \r\n\t", Pos+1);
        Filters.push_back(Filter.substr(Pos+1, End==string::npos?End:(End-(Pos+1))));
        Pos=End==string::npos?End:Filter.find('/', End);
    }
    if (Filters.empty())
        return Filter.find_first_not_of("[] \r\n\t")==string::npos; //No filter, anything else (e.g. in another object) is not supported
    if (Filters.size()>8)
        return false; //Arbitrary limit

    //Parameters of the filters, in a dictionary or, when the filters are in an array, in an array with 1 item (dictionary or null) per filter
    const string Parameters=Stream.Dictionary["DecodeParms"].To_UTF8();
    vector<string> Parameters_PerFilter;
    size_t Parameters_Pos=Parameters.find_first_not_of(" \r\n\t");
    if (Parameters_Pos!=string::npos && Parameters[Parameters_Pos]=='[')
    {
        for (Parameters_Pos++;;)
        {
            Parameters_Pos=Parameters.find_first_not_of(" \r\n\t", Parameters_Pos);
            if (Parameters_Pos==string::npos || Parameters[Parameters_Pos]==']')
                break;
            size_t End=Parameters_Pos;
            if (!Parameters.compare(Parameters_Pos, 2, "<<"))
            {
                //Dictionary, up to its end
                size_t Level=0;
                do
                {
                    if (!Parameters.compare(End, 2, "<<"))
                    {
                        Level++;
                        End+=2;
                    }
                    else if (!Parameters.compare(End, 2, ">>"))
                    {
                        Level--;
                        End+=2;
                    }
                    else
                        End++;
                }
                while (Level && End<Parameters.size());
            }
            else
            {
                End=Parameters.find_first_of("] \r\n\t<", Parameters_Pos+1); //At least 1 character
                if (End==string::npos)
                    End=Parameters.size();
            }
            Parameters_PerFilter.push_back(Parameters.substr(Parameters_Pos, End-Parameters_Pos));
            Parameters_Pos=End;
        }
    }
    else
        Parameters_PerFilter.push_back(Parameters);
    const auto Parameter=[&](size_t Filter_Pos, const char* Name, int32u Default) -> int32u
    {
        if (!Filter_Pos)
        {
            const auto Item=Stream.Dictionary.find(string("DecodeParms/")+Name);
            if (Item!=Stream.Dictionary.end())
                return Item->second.To_int32u();
        }
        if (Filter_Pos>=Parameters_PerFilter.size())
            return Default;
        const string& Content=Parameters_PerFilter[Filter_Pos];
        size_t Pos=Content.find(string("/")+Name);
        if (Pos!=string::npos)
            Pos=Content.find_first_of("0123456789", Pos);
        return Pos==string::npos?Default:(int32u)strtoul(Content.c_str()+Pos, NULL, 10);
    };

    //Decoding, the content after a filter being the content before the next one
    for (size_t Filter_Pos=0; Filter_Pos<Filters.size(); Filter_Pos++)
    {
        const string& Name=Filters[Filter_Pos];
        vector<int8u> Decoded;
        bool HasPredictor=false;
        bool IsOk;
        if (Name=="FlateDecode" || Name=="Fl")
        {
            IsOk=Pdf_Inflate(Stream.Data, Stream.Size, Decoded);
            HasPredictor=true;
        }
        else if (Name=="LZWDecode" || Name=="LZW")
        {
            IsOk=Pdf_Lzw(Stream.Data, Stream.Size, Parameter(Filter_Pos, "EarlyChange", 1), Decoded);
            HasPredictor=true;
        }
        else if (Name=="ASCIIHexDecode" || Name=="AHx")
            IsOk=Pdf_AsciiHex(Stream.Data, Stream.Size, Decoded);
        else if (Name=="ASCII85Decode" || Name=="A85")
            IsOk=Pdf_Ascii85(Stream.Data, Stream.Size, Decoded);
        else if (Name=="RunLengthDecode" || Name=="RL")
            IsOk=Pdf_RunLength(Stream.Data, Stream.Size, Decoded);
        else
            return false; //Not supported
        if (!IsOk)
        {
            Trusted_IsNot("Problem during the decoding");
            return false;
        }
        if (HasPredictor && !Pdf_Predictor(Decoded, Parameter(Filter_Pos, "Predictor", 1), Parameter(Filter_Pos, "Colors", 1), Parameter(Filter_Pos, "BitsPerComponent", 8), Parameter(Filter_Pos, "Columns", 1)))
            return false; //Not supported
        Stream.Decoded.swap(Decoded);
        Stream.Data=Stream.Decoded.data();
        Stream.Size=Stream.Decoded.size();
    }
    return true;
}

} //NameSpace

#endif //MEDIAINFO_PDF_YES
