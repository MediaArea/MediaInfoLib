/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__Analyze_Element.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include <iostream>
#include <iomanip>

namespace MediaInfoLib
{
#if MEDIAINFO_TRACE
//***************************************************************************
// Element_Node_Data
//***************************************************************************

//---------------------------------------------------------------------------
element_details::Element_Node_Data& element_details::Element_Node_Data::operator=(const Element_Node_Data& v)
{
    if (this == &v)
        return *this;

    is_empty = v.is_empty;
    type = v.type;
    if (!is_empty)
    {
        switch (type)
        {
          case element_details::Element_Node_Data::ELEMENT_NODE_STR:
          {
              size_t len = strlen(v.val.Str);
              val.Str = new char[len];
              std::memcpy(val.Str, v.val.Str, len);
              val.Str[len] = '\0';
              break;
          }
          case element_details::Element_Node_Data::ELEMENT_NODE_INT128U:
              val.i128u = new int128u;
              *val.i128u = *v.val.i128u;
              break;
          default:
              val = v.val;
              break;
        }
    }
    return *this;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const Ztring& v)
{
    std::string tmp = v.To_UTF8();
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_STR;
    val.Str = new char[tmp.length() + 1];
    std::memcpy(val.Str, tmp.c_str(), tmp.length());
    val.Str[tmp.length()] = '\0';
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const std::string& v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_STR;
    val.Str = new char[v.length() + 1];
    std::memcpy(val.Str, v.c_str(), v.length());
    val.Str[v.length()] = '\0';
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const char* v)
{
    if (!v)
        return;

    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_STR;
    int len = strlen(v);
    val.Str = new char[len + 1];
    std::memcpy(val.Str, v, len);
    val.Str[len] = '\0';
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(bool v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_BOOL;
    val.b = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8u v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT8U;
    val.i8u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8s v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT8S;
    val.i8s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16u v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT16U;
    val.i16u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16s v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT16S;
    val.i16s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32u v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT32U;
    val.i32u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32s v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT32S;
    val.i32s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64u v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT64U;
    val.i64u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64s v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT64S;
    val.i64s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int128u v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_INT128U;
    val.i128u = new int128u;
    *val.i128u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float32 v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_FLOAT32;
    val.f32 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float64 v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_FLOAT64;
    val.f64 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float80 v)
{
    is_empty = false;
    type = element_details::Element_Node_Data::ELEMENT_NODE_FLOAT80;
    val.f80 = v;
}

//---------------------------------------------------------------------------
bool element_details::Element_Node_Data::operator==(const std::string& v)
{
    if (type != element_details::Element_Node_Data::ELEMENT_NODE_STR)
        return false;
    return v == val.Str;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::clear()
{
    if (is_empty)
        return;

    switch (type)
    {
      case element_details::Element_Node_Data::ELEMENT_NODE_STR:
          if (!is_empty)
              delete [] val.Str;
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT128U:
          if (!is_empty)
              delete val.i128u;
          break;
      default:
          break;
    }
    is_empty = true;
}

//---------------------------------------------------------------------------
bool element_details::Element_Node_Data::empty()
{
    return is_empty;
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
std::ostream& operator<<(std::ostream& os, const element_details::Element_Node_Data& v)
{
    switch (v.type)
    {
      case element_details::Element_Node_Data::ELEMENT_NODE_STR:
      {
          Ztring str = Ztring().From_UTF8(v.val.Str);
          size_t Modified = 0;
          MediaInfo_Internal::Xml_Content_Escape_Modifying(str, Modified);
          os << str.To_UTF8();
          break;
      }
      case element_details::Element_Node_Data::ELEMENT_NODE_BOOL:
          if (v.val.b)
              os << "Yes";
          else
              os << "No";
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT8U:
          os << Ztring::ToZtring(v.val.i8u).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring().From_CC1(v.val.i8u).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 8);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT8S:
          os << Ztring::ToZtring(v.val.i8s).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring().From_CC1(v.val.i8s).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 8);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT16U:
          os << Ztring::ToZtring(v.val.i16u).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring().From_CC2(v.val.i16u).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 16);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT16S:
          os << Ztring::ToZtring(v.val.i16s).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring().From_CC2(v.val.i16s).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 16);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT32U:
          os << Ztring::ToZtring(v.val.i32u).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring::ToZtring(v.val.i32u, 16).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 32);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT32S:
          os << Ztring::ToZtring(v.val.i32s).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring::ToZtring(v.val.i32s, 16).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 32);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT64U:
          os << Ztring::ToZtring(v.val.i64u).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring::ToZtring(v.val.i64u, 16).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 64);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT64S:
          os << Ztring::ToZtring(v.val.i64s).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring::ToZtring(v.val.i64s, 16).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 64);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT128U:
          os << Ztring::ToZtring(*v.val.i128u).To_UTF8();
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = Ztring::ToZtring(*v.val.i128u, 16).To_UTF8();
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 128);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT32:
          os << Ztring::ToZtring(v.val.f32, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT64:
          os << Ztring::ToZtring(v.val.f64, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT80:
          os << Ztring::ToZtring(v.val.f80, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
    }
    return os;
}

//***************************************************************************
// Element_Node_Info
//***************************************************************************
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, element_details::Element_Node_Info* v)
{
    if (!v)
        return os;

    os << v->data;
    if (!v->Measure.empty())
        os << v->Measure;

    return os;
}

//***************************************************************************
// Element_Node
//***************************************************************************
//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node()
: Pos(0), Size(0), Header_Size(0),
  Current_Child(-1), NoShow(false), OwnChildren(true), IsCat(false)
{
}
 
//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node(const Element_Node& node)
{
    if (this == &node)
        return;

    Pos = node.Pos;
    Size = node.Size;
    Header_Size = node.Header_Size;
    Name = node.Name;
    Value = node.Value;
    Infos = node.Infos;
    Children = node.Children;
    Parser = node.Parser;
    Current_Child = node.Current_Child;
    NoShow = node.NoShow;
    OwnChildren = node.OwnChildren;
    IsCat = node.IsCat;
}

//---------------------------------------------------------------------------
element_details::Element_Node::~Element_Node()
{
    if (!OwnChildren)
        return;

    for (size_t i = 0; i < Children.size(); ++i)
        delete Children[i];
    Children.clear();

    for (size_t i = 0; i < Infos.size(); ++i)
        delete Infos[i];
    Infos.clear();
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Init()
{
    Pos = 0;
    Size = 0;
    Header_Size = 0;
    Name.clear();
    Value.clear();
    if (Children.size() && OwnChildren)
        for (size_t i = 0; i < Children.size(); ++i)
            delete Children[i];
    Children.clear();
    if (OwnChildren && Infos.size())
        for (size_t i = 0; i < Infos.size(); ++i)
            delete Infos[i];
    Infos.clear();
    Parser.clear();
    Current_Child = -1;
    NoShow = false;
    OwnChildren = true;
    IsCat = false;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Micro_Xml(std::stringstream& ss, size_t level)
{
    std::string spaces;
    Ztring Name_Escaped;

    if (IsCat || !Name.length())
        goto print_children;

    spaces.resize(level, ' ');
    ss << spaces;

    if (Value.empty())
        ss << "<b";
    else
        ss << "<d";

    Name_Escaped = MediaInfo_Internal::Xml_Name_Escape(Ztring().From_UTF8(Name));
    ss << " o=\"" << Pos << "\" n=\"" << Name_Escaped.To_UTF8() << "\"";
    Name_Escaped.clear();

    if (!Parser.empty())
        ss << " parser=\"" << Parser << "\"";

    for (size_t i = 0; i < Infos.size(); ++i)
    {
        ss << " i";
        if (i)
            ss << (i + 1);
        ss << "=\"" << Infos[i] << "\"";
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        ss << ">" << Value << "</d>";
    }
    else
        ss << " s=\"" << Size << "\">";

	level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Micro_Xml(ss, level);

    if (!IsCat && Name.length())
    {
        //end tag
        if (Value.empty())
        {
            //block
            ss << spaces << "</b>";
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Xml(std::stringstream& ss, size_t level)
{
    std::string spaces;
    Ztring Name_Escaped;

    if (IsCat || !Name.length())
        goto print_children;

    spaces.resize(level, ' ');
    ss << spaces;

    if (Value.empty())
        ss << "<block";
    else
        ss << "<data";

    Name_Escaped = MediaInfo_Internal::Xml_Name_Escape(Ztring().From_UTF8(Name));
    ss << " offset=\"" << Pos << "\" name=\"" << Name_Escaped.To_UTF8() << "\"";
    Name_Escaped.clear();

    if (!Parser.empty())
        ss << " parser=\"" << Parser << "\"";

    for (size_t i = 0; i < Infos.size(); ++i)
    {
        ss << " info";
        if (i)
            ss << (i + 1);
        ss << "=\"" << Infos[i] << "\"";
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        ss << ">" << Value << "</data>";
    }
    else
        ss << " size=\"" << Size << "\">";

    ss << "\n";

	level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Xml(ss, level);

    if (!IsCat && Name.length())
    {
        //end tag
        if (Value.empty())
        {
            //block
            ss << spaces << "</block>" << "\n";
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree_Cat(std::stringstream& ss, size_t level)
{
    std::stringstream offset;
    offset << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << Pos << std::nouppercase << std::dec;

    std::string spaces;
    spaces.resize(level, ' ');

    std::string ToShow;
    ToShow += "---   ";
    ToShow += Name;
    ToShow += "   ---";

    std::string minuses;
    minuses.resize(ToShow.size(), '-');

    ss << offset.str() << spaces << minuses << "\n";
    ss << offset.str() << spaces << ToShow << "\n";
    ss << offset.str() << spaces << minuses << "\n";
    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree(std::stringstream& ss, size_t level)
{
    std::string spaces;

    if (IsCat)
        return Print_Tree_Cat(ss, level);
    else if (!Name.length())
        goto print_children;

    ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << Pos << std::nouppercase << std::dec;
    spaces.resize(level, ' ');
    ss << spaces;
    ss << Name;

#define NB_SPACES 40
    if (!Value.empty())
    {
        ss << ":";
        spaces.clear();
        int nb_free = NB_SPACES - ss.str().length();
        spaces.resize(nb_free > 0 ? nb_free : 1, ' ');
        Value.Set_Output_Format(Element_Node_Data::Format_Tree);
        ss << spaces << Value;
    }
#undef NB_SPACES

    if (!Parser.empty())
        ss << " - parser=\"" << Parser << "\"";

    for (size_t i = 0; i < Infos.size(); ++i)
        ss << " - " << Infos[i];

    if (Value.empty())
        ss << " (" << Size << " bytes)";

    ss << "\n";

	level += 1;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Tree(ss, level);

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print(MediaInfo_Config::trace_Format Format, std::string& Str)
{
    std::stringstream ss;
    int ret = -1;
    switch (Format)
    {
        case MediaInfo_Config::Trace_Format_Tree:
            ret = Print_Tree(ss, 1);
            break;
        case MediaInfo_Config::Trace_Format_CSV:
            break;
        case MediaInfo_Config::Trace_Format_XML:
            ret = Print_Xml(ss, 0);
            break;
        case MediaInfo_Config::Trace_Format_MICRO_XML:
            ret = Print_Micro_Xml(ss, 0);
            break;
        default:
            break;
    }
    Str = ss.str();
    return ret;
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Add_Child(Element_Node* node)
{
    Element_Node *new_node = new Element_Node(*node);
    node->OwnChildren = false;
    Children.push_back(new_node);
}
#endif

}
