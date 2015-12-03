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
#include "MediaInfo/HashWrapper.h" //For getting MEDIAINFO_HASH, not in setup
//---------------------------------------------------------------------------

#if MEDIAINFO_HASH

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
using namespace ZenLib;
#if MEDIAINFO_MD5
    extern "C"
    {
        #include <md5.h>
    }
#endif //MEDIAINFO_MD5
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
HashWrapper::HashWrapper (const HashFunctions &Functions)
{
    m = new void*[HashFunction_Max];

    #if MEDIAINFO_MD5
        if (Functions[MD5])
        {
            ((void**)m)[MD5]=new struct MD5Context;
            MD5Init((struct MD5Context*)((void**)m)[MD5]);
        }
        else
            ((void**)m)[MD5]=NULL;
    #endif //MEDIAINFO_MD5
}

HashWrapper::~HashWrapper ()
{
    #if MEDIAINFO_MD5
        delete (struct MD5Context*)((void**)m)[MD5];
    #endif //MEDIAINFO_MD5

    delete[] m;
}

void HashWrapper::Update (const int8u* Buffer, const size_t Buffer_Size)
{
    #if MEDIAINFO_MD5
        if (((void**)m)[MD5])
            MD5Update((struct MD5Context*)((void**)m)[MD5], Buffer, (unsigned int)Buffer_Size);
    #endif //MEDIAINFO_MD5
}

string HashWrapper::Generate (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5 && ((void**)m)[MD5])
        {
            unsigned char Digest[16];
            MD5Final(Digest, (struct MD5Context*)((void**)m)[MD5]);
            Ztring Temp;
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 0));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 2));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 4));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 6));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 8));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+10));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+12));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+14));
            Temp.MakeLowerCase();
            return Temp.To_UTF8();
        }
    #endif //MEDIAINFO_MD5
    return string();
}

string HashWrapper::Name (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5)
            return "MD5";
    #endif //MEDIAINFO_MD5
    return string();
}

} //NameSpace

#endif //MEDIAINFO_HASH
