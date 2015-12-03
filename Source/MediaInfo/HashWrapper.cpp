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
#if MEDIAINFO_SHA1
    extern "C"
    {
        #include <sha1.h>
    }
#endif //MEDIAINFO_SHA1
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// info
//***************************************************************************

const char* HashWrapper_Hex = "0123456789abcdef";

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

    #if MEDIAINFO_SHA1
        if (Functions[SHA1])
        {
            ((void**)m)[SHA1]=new sha1_ctx;
            sha1_begin((sha1_ctx*)((void**)m)[SHA1]);
        }
        else
            ((void**)m)[SHA1]=NULL;
    #endif //MEDIAINFO_SHA1
}

HashWrapper::~HashWrapper ()
{
    #if MEDIAINFO_MD5
        delete (struct MD5Context*)((void**)m)[MD5];
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        delete (sha1_ctx*)((void**)m)[SHA1];
    #endif //MEDIAINFO_SHA1

    delete[] m;
}

void HashWrapper::Update (const int8u* Buffer, const size_t Buffer_Size)
{
    #if MEDIAINFO_MD5
        if (((void**)m)[MD5])
            MD5Update((struct MD5Context*)((void**)m)[MD5], Buffer, (unsigned int)Buffer_Size);
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (((void**)m)[SHA1])
            sha1_hash(Buffer, (unsigned long)Buffer_Size, (sha1_ctx*)((void**)m)[SHA1]);
    #endif //MEDIAINFO_SHA1
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

    #if MEDIAINFO_SHA1
        if (Function==SHA1 && ((void**)m)[SHA1])
        {
            unsigned char Digest[20];
            sha1_end(Digest, (sha1_ctx*)((void**)m)[SHA1]);
            string DigestS;
            DigestS.reserve(40);
            for (size_t i=0; i<20; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
        }
    #endif //MEDIAINFO_SHA1

    return string();
}

string HashWrapper::Name (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5)
            return "MD5";
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Function==SHA1)
            return "SHA-1";
    #endif //MEDIAINFO_SHA1

    return string();
}

} //NameSpace

#endif //MEDIAINFO_HASH
