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

} //NameSpace
