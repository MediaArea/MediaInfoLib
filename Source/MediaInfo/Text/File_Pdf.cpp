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
    Info_IsParsed=false;
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
        case State_Parsing_xref         : xref(); if (!Element_IsWaitingForMoreData()) trailer(); break;
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
    offsets::iterator Offsets_Next=upper_bound(Offsets.begin(), Offsets.end(), (int32u)(File_Offset+Buffer_Offset));
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
    Get_String(SizeOfLine(), Line,                              "Header");

    size_t Space_Pos=Line.find(' ');
    int32u ObjectNumber=Ztring().From_UTF8(Line.substr(0, Space_Pos)).To_int32u();
    Element_Info1(ObjectNumber);
    objects::iterator Object=Objects.find(ObjectNumber);
    if (Object==Objects.end())
        Skip_XX(Element_Size-Element_Offset,                    "Data");
    else
        switch(Object->second.Type)
        {
            case Type_Root      : Object_Root(); break;
            case Type_Info      : Object_Info(); break;
            case Type_Metadata  : Object_Metadata(); break;
            default             : Skip_XX(Element_Size-Element_Offset, "Data");
        }

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
         GoTo(Objects_Current->second.Offset);
         break;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pdf::xref()
{
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
        return;
    }
    Element_Begin1("Cross-Reference SubSection");
        Get_String(SizeOfLine(), FirstLine,                         "Header");
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
                const int32u Offset=(int32u)atoi(atoi_buf);
                if (Objects[Base+Pos].Offset==(int32u)-1) //The sections are parsed from the last update of the file to the first one, the last content of an object is the one to parse
                    Objects[Base+Pos].Offset=Offset;
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

    Element_End0();

    Element_End0();

    if (File_Offset+Buffer_Offset>Offsets_Max)
        Offsets_Max=(int32u)(File_Offset+Buffer_Offset);
}

//---------------------------------------------------------------------------
void File_Pdf::trailer()
{
    Element_Begin1("Trailer");

    //Parsing
    int32u Prev=(int32u)-1;
    string Key;
    Ztring Value;
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
                    int32u ObjectNumber=Value.To_int32u();
                    Objects[ObjectNumber].Type=Type_Root;
                    Objects[ObjectNumber].TopObject=(int32u)-1;
                    Objects[(int32u)-1].Bottoms.push_back(ObjectNumber);
                    Param_Info1(__T("Document Catalog is at offset 0x"+Ztring().From_Number(Objects[ObjectNumber].Offset, 16)));
                }
                else if (Key=="Info")
                {
                    int32u ObjectNumber=Value.To_int32u();
                    Objects[ObjectNumber].Type=Type_Info;
                    Objects[ObjectNumber].TopObject=(int32u)-1;
                    Objects[(int32u)-1].Bottoms.push_back(ObjectNumber);
                    Param_Info1(__T("Info is at offset 0x"+Ztring().From_Number(Objects[ObjectNumber].Offset, 16)));
                }
                else if (Key=="Encrypt")
                    IsEncrypted=true;
                else if (Key=="Prev")
                {
                    Prev=Value.To_int32u();
                    Param_Info1(__T("Previous Cross-Reference Table is at offset 0x"+Ztring().From_Number(Prev, 16)));
                }
            }

            continue;
        }

        if (Key.empty())
            break;
    }

    if (Prev!=(int32u)-1 && find(xref_Offsets.begin(), xref_Offsets.end(), Prev)!=xref_Offsets.end())
    {
        Trusted_IsNot("Cross-reference section already parsed");
        Prev=(int32u)-1;
    }

    Element_End0();

    //Previous Cross-Reference Table
    if (Prev!=(int32u)-1)
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
    Objects[(int32u)-1].BottomPos=0;
    Objects_Current=Objects.find(Object_Top->second.Bottoms[0]);
    GoTo(Objects_Current->second.Offset);
    State=State_Parsing_object;
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
    int32u xref_Offset=atoi(xrefOffsetS.c_str());

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
void File_Pdf::Object_Info()
{
    Element_Info1("Info");

    //Parsing
    const bool IsUsed=!IsEncrypted && !Info_IsParsed; //The strings of an encrypted file are encrypted, and the first dictionary parsed is the one of the last update of the file
    Info_IsParsed=true;
    string Key, Raw;
    Ztring Value, CreationDate, ModDate;
    while (Element_Offset<Element_Size)
    {
        if (Get_Next(Key, Value, &Raw))
        {
            for (;;)
            {
                Get_Next(Key, Value, &Raw);
                if (Key.empty())
                    break;
                if (!IsUsed)
                    continue;
                const Ztring Text=Pdf_String(Raw);
                     if (Key=="Title")
                    Fill(Stream_General, 0, General_Title, Text);
                else if (Key=="Author")
                    Fill(Stream_General, 0, General_Producer, Text);
                else if (Key=="Subject")
                    Fill(Stream_General, 0, General_Description, Text);
                else if (Key=="Keywords")
                    Fill(Stream_General, 0, General_Keywords, Text);
                else if (Key=="Creator")
                    Fill(Stream_General, 0, General_Encoded_Application, Text);
                else if (Key=="Producer")
                    Fill(Stream_General, 0, General_Encoded_Library, Text);
                else if (Key=="CreationDate")
                    CreationDate=Pdf_Date(Text);
                else if (Key=="ModDate")
                    ModDate=Pdf_Date(Text);
            }

            continue;
        }

        if (Key.empty())
            break;
    }

    //Same parameters as the ones filled from the XMP metadata
    Fill(Stream_General, 0, General_Encoded_Date, ModDate.empty()?CreationDate:ModDate);
}

//---------------------------------------------------------------------------
void File_Pdf::Object_Metadata()
{
    Element_Info1("Metadata");

    //Parsing
    string Key;
    Ztring Value;
    int32u Length=0;
    while (Element_Offset<Element_Size)
    {
        if (Get_Next(Key, Value))
        {
            for (;;)
            {
                Get_Next(Key, Value);
                if (Key.empty())
                    break;
                else if (Key=="Length")
                {
                    Length=Value.To_int32u();
                }
            }

            continue;
        }

        if (Key.empty())
            break;
        else if (Key=="stream")
        {
            if (Buffer_Offset>Buffer_Size || Element_Offset>Buffer_Size-Buffer_Offset)
            {
                Trusted_IsNot("Invalid metadata offset");
                return;
            }

            //Removig end of lines
            if (Element_Offset<Element_Size && Element_Offset<Buffer_Size-Buffer_Offset && Buffer[Buffer_Offset+(size_t)Element_Offset]=='\r')
                Element_Offset++;
            if (Element_Offset<Element_Size && Element_Offset<Buffer_Size-Buffer_Offset && Buffer[Buffer_Offset+(size_t)Element_Offset]=='\n')
                Element_Offset++;

            if (Element_Offset>Element_Size || Length>Element_Size-Element_Offset
             || Length>Buffer_Size-Buffer_Offset-Element_Offset)
            {
                Trusted_IsNot("Invalid metadata length");
                return;
            }

            #if defined(MEDIAINFO_XMP_YES)
            File_Xmp MI;
            Open_Buffer_Init(&MI, Length);
            Open_Buffer_Continue(&MI, Buffer+Buffer_Offset+(size_t)Element_Offset, Length);
            Open_Buffer_Finalize(&MI);
            Merge_Tags(MI, Tags_Xmp);
            #endif
            Skip_XX(Length,                                     "Stream, Data");
        }
    }
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

} //NameSpace

#endif //MEDIAINFO_PDF_YES
