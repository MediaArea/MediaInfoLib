/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// TIFF Format
//
// From
// http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf
// http://partners.adobe.com/public/developer/en/tiff/TIFFphotoshop.pdf
// http://www.fileformat.info/format/tiff/
// http://en.wikipedia.org/wiki/Tagged_Image_File_Format
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#if defined(MEDIAINFO_TIFF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Tiff.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
namespace Tiff_Tag
{
    const int16u ImageWidth                 = 256;
    const int16u ImageLength                = 257;
    const int16u BitsPerSample              = 258;
    const int16u Compression                = 259;
    const int16u PhotometricInterpretation  = 262;
    const int16u Make                       = 271;
    const int16u Model                      = 272;
    const int16u StripOffsets               = 273;
    const int16u SamplesPerPixel            = 277;
    const int16u RowsPerStrip               = 278;
    const int16u StripByteCounts            = 279;
    const int16u XResolution                = 282;
    const int16u YResolution                = 283;
    const int16u PlanarConfiguration        = 284;
    const int16u ResolutionUnit             = 296;
    const int16u Software                   = 305;
    const int16u ExtraSamples               = 338;
}

//---------------------------------------------------------------------------
static const char* Tiff_Compression(int32u Compression)
{
    switch (Compression)
    {
        case     1 : return "Raw";
        case     2 : return "CCITT Group 3";
        case     3 : return "CCITT T.4";
        case     5 : return "LZW";
        case     6 : return "JPEG (TIFF v6)";
        case     7 : return "JPEG (ISO)";
        case     8 : return "Deflate";
        case 32773 : return "PackBits";
        default    : return ""; //Unknown
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_Compression_Mode(int32u Compression)
{
    switch (Compression)
    {
        case     1 :
        case     2 :
        case     3 :
        case     5 :
        case     8 :
        case 32773 : return "Lossless";
        default    : return ""; //Unknown or depends of the compresser (e.g. JPEG can be lossless or lossy)
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_PhotometricInterpretation(int32u PhotometricInterpretation)
{
    switch (PhotometricInterpretation)
    {
        case     0 :
        case     1 : return "B/W or Grey scale";
        case     2 : return "RGB";
        case     3 : return "Palette";
        case     4 : return "Transparency mask";
        case     5 : return "CMYK";
        case     6 : return "YCbCr";
        case     8 : return "CIELAB";
        default    : return ""; //Unknown
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_PhotometricInterpretation_ColorSpace (int32u PhotometricInterpretation)
{
    switch (PhotometricInterpretation)
    {
        case     0 :
        case     1 : return "Y";
        case     2 : return "RGB";
        case     3 : return "RGB"; //Palette
        case     4 : return "A"; //Transparency mask;
        case     5 : return "CMYK";
        case     6 : return "YUV"; //YCbCr
        case     8 : return "CIELAB"; //What is it?
        default    : return ""; //Unknown
    }
}

static const char* Tiff_ExtraSamples_ColorSpace(int32u ExtraSamples)
{
    switch (ExtraSamples)
    {
        case     1 : return "A";
        default    : return ""; //Unknown
    }
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Tiff::Streams_Finish()
{
    if (Retrieve_Const(Stream_General, 0, General_Format).empty()) {
        Fill(Stream_General, 0, General_Format, "TIFF");
    }
    if (!Count_Get(Stream_Image)) {
        Stream_Prepare(Stream_Image);
    }
    Fill(Stream_Image, 0, Image_Format_Settings, LittleEndian?"Little":"Big");
    Fill(Stream_Image, 0, Image_Format_Settings_Endianness, LittleEndian?"Little":"Big");

    infos::iterator Info;

    //Width
    Info=Infos[0].find(Tiff_Tag::ImageWidth);
    if (Info!=Infos[0].end())
        Fill(Stream_Image, 0, Image_Width, Info->second.Read());

    //Height
    Info=Infos[0].find(Tiff_Tag::ImageLength);
    if (Info!=Infos[0].end())
        Fill(Stream_Image, 0, Image_Height, Info->second.Read());

    //BitsPerSample
    Info=Infos[0].find(Tiff_Tag::BitsPerSample);
    if (Info!=Infos[0].end())
    {
        if (Info->second.size()>1)
        {
            bool IsOk=true;
            for (size_t Pos=1; Pos<Info->second.size(); ++Pos)
                if (Info->second[Pos]!=Info->second[0])
                    IsOk=false;
            if (IsOk)
                Info->second.resize(1); //They are all same, we display 1 piece of information
        }

        Fill(Stream_Image, 0, Image_BitDepth, Info->second.Read());
    }

    //Compression
    Info=Infos[0].find(Tiff_Tag::Compression);
    if (Info!=Infos[0].end())
    {
        int32u Value=Info->second.Read().To_int32u();
        Fill(Stream_Image, 0, Image_Format, Tiff_Compression(Value));
        Fill(Stream_Image, 0, Image_Codec, Tiff_Compression(Value));
        Fill(Stream_Image, 0, Image_Compression_Mode, Tiff_Compression_Mode(Value));
    }

    //PhotometricInterpretation
    Info=Infos[0].find(Tiff_Tag::PhotometricInterpretation);
    if (Info!=Infos[0].end())
    {
        int32u Value=Info->second.Read().To_int32u();
        Fill(Stream_Image, 0, Image_ColorSpace, Tiff_PhotometricInterpretation_ColorSpace(Value));
        //Note: should we differeniate between raw RGB and palette (also RGB actually...)
    }

    //Make
    Info=Infos[0].find(Tiff_Tag::Make);
    if (Info!=Infos[0].end())
        Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Info->second.Read()); // TODO: if this is removed, we lose some info in the displayed string when there are several sources for application name (TIFF, Exif, XMP...)

    //Model
    Info=Infos[0].find(Tiff_Tag::Model);
    if (Info!=Infos[0].end())
        Fill(Stream_General, 0, General_Encoded_Library_Name, Info->second.Read()); // TODO: if this is removed, we lose some info in the displayed string when there are several sources for application name (TIFF, Exif, XMP...)

    //XResolution
    Info=Infos[0].find(Tiff_Tag::XResolution);
    if (Info!=Infos[0].end())
    {
        Fill(Stream_Image, 0, "Density_X", Info->second.Read());
        Fill_SetOptions(Stream_Image, 0, "Density_X", "N NT");
    }

    //YResolution
    Info=Infos[0].find(Tiff_Tag::YResolution);
    if (Info!=Infos[0].end())
    {
        Fill(Stream_Image, 0, "Density_Y", Info->second.Read());
        Fill_SetOptions(Stream_Image, 0, "Density_Y", "N NT");
    }

    //ResolutionUnit
    Info=Infos[0].find(Tiff_Tag::ResolutionUnit);
    if (Info!=Infos[0].end())
    {
        switch (Info->second.Read().To_int32u())
        {
            case 0 : break;
            case 1 : Fill(Stream_Image, 0, "Density_Unit", "dpcm"); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT"); break;
            case 2 : Fill(Stream_Image, 0, "Density_Unit", "dpi"); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT"); break;
            default: Fill(Stream_Image, 0, "Density_Unit", Info->second.Read()); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT");
        }
    }
    else if (Infos[0].find(Tiff_Tag::XResolution)!=Infos[0].end() || Infos[0].find(Tiff_Tag::YResolution)!=Infos[0].end())
    {
        Fill(Stream_Image, 0, "Density_Unit", "dpi");
        Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT");
    }
    
    //XResolution or YResolution 
    if (Infos[0].find(Tiff_Tag::XResolution)!=Infos[0].end() || Infos[0].find(Tiff_Tag::YResolution)!=Infos[0].end())
    {
        Ztring X=Retrieve(Stream_Image, 0, "Density_X");
        if (X.empty())
            X.assign(1, __T('?'));
        Ztring Y=Retrieve(Stream_Image, 0, "Density_Y");
        if (Y.empty())
            Y.assign(1, __T('?'));
        if (X!=Y)
        {
            X+=__T('x');
            X+=Y;
        }
        Y=Retrieve(Stream_Image, 0, "Density_Unit");
        if (!Y.empty())
        {
            X+=__T(' ');
            X+=Y;
            Fill(Stream_Image, 0, "Density/String", X);
        }
    }

    //Software
    Info=Infos[0].find(Tiff_Tag::Software);
    if (Info!=Infos[0].end())
        Fill(Stream_General, 0, General_Encoded_Application_Name, Info->second.Read()); // TODO: if this is removed, we lose some info in the displayed string when there are several sources for application name (TIFF, Exif, XMP...)

    //ExtraSamples
    Info=Infos[0].find(Tiff_Tag::ExtraSamples);
    if (Info!=Infos[0].end())
    {
        Ztring ColorSpace=Retrieve(Stream_Image, 0, Image_ColorSpace);
        ColorSpace+=Ztring().From_UTF8(Tiff_ExtraSamples_ColorSpace(Info->second.Read().To_int32u()));
        Fill(Stream_Image, 0, Image_ColorSpace, ColorSpace, true);
    }

    File_Exif::Streams_Finish();

    //FileSize
    Info = Infos[0].find(Tiff_Tag::StripOffsets);
    if (Info != Infos[0].end())
    {
        size_t ExpectedFileSize_Pos=(int64u)-1;
        for (size_t i=0; i<Info->second.size(); i++)
        {
            auto Offset=Info->second[i].To_int64u();
            if (ExpectedFileSize<Offset)
            {
                ExpectedFileSize=Offset;
                ExpectedFileSize_Pos=i;
            }
        }
        if (ExpectedFileSize!=(int64u)-1)
        {
            Info = Infos[0].find(Tiff_Tag::StripByteCounts);
            if (Info != Infos[0].end() && ExpectedFileSize_Pos<Info->second.size())
            {
                ExpectedFileSize+=Info->second[ExpectedFileSize_Pos].To_int64u();
            }
        }
        
        if (ExpectedFileSize>File_Size)
            IsTruncated(ExpectedFileSize, false, "TIFF");
    }
}

} //NameSpace

#endif

