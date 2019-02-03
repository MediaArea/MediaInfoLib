/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Export_NisoH
#define Export_NisoH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
/// @brief Export_EbuCore
//***************************************************************************

class Export_Niso
{
public :
    //Constructeur/Destructeur
    Export_Niso ();
    ~Export_Niso ();

    ZenLib::Ztring Transform(MediaInfo_Internal &MI, Ztring ExternalMetadataValues=Ztring(), Ztring ExternalMetaDataConfig=Ztring());
};

} //NameSpace
#endif
