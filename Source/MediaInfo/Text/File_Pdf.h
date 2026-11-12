/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PDF files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_PdfH
#define MediaInfo_File_PdfH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Sami
//***************************************************************************

class File_Pdf : public File__Analyze
{
public :
    File_Pdf();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Buffer - Per element
    bool Header_Begin();
    void Header_Parse();
    void Data_Parse();

    //Elements
    void obj();
    void xref();
    void xref_Stream();
    void xref_End(int32u Prev);
    void trailer();
    void startxref();
    void Object_Root();
    void Object_Info();
    void Object_Info_Fill(const string &Key, const string &Raw);
    void Object_InfoValue(int32u ObjectNumber);
    void Object_Metadata();
    void Object_ObjStm();
    void Objects_Next();

    //Helpers
    int64u SizeOfLine();
    int64u SizeOfObjectHeader(); //0 if it is not the beginning of an object
    bool   Get_Next(string &Key, Ztring &Value, string* Raw=NULL); //Returns true if it is an array
    struct stream_data
    {
        std::map<std::string, Ztring> Dictionary; //The keys of a dictionary in the dictionary have the key of this dictionary and a solidus before them
        const int8u* Data; //In the buffer, or in Decoded when there is a filter
        size_t Size;
        int64u Offset; //Of the data in the element
        bool IsTruncated;
        std::vector<int8u> Decoded;

        stream_data()
        :
            Data(NULL),
            Size(0),
            Offset(0),
            IsTruncated(false)
        {
        }
    };
    bool   Get_Stream(stream_data &Stream); //Dictionary and place of the data, returns false if the object is not a stream
    bool   Get_Stream_Decoded(stream_data &Stream); //Data after its filter, returns false if it can not be decoded
    void   Skip_Stream(const stream_data &Stream);

    //Temp
    enum state
    {
        State_ParsingElements,
        State_Parsing_xref,
        State_Parsing_startxref,
        State_Parsing_object,
    };
    state State;
    bool IsEncrypted;
    bool Info_IsParsed;
    Ztring Info_CreationDate;
    Ztring Info_ModDate;
    std::map<int32u, std::vector<std::string> > Info_Values; //Keys of the document information whose value is the object
    //typedef std::map<int32u, int32u> offsets;
    //offsets Offsets; //Key is offset
    //offsets::iterator Offsets_Current;
    typedef std::vector<int32u> offsets;
    offsets Offsets;
    offsets xref_Offsets; //Of the cross-reference sections already parsed
    int32u Offsets_Max;
    enum type
    {
        Type_Root,
        Type_Info,
        Type_Metadata,
        Type_InfoValue,
        Type_Other,
    };
    struct object
    {
        int32u Offset;
        int32u ObjectStream; //Object stream the object is in, if any
        type Type;
        int32u TopObject;
        size_t BottomPos;
        std::vector<int32u> Bottoms;

        object()
        :
            Offset((int32u)-1),
            ObjectStream((int32u)-1),
            #ifdef __BORLANDC__
            Type(type::Type_Other),
            #else
            Type(Type_Other),
            #endif
            TopObject((int32u)-1),
            BottomPos((size_t)-1)
        {
        }

        bool IsSet() const
        {
            return Offset!=(int32u)-1 || ObjectStream!=(int32u)-1;
        }
    };
    typedef std::map<int32u, object> objects;
    objects Objects; //Key is object number
    objects::iterator Objects_Current;

    size_t Catalog_Level;
};

} //NameSpace

#endif
