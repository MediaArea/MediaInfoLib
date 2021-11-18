/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Export_GraphH
#define Export_GraphH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
/// @brief Export_Graph
//***************************************************************************

class Export_Graph
{
public :
    //Constructeur/Destructeur
    Export_Graph ();
    ~Export_Graph ();


    //Types
    enum format
    {
        Format_Dot,
        Format_Svg,
        Format_Max
    };

    enum graph
    {
        Graph_All,
        Graph_Ac4,
        Graph_Ed2,
        Graph_Adm,
        Graph_Mpegh3da,
        Graph_Max
    };

    //Functions
    static bool Load();
    ZenLib::Ztring Transform(MediaInfo_Internal &MI, graph Graph, format Format=Format_Dot);

private:
    //Types
    struct relation
    {
        Ztring Src;
        Ztring Dst;
        Ztring Opts;
        relation(Ztring Src, Ztring Dst, Ztring Opts) : Src(Src), Dst(Dst), Opts(Opts)
        {};
    };

    //Functions
    #if defined(MEDIAINFO_AC4_YES)
    Ztring Ac4_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level);
    #endif //defined(MEDIAINFO_AC4_YES)
    #if defined(MEDIAINFO_DOLBYE_YES)
    Ztring Ed2_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level);
    #endif //defined(MEDIAINFO_DOLBYE_YES)
    #if defined(MEDIAINFO_ADM_YES)
    Ztring Adm_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level);
    #endif //defined(MEDIAINFO_ADM_YES)
    #if defined(MEDIAINFO_MPEGH3DA_YES)
    Ztring Mpegh3da_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level);
    #endif //defined(MEDIAINFO_MPEGH3DA_YES )
};

} //NameSpace
#endif
