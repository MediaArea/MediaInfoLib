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
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__Analyze_Element.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include <cstring>
#include <utility>
#include "ThirdParty/base64/base64.h"

namespace MediaInfoLib
{
#if MEDIAINFO_TRACE
//***************************************************************************
// Element_Node_String
//***************************************************************************

//---------------------------------------------------------------------------
void element_details::Element_Node_String::assign(const char* Data, size_t Size)
{
    item* New=NULL;
    if (Size)
    {
        New=(item*)::operator new(sizeof(item)+Size+1);
        New->Count=1;
        New->Size=Size;
        char* New_Data=(char*)(New+1);
        std::memcpy(New_Data, Data, Size);
        New_Data[Size]='\0';
    }
    clear(); //After the copy, Data may be in the previous string
    Item=New;
}

//***************************************************************************
// Element_Node_Data
//***************************************************************************

//---------------------------------------------------------------------------
element_details::Element_Node_Data& element_details::Element_Node_Data::operator=(const Element_Node_Data& v)
{
    if (this == &v)
        return *this;

    clear();

    type = v.type;
    switch (type)
    {
        case ELEMENT_NODE_STR:
        {
            size_t len = strlen(v.val.Str);
            val.Str = new char[len + 1];
            std::memcpy(val.Str, v.val.Str, len);
            val.Str[len] = '\0';
            break;
        }
        case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT80:
              val.f80 = new float80;
              *val.f80 = *v.val.f80;
              break;
        case ELEMENT_NODE_INT128U:
            val.i128u = new int128u;
            *val.i128u = *v.val.i128u;
            break;
        default:
            val = v.val;
            break;
    }

    format_out = v.format_out;
    Option = v.Option;

    return *this;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const Ztring& v)
{
    clear();

    std::string tmp = v.To_UTF8();
    size_t len = tmp.length();
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, tmp.c_str(), len);
        set_Option(len);
    }
    else
    {
        type = ELEMENT_NODE_STR;
        val.Str = new char[len + 1];
        std::memcpy(val.Str, tmp.c_str(), len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const std::string& v)
{
    clear();

    size_t len =v.length();
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, v.c_str(), len);
        set_Option(len);
    }
    else
    {
        type = ELEMENT_NODE_STR;
        val.Str = new char[len + 1];
        std::memcpy(val.Str, v.c_str(), len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const char* v)
{
    clear();

    if (!v)
        return;

    type = ELEMENT_NODE_STR;
    int len = strlen(v);
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, v, len);
        set_Option(len);
    }
    else
    {
        val.Str = new char[len + 1];
        std::memcpy(val.Str, v, len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(bool v)
{
    clear();

    type = ELEMENT_NODE_BOOL;
    val.b = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8u v)
{
    clear();

    type = ELEMENT_NODE_INT8U;
    val.i8u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8s v)
{
    clear();

    type = ELEMENT_NODE_INT8S;
    val.i8s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16u v)
{
    clear();

    type = ELEMENT_NODE_INT16U;
    val.i16u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16s v)
{
    clear();

    type = ELEMENT_NODE_INT16S;
    val.i16s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32u v)
{
    clear();

    type = ELEMENT_NODE_INT32U;
    val.i32u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32s v)
{
    clear();

    type = ELEMENT_NODE_INT32S;
    val.i32s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64u v)
{
    clear();

    type = ELEMENT_NODE_INT64U;
    val.i64u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64s v)
{
    clear();

    type = ELEMENT_NODE_INT64S;
    val.i64s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int128u v)
{
    clear();

    type = ELEMENT_NODE_INT128U;
    val.i128u = new int128u;
    *val.i128u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float32 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT32;
    val.f32 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float64 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT64;
    val.f64 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float80 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT80;
    val.f80 = new float80;
    *val.f80 = v;
}

//---------------------------------------------------------------------------
bool element_details::Element_Node_Data::operator==(const std::string& v)
{
    if (type == ELEMENT_NODE_CHAR8)
        return v == std::string(val.Chars, Option);
    if (type == ELEMENT_NODE_STR)
        return v == val.Str;

    return false;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::clear()
{
    switch (type)
    {
      case ELEMENT_NODE_STR:
          delete [] val.Str;
          break;
      case ELEMENT_NODE_FLOAT80:
          delete val.f80;
          break;
      case ELEMENT_NODE_INT128U:
          delete val.i128u;
          break;
      default:
          break;
    }

    type = ELEMENT_NODE_NONE;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::swap(Element_Node_Data& v)
{
    std::swap(val, v.val);
    std::swap(type, v.type);
    std::swap(format_out, v.format_out);
    std::swap(Option, v.Option);
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(std::string& val, int8u bits, int8u default_bits)
{
    if (bits == (int8u)-1)
        bits = default_bits;

    std::string zeros;
    bool full = (bits % 4 == 0);
    int nb_zero = (bits / 4) + (full?0:1);
    nb_zero -= val.size();
    if (nb_zero > 0)
        zeros.resize(nb_zero, '0');
    val = zeros + val;
}

//---------------------------------------------------------------------------
static size_t Xml_Name_Escape_MustEscape(const char* Name, size_t Size)
{
    // Case it is empty
    if (!Size)
        return (size_t)-1;

    // Case first char is a digit
    if ((Name[0] >= '0' && Name[0] <= '9'))
        return 0;

    // Cheking all chars
    for (size_t Pos = 0; Pos < Size; Pos++)
    {
        const char C = Name[Pos];
        switch (C)
        {
            case ' ':
            case '/':
            case '(':
            case ')':
            case '*':
            case ',':
            case ':':
            case '@':
                return Pos;
            default:
                if (!(C >= 'A' && C <= 'Z')
                    && !(C >= 'a' && C <= 'z')
                    && !(C >= '0' && C <= '9')
                    && !(C == '_'))
                    return Pos;
        }
     }

    return (size_t)-1;
}

//---------------------------------------------------------------------------
static inline size_t Xml_Name_Escape_MustEscape(const std::string &Name)
{
    return Xml_Name_Escape_MustEscape(Name.c_str(), Name.size());
}

//---------------------------------------------------------------------------
static size_t Xml_Content_Escape_MustEscape(const char* Content, size_t Size)
{
    // Cheking all chars
    for (size_t Pos = 0; Pos < Size; Pos++)
    {
        const unsigned char C = Content[Pos];
        switch (C)
        {
            case '\"':
            case '&':
            case '\'':
            case '<':
            case '>':
                return Pos;
            default:
                if (C<0x20)
                    return Pos;
        }
    }

    return (size_t)-1;
}

//---------------------------------------------------------------------------
static inline size_t Xml_Content_Escape_MustEscape(const std::string &Content)
{
    return Xml_Content_Escape_MustEscape(Content.c_str(), Content.size());
}

//---------------------------------------------------------------------------
static void Xml_Content_Escape(const char* Content, size_t Size, std::string& ToReturn, size_t Pos)
{
    if (Size)
        ToReturn = std::string(Content, Size);
    else
    {
        ToReturn.clear();
        return;
    }

    size_t Content_Size = Size;

    for (; Pos<Size; Pos++)
    {
        const char C = ToReturn[Pos];
        switch (C)
        {
            case '\"':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "quot;");
                Pos += 5;
                Size += 5;
                break;
            case '&':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "amp;");
                Pos += 4;
                Size += 4;
                break;
            case '\'':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "apos;");
                Pos += 5;
                Size += 5;
                break;
            case '<':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "lt;");
                Pos += 3;
                Size += 3;
                break;
            case '>':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "gt;");
                Pos += 3;
                Size += 3;
                break;
            case '\n':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "#xA;");
                Pos += 4;
                Size += 4;
                break;
            case '\r': //Doing like XML rule "(...) translating both the two-character sequence #xD #xA and any #xD that is not followed by #xA to a single #xA character".
                ToReturn[Pos] = '&';
                if (Pos + 1 < Size && ToReturn[Pos + 1] == '\n')
                {
                    Pos++;
                    ToReturn[Pos] = '#';
                    ToReturn.insert(Pos + 1, "xA;");
                    Pos += 3;
                    Size += 3;
                    break;
                }
                ToReturn.insert(Pos + 1, "#xA;");
                Pos += 4;
                Size += 4;
                break;
            default:
                if (C<0x20)
                {
                    ToReturn = Base64::encode(std::string(Content, Content_Size));
                    return; //End
                }
        }
    }
}

//---------------------------------------------------------------------------
static inline void Xml_Content_Escape(const std::string &Content, std::string& ToReturn, size_t Pos)
{
    Xml_Content_Escape(Content.c_str(), Content.size(), ToReturn, Pos);
}

//---------------------------------------------------------------------------
// Output helpers, the output is appended to a std::string
//---------------------------------------------------------------------------
static void Append_Dec(std::string& Out, int64u Value)
{
    char Temp[20];
    char* const End=Temp+20;
    char* Pos=End;
    do
    {
        *--Pos=(char)('0'+Value%10);
        Value/=10;
    }
    while (Value);
    Out.append(Pos, End-Pos);
}

//---------------------------------------------------------------------------
static void Append_Dec(std::string& Out, int64s Value)
{
    if (Value<0)
    {
        Out+='-';
        Append_Dec(Out, ((int64u)0)-(int64u)Value);
    }
    else
        Append_Dec(Out, (int64u)Value);
}

//---------------------------------------------------------------------------
// Uppercase, with zeros on the left up to Digits_Min digits
static void Append_Hex(std::string& Out, int64u Value, size_t Digits_Min)
{
    static const char Digits[]="0123456789ABCDEF";
    char Temp[16];
    char* const End=Temp+16;
    char* Pos=End;
    do
    {
        *--Pos=Digits[Value&0xF];
        Value>>=4;
    }
    while (Value);
    size_t Size=End-Pos;
    if (Size<Digits_Min)
        Out.append(Digits_Min-Size, '0');
    Out.append(Pos, Size);
}

//---------------------------------------------------------------------------
// Hexadecimal value of an integer in tree format, Bits is the count of valid bits
static void Append_Integer_Hex(std::string& Out, int64u Value, int8u Bits, int8u Bits_Default)
{
    if (Bits==(int8u)-1)
        Bits=Bits_Default;
    Out+=" (0x";
    Append_Hex(Out, Value, Bits/4+((Bits%4)?1:0));
    Out+=')';
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::Append(std::string& Out) const
{
    switch (type)
    {
      case ELEMENT_NODE_CHAR8:
      {
          if (format_out==Format_Tree)
          {
              for (int8u i=0; i<Option; ++i)
                  Out+=val.Chars[i]?val.Chars[i]:' '; //Text output does not like NULL chars, replacing it by space
              break;
          }

          size_t MustEscape=Xml_Content_Escape_MustEscape(val.Chars, Option);
          if (MustEscape!=(size_t)-1)
          {
              std::string str;
              Xml_Content_Escape(val.Chars, Option, str, MustEscape);
              Out+=str;
          }
          else
              Out.append(val.Chars, Option);
          break;
      }
      case ELEMENT_NODE_STR:
      {
          if (format_out==Format_Tree)
          {
              const char* a=val.Str;
              for (;;)
              {
                  const char* b=strchr(a, '\n');
                  if (!b)
                      break;
                  size_t c=b-a;
                  if (c && a[c-1]=='\r')
                      c--;
                  Out.append(a, c);
                  a=b+1;
                  if (*a) // If it is not the last character
                      Out+=" / ";
              }
              Out+=a;
              break;
          }

          size_t Size=strlen(val.Str);
          size_t MustEscape=Xml_Content_Escape_MustEscape(val.Str, Size);
          if (MustEscape!=(size_t)-1)
          {
              std::string str;
              Xml_Content_Escape(val.Str, Size, str, MustEscape);
              Out+=str;
          }
          else
              Out.append(val.Str, Size);
          break;
      }
      case ELEMENT_NODE_BOOL:
          Out+=val.b?"Yes":"No";
          break;
      // The hexadecimal value of signed integers is the one of their type when they were displayed with a stream (int for 8-bit and 16-bit)
      case ELEMENT_NODE_INT8U:
          Append_Dec(Out, (int64u)val.i8u);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, val.i8u, Option, 8);
          break;
      case ELEMENT_NODE_INT8S:
          Append_Dec(Out, (int64s)val.i8s);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, (int32u)(int32s)val.i8s, Option, 8);
          break;
      case ELEMENT_NODE_INT16U:
          Append_Dec(Out, (int64u)val.i16u);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, val.i16u, Option, 16);
          break;
      case ELEMENT_NODE_INT16S:
          Append_Dec(Out, (int64s)val.i16s);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, (int32u)(int32s)val.i16s, Option, 16);
          break;
      case ELEMENT_NODE_INT32U:
          Append_Dec(Out, (int64u)val.i32u);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, val.i32u, Option, 32);
          break;
      case ELEMENT_NODE_INT32S:
          Append_Dec(Out, (int64s)val.i32s);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, (int32u)val.i32s, Option, 32);
          break;
      case ELEMENT_NODE_INT64U:
          Append_Dec(Out, val.i64u);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, val.i64u, Option, 64);
          break;
      case ELEMENT_NODE_INT64S:
          Append_Dec(Out, val.i64s);
          if (format_out==Format_Tree)
              Append_Integer_Hex(Out, (int64u)val.i64s, Option, 64);
          break;
      case ELEMENT_NODE_INT128U:
          Out+=uint128toString(*val.i128u, 10);
          if (format_out==Format_Tree)
          {
              std::string str=uint128toString(*val.i128u, 16);
              get_hexa_from_deci_limited_by_bits(str, Option, 128);
              Out+=" (0x";
              Out+=str;
              Out+=')';
          }
          break;
      case ELEMENT_NODE_FLOAT32:
          Out+=Ztring::ToZtring(val.f32, Option==(int8u)-1?3:Option).To_UTF8();
          break;
      case ELEMENT_NODE_FLOAT64:
          Out+=Ztring::ToZtring(val.f64, Option==(int8u)-1?3:Option).To_UTF8();
          break;
      case ELEMENT_NODE_FLOAT80:
          Out+=Ztring::ToZtring(*val.f80, Option==(int8u)-1?3:Option).To_UTF8();
          break;
      default:
          break;
    }
}

//***************************************************************************
// Element_Node_Info
//***************************************************************************
//---------------------------------------------------------------------------
static void Append_Info(std::string& Out, const element_details::Element_Node_Info& v)
{
    v.data.Append(Out);
    if (!v.Measure.empty())
        Out.append(v.Measure.data(), v.Measure.size());
}

//---------------------------------------------------------------------------
static void Append_Xml_Content(std::string& Out, const char* Content, size_t Content_Size)
{
    size_t MustEscape=Xml_Content_Escape_MustEscape(Content, Content_Size);
    if (MustEscape!=(size_t)-1)
    {
        std::string str;
        Xml_Content_Escape(Content, Content_Size, str, MustEscape);
        Out+=str;
    }
    else
        Out.append(Content, Content_Size);
}

//***************************************************************************
// Element_Node
//***************************************************************************
//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node()
: Pos(0), Size(0),
  Current_Child(-1), NoShow(false), OwnChildren(true), IsCat(false), HasError(false), RemoveIfNoErrors(false)
{
}

//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node(const Element_Node& node)
{
    if (this == &node)
        return;

    Pos = node.Pos;
    Size = node.Size;
    Name = node.Name;
    Value = node.Value;
    Infos = node.Infos;
    Children = node.Children;
    Current_Child = node.Current_Child;
    NoShow = node.NoShow;
    OwnChildren = node.OwnChildren;
    IsCat = node.IsCat;
    HasError = node.HasError;
    RemoveIfNoErrors = node.RemoveIfNoErrors;
}

//---------------------------------------------------------------------------
void element_details::Element_Node::TakeChilrenFrom(Element_Node& node)
{
    if (this == &node || !OwnChildren || !node.OwnChildren)
        return;

    for (size_t i = 0; i < node.Children.size(); ++i)
        Children.push_back(node.Children[i]);
    node.Children.clear();
}

//---------------------------------------------------------------------------
element_details::Element_Node::~Element_Node()
{
    if (!OwnChildren)
        return;

    for (size_t i = 0; i < Children.size(); ++i)
        delete Children[i];
    Children.clear();
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Init()
{
    Pos = 0;
    Size = 0;
    Name.clear();
    Value.clear();
    if (Children.size() && OwnChildren)
        for (size_t i = 0; i < Children.size(); ++i)
            delete Children[i];
    Children.clear();
    Infos.clear();
    Current_Child = -1;
    NoShow = false;
    OwnChildren = true;
    IsCat = false;
    HasError = false;
    RemoveIfNoErrors = false;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Micro_Xml(print_struc& s)
{
    if (NoShow)
        return 0;

    std::string& Out=s.Out;
    if (IsCat || Name_Is_Empty())
        goto print_children;

    Out+=Value.empty()?"<b":"<d";
    Out+=" o=\"";
    Append_Dec(Out, Pos);
    Out+="\" n=\"";
    Append_Xml_Content(Out, Name.data(), Name.size());
    Out+='\"';

    for (size_t i = 0, info_nb = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = &Infos[i];

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
            {
                Out+=" parser=\"";
                Info->data.Append(Out);
                Out+='\"';
            }
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
            {
                Out+=" e=\"";
                Info->data.Append(Out);
                Out+='\"';
            }
            continue;
        }

        info_nb++;
        Out+=" i";
        if (info_nb > 1)
            Append_Dec(Out, (int64u)info_nb);
        Out+="=\"";
        Append_Info(Out, *Info);
        Out+='\"';
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        Out+='>';
        Value.Append(Out);
        Out+="</d>";
    }
    else
    {
        Out+=" s=\"";
        Append_Dec(Out, Size);
        Out+="\">";
    }

    s.level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Micro_Xml(s);

    if (!IsCat && !Name_Is_Empty())
    {
        s.level -= 4;

        //end tag
        if (Value.empty())
        {
            //block
            Out+="</b>";
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Xml(print_struc& s)
{
    if (NoShow)
        return 0;

    std::string& Out=s.Out;
    const size_t Level=s.level;

    if (IsCat || Name_Is_Empty())
        goto print_children;

    Out.append(Level, ' ');
    Out+=Value.empty()?"<block":"<data";
    Out+=" offset=\"";
    Append_Dec(Out, Pos);
    Out+="\" name=\"";
    Append_Xml_Content(Out, Name.data(), Name.size());
    Out+='\"';

    for (size_t i = 0, info_nb = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = &Infos[i];

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
            {
                Out+=" parser=\"";
                Info->data.Append(Out);
                Out+='\"';
            }
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
            {
                Out+=" error=\"";
                Info->data.Append(Out);
                Out+='\"';
            }
            continue;
        }

        info_nb++;
        Out+=" info";
        if (info_nb > 1)
            Append_Dec(Out, (int64u)info_nb);
        Out+="=\"";
        Append_Info(Out, *Info);
        Out+='\"';
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        Out+='>';
        Value.Append(Out);
        Out+="</data>";
    }
    else
    {
        Out+=" size=\"";
        Append_Dec(Out, Size);
        Out+="\">";
    }

    Out+=s.eol;

    s.level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Xml(s);

    if (!IsCat && !Name_Is_Empty())
    {
        s.level -= 4;

        //end tag
        if (Value.empty())
        {
            //block
            Out.append(Level, ' ');
            Out+="</block>";
            Out+=s.eol;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree_Cat(print_struc& s)
{
    std::string& Out=s.Out;

    // 3 lines: minuses, "---   Name   ---", minuses
    for (int Line = 0; Line < 3; ++Line)
    {
        Append_Hex(Out, Pos, s.offset_size);
        Out.append(s.level, ' ');
        if (Line == 1)
        {
            Out+="---   ";
            Out.append(Name.data(), Name.size());
            Out+="   ---";
        }
        else
            Out.append(Name.size()+12, '-');
        Out+=s.eol;
    }
    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree(print_struc& s)
{
    if (NoShow)
        return 0;

    if (IsCat)
        return Print_Tree_Cat(s);
    else if (Name_Is_Empty())
        goto print_children;

    {
    std::string& Out=s.Out;
    Append_Hex(Out, Pos, s.offset_size);
    Out.append(s.level, ' ');
    Out.append(Name.data(), Name.size());

#define NB_SPACES 40
    if (!Value.empty())
    {
        Out+=':';
        int nb_free = NB_SPACES - (int)s.level - (int)Name.size(); // 40 - len(Name) - len(spaces)
        Out.append(nb_free > 0 ? nb_free : 1, ' ');
        Value.Set_Output_Format(Element_Node_Data::Format_Tree);
        Value.Append(Out);
    }
#undef NB_SPACES

    for (size_t i = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = &Infos[i];

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
            {
                Out+=" - Parser=";
                Info->data.Append(Out);
            }
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
            {
                Out+=" - Error=";
                Info->data.Append(Out);
            }
            continue;
        }

        Info->data.Set_Output_Format(Element_Node_Data::Format_Tree);
        Out+=" - ";
        Append_Info(Out, *Info);
    }

    if (Value.empty())
    {
        Out+=" (";
        Append_Dec(Out, Size);
        Out+=" bytes)";
    }

    Out+=s.eol;
    }

    s.level++;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Tree(s);

    if (!Name_Is_Empty())
        s.level--;

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print(MediaInfo_Config::trace_Format Format, std::string& Str, const string& eol, int64u File_Size)
{
    //Computing how many characters are needed for displaying maximum file size
    size_t offset_size = sizeof(File_Size)*8-1;
    while (offset_size>1 && (((int64u)1) << offset_size) - 1 >= File_Size)
        offset_size--;
    offset_size++;
    offset_size = (offset_size / 4) + ((offset_size % 4) ? 1 : 0); //4 bits per offset char

    Str.clear();
    int ret = -1;
    print_struc s(Str, eol, offset_size);
    switch (Format)
    {
        case MediaInfo_Config::Trace_Format_Tree:
            s.level = 1;
            ret = Print_Tree(s);
        break;
        case MediaInfo_Config::Trace_Format_CSV:
            break;
        case MediaInfo_Config::Trace_Format_XML:
            ret = Print_Xml(s);
            break;
        case MediaInfo_Config::Trace_Format_MICRO_XML:
            ret = Print_Micro_Xml(s);
            break;
        default:
            break;
    }
    return ret;
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Add_Child(Element_Node* node)
{
    if (node->HasError)
    {
        HasError = node->HasError;
        NoShow = false;
    }

    if (RemoveIfNoErrors && !node->HasError)
    {
        if (!HasError)
            NoShow = true;
        return;
    }

    //Content is moved instead of copied, node is only reset after this call
    Element_Node *new_node = new Element_Node;
    new_node->Pos = node->Pos;
    new_node->Size = node->Size;
    new_node->Name.swap(node->Name);
    new_node->Value.swap(node->Value);
    new_node->Infos.swap(node->Infos);
    new_node->Children.swap(node->Children);
    new_node->Current_Child = node->Current_Child;
    new_node->NoShow = node->NoShow;
    new_node->OwnChildren = node->OwnChildren;
    new_node->IsCat = node->IsCat;
    new_node->HasError = node->HasError;
    new_node->RemoveIfNoErrors = node->RemoveIfNoErrors;
    node->OwnChildren = false;
    Children.push_back(new_node);
}

#endif

}
