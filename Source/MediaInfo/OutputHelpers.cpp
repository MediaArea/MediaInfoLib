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
#include "MediaInfo/OutputHelpers.h"
#include <ctime>

using namespace std;
using namespace ZenLib;

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
Ztring XML_Encode (const Ztring& Data)
{
    Ztring Result;
    wstring::size_type Pos;
    for (Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case __T('"'): Result+=__T("&quot;"); break;
            case __T('&'): Result+=__T("&amp;"); break;
            case __T('\''): Result+=__T("&apos;"); break;
            case __T('<'): Result+=__T("&lt;"); break;
            case __T('>'): Result+=__T("&gt;"); break;
            default: Result+=Data[Pos];
        }
    }
    return Result;
}

//---------------------------------------------------------------------------
string XML_Encode (const string& Data)
{
    string Result;
    for (string::size_type Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case '\'': Result+="&apos;"; break;
            case '"': Result+="&quot;"; break;
            case '&': Result+="&amp;"; break;
            case '<': Result+="&lt;"; break;
            case '>': Result+="&gt;"; break;
            default: Result+=Data[Pos];
        }
    }
    return Result;
}

//---------------------------------------------------------------------------
string To_XML (Node& Cur_Node, const int& Level)
{
    string Result;
    if (!Level)
    {
        Result+="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        //Current date/time is ISO format
        time_t Time=time(NULL);
        Ztring TimeS; TimeS.Date_From_Seconds_1970((int32u)Time);
        TimeS.FindAndReplace(__T("UTC "), __T(""));
        TimeS.FindAndReplace(__T(" "), __T("T"));
        TimeS+=__T('Z');

        Result+=string("<!-- Generated at "+TimeS.To_UTF8()+" by "+MediaInfoLib::Config.Info_Version_Get().To_UTF8()+" -->\n");
    }
    else
        Result+="\n";

    if (Cur_Node.XmlCommentOut.size())
        Result+=string(Level, '\t')+"<!-- "+Cur_Node.XmlCommentOut+"\n";

    Result+=string(Level, '\t')+"<"+Cur_Node.Name;

    for (size_t Pos=0; Pos<Cur_Node.Attrs.size(); Pos++)
    {
        if (Cur_Node.Attrs[Pos].first.empty())
            continue;

        Result+=" "+Cur_Node.Attrs[Pos].first+"=\""
               +XML_Encode(Cur_Node.Attrs[Pos].second)+"\"";
    }
    Cur_Node.Attrs.clear(); //Free memory

    if (Cur_Node.Value.empty() && Cur_Node.Childs.empty())
    {
        Result+=" />";
        if (Cur_Node.XmlCommentOut.size())
            Result+="\n"+string(Level, '\t')+"-->";
        return Result;
    }

    Result+=">";

    if (Cur_Node.Value.size())
    {
        if (Cur_Node.Childs.size())
            Result+="\n"+string(Level+1, '\t');
        Result+=XML_Encode(Cur_Node.Value);
    }

    if (Cur_Node.Childs.size())
    {
        for (size_t Pos=0; Pos<Cur_Node.Childs.size(); Pos++)
        {
            if (!Cur_Node.Childs[Pos])
                continue;

            Result+=To_XML(*Cur_Node.Childs[Pos],Level+1);
            delete Cur_Node.Childs[Pos];
            Cur_Node.Childs[Pos]=NULL;
        }
        Cur_Node.Childs.clear(); //Free memory
        Result+="\n"+string(Level, '\t');
    }

    Result+="</"+Cur_Node.Name+">";
    if (Cur_Node.XmlCommentOut.size())
        Result+="\n"+string(Level, '\t')+"-->";
    if (!Level)
        Result+="\n";

    return Result;
}

//---------------------------------------------------------------------------
string JSON_Encode (const string& Data)
{
    string Result;
    for (string::size_type Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case '\b': Result+="\\b"; break;
            case '\f': Result+="\\f"; break;
            case '\n': Result+="\\n"; break;
            case '\r': Result+="\\r"; break;
            case '\t': Result+="\\t"; break;
            case '"': Result+="\\\""; break;
            case '\\': Result+="\\\\"; break;
            default: Result+=Data[Pos];
        }
    }
     return Result;
}

//---------------------------------------------------------------------------
string To_JSON_Attributes(Node& Cur_Node, const int& Level)
{
    string Result;
    for (size_t Pos=0; Pos<Cur_Node.Attrs.size(); Pos++)
    {
        if (Cur_Node.Attrs[Pos].first.empty() || Cur_Node.Attrs[Pos].first.substr(0, 5)=="xmlns" || Cur_Node.Attrs[Pos].first.substr(0, 3)=="xsi")
            continue;

        Result+="\n"+string(Level, '\t')+"\"@"+Cur_Node.Attrs[Pos].first+"\": \""
               +JSON_Encode(Cur_Node.Attrs[Pos].second)+"\"";

        if (Pos<Cur_Node.Attrs.size()-1 || Cur_Node.Value.size() || Cur_Node.Childs.size())
            Result+=",";
    }
    Cur_Node.Attrs.clear(); //Free memory

    return Result;
}

//---------------------------------------------------------------------------
string To_JSON_Elements(Node& Cur_Node, const int& Level)
{
    string Result;
    for (size_t Pos=0; Pos<Cur_Node.Childs.size(); Pos++)
    {
        if (!Cur_Node.Childs[Pos])
            continue;
        Result+="\n"+string(Level, '\t')+"\""+Cur_Node.Childs[Pos]->Name+"\": ";

        Result+="[\n";

        string Name=Cur_Node.Childs[Pos]->Name;
        for (int Pos2=Pos; Pos2<Cur_Node.Childs.size() && Cur_Node.Childs[Pos2]->Name==Name; Pos2++)
        {
            if (!Cur_Node.Childs[Pos2])
                continue;

            Result+=string(Level+1, '\t')+"{";
            Result+=To_JSON_Attributes(*Cur_Node.Childs[Pos2], Level+2);
            Result+=To_JSON_Elements(*Cur_Node.Childs[Pos2], Level+2);
            Result+="\n";

            if(!Cur_Node.Childs[Pos2]->Value.empty())
                Result+=string(Level+2, '\t')+"\"#value\": \""+JSON_Encode(Cur_Node.Childs[Pos2]->Value)+"\"\n";
            Result+=string(Level+1, '\t')+"}";

            if (Pos2<Cur_Node.Childs.size()-1 && Cur_Node.Childs[Pos2]->Name==Cur_Node.Childs[Pos2+1]->Name)
                Result+=",";
            Result+="\n";

            delete Cur_Node.Childs[Pos2];
            Cur_Node.Childs[Pos2]=NULL;
            Pos=Pos2;
        }
        Result+=string(Level, '\t')+"]";

        if (Pos<Cur_Node.Childs.size()-1 || !Cur_Node.Value.empty())
            Result+=",";
    }
    Cur_Node.Childs.clear(); //Free memory

    return Result;
}

//---------------------------------------------------------------------------
string To_JSON (Node& Cur_Node, const int& Level)
{
    string Result;

    if (!Level)
        Result+="{\n";

    Result+=string(Level+1, '\t')+"\""+Cur_Node.Name+"\": ";

    if (Cur_Node.Attrs.empty() && Cur_Node.Childs.empty() && !Cur_Node.Multiple)
    {
        if (Cur_Node.Value.empty())
            Result+="null";
        else
            Result+="\""+JSON_Encode(Cur_Node.Value)+"\"";
        if (!Level)
            Result+="\n}\n";
        return Result;
    }

    Result+="{";
    Result+=To_JSON_Attributes(Cur_Node, Level+2);
    Result+=To_JSON_Elements(Cur_Node, Level+2);
    if (!Cur_Node.Value.empty())
        Result+="\n"+string(Level+2, '\t')+"\"#value\": \""+JSON_Encode(Cur_Node.Value)+"\"";

    Result+="\n"+string(Level+1, '\t')+"}";

    if (!Level)
        Result+="\n}\n";

    return Result;
}

} //NameSpace
