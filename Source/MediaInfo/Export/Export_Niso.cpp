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
#if defined(MEDIAINFO_NISO_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_Niso.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include "MediaInfo/OutputHelpers.h"
#include <ctime>
#include <cmath>

using namespace std;

//---------------------------------------------------------------------------

namespace MediaInfoLib
{
//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Export_Niso::Export_Niso ()
{
}

//---------------------------------------------------------------------------
Export_Niso::~Export_Niso ()
{
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
Node* Transform_Header()
{
    //Root node
    Node* Node_Header=new Node("mix:mix");
    Node_Header->Add_Attribute("xmlns:mix", "http://www.loc.gov/mix/v20");
    Node_Header->Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    Node_Header->Add_Attribute("xsi:schemaLocation", "http://www.loc.gov/mix/v20 http://www.loc.gov/standards/mix/mix20/mix20.xsd");

    return Node_Header;
}

//---------------------------------------------------------------------------
void ComputeSamplingFrequency(Node* Parent, Ztring& Value)
{
    while (Value.size()>0 && Value[Value.size()-1]==__T('0'))
        Value.resize(Value.size()-1);
    if (Value.size()>0 && Value[Value.size()-1]==__T('.'))
        Value.resize(Value.size()-1);

    int32u SamplingFrequencyDenominator=0;
    size_t Dot=Value.find(__T('.'));
    if (Dot!=std::string::npos)
    {
        Value.erase(Dot, 1);
        SamplingFrequencyDenominator=(int32u)std::pow((double)10, (double)Value.size()-Dot);
    }

    Parent->Add_Child("mix:numerator", Value);

    if (SamplingFrequencyDenominator)
        Parent->Add_Child("mix:denominator", Ztring().From_Number(SamplingFrequencyDenominator));
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
Ztring Export_Niso::Transform(MediaInfo_Internal &MI, Ztring ExternalMetadataValues, Ztring ExternalMetaDataConfig)
{
    bool UseExternalMetaData=(!ExternalMetaDataConfig.empty() && !ExternalMetadataValues.empty());

    Ztring ToReturn;
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Image); Pos++)
    {
        Node* Node_Root=Transform_Header();
        Node* Node_Extension=NULL;

        //Use external metadata
        if (UseExternalMetaData)
        {
            Ztring FileName;
            if (!MI.Get(Stream_General, 0, General_FileName).empty())
                FileName=MI.Get(Stream_General, 0, General_FileName);
            if (!MI.Get(Stream_General, 0, General_FileExtension).empty())
                FileName+=__T('.')+MI.Get(Stream_General, 0, General_FileExtension);
            if (FileName.empty())
            {
                MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "File name not found in external metadata file");
                delete Node_Root;
                return Ztring();
            }

            Node_Extension= new Node("mix:Extension");
            Node* Node_CoreMain=NULL;

            if (ExternalMetaDataConfig.find(__T("<ebucore:ebuCoreMain"))<100)
            {
                //TODO: merge with EBUCore code
                //Current date/time is ISO format
                time_t Seconds=time(NULL);
                Ztring DateTime; DateTime.Date_From_Seconds_1970((int32u)Seconds);
                if (DateTime.size() >= 4 && DateTime[0] == __T('U') && DateTime[1] == __T('T') && DateTime[2] == __T('C') && DateTime[3] == __T(' '))
                {
                    DateTime.erase(0, 4);
                    DateTime += __T('Z');
                }
                Ztring Date=DateTime.substr(0, 10);
                Ztring Time=DateTime.substr(11);

                Node_CoreMain=Node_Extension->Add_Child("ebucore:ebuCoreMain");
                Node_CoreMain->Add_Attribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
                {
                    Node_CoreMain->Add_Attribute("xmlns:ebucore", "urn:ebu:metadata-schema:ebucore");
                    Node_CoreMain->Add_Attribute("xmlns:xalan", "http://xml.apache.org/xalan");
                    Node_CoreMain->Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
                    Node_CoreMain->Add_Attribute("xsi:schemaLocation", string("urn:ebu:metadata-schema:ebucore http") + string(MediaInfoLib::Config.Https_Get() ? "s" : "") + "://www.ebu.ch/metadata/schemas/EBUCore/20171009/ebucore.xsd");
                    Node_CoreMain->Add_Attribute("version", "1.8");
                    Node_CoreMain->Add_Attribute("writingLibraryName", "MediaInfoLib");
                    Node_CoreMain->Add_Attribute("writingLibraryVersion", MediaInfoLib::Config.Info_Version_Get().SubString(__T(" - v"), Ztring()));
                }
                Node_CoreMain->Add_Attribute("dateLastModified", Date);
                Node_CoreMain->Add_Attribute("timeLastModified", Time);
            }

            if (!ExternalMetadata(FileName, ExternalMetadataValues, ExternalMetaDataConfig,
                                  ZtringList(__T("mix:Extension;ebucore:ebuCoreMain")), __T(""),
                                  Node_CoreMain?Node_CoreMain:Node_Extension, NULL))
            {
                delete Node_Root;
                return Ztring();
            }
        }

        //BasicDigitalObjectInformation
        Node* Node_BasicDigitalObjectInformation=Node_Root->Add_Child("mix:BasicDigitalObjectInformation");
        Node* Node_ObjectIdentifier=Node_BasicDigitalObjectInformation->Add_Child("mix:ObjectIdentifier");

        Node_ObjectIdentifier->Add_Child("mix:objectIdentifierType", std::string("MediaInfo"));
        Node_ObjectIdentifier->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_CompleteName, "mix:objectIdentifierValue");

        if (MI.Get(Stream_General, 0, General_FileSize).To_int64u()>0)
            Node_BasicDigitalObjectInformation->Add_Child("mix:fileSize", MI.Get(Stream_General, 0, General_FileSize));

        if (MI.Get(Stream_General, 0, General_Format)==__T("TIFF"))
             Node_BasicDigitalObjectInformation->Add_Child("mix:FormatDesignation")->Add_Child("mix:formatName", std::string("image/tiff"));

        std::string byteOrder=MI.Get(Stream_Image, Pos, Image_Format_Settings_Endianness).To_UTF8();
        if (!byteOrder.empty())
            Node_BasicDigitalObjectInformation->Add_Child("mix:byteOrder", byteOrder=="Little"?std::string("little endian"):(byteOrder=="Big"?std::string("big endian"): byteOrder));

        std::string CompressionScheme=MI.Get(Stream_Image, Pos, Image_Format).To_UTF8();
        if (!CompressionScheme.empty())
            Node_BasicDigitalObjectInformation->Add_Child("mix:Compression")->Add_Child("mix:compressionScheme", CompressionScheme=="Raw"?std::string("Uncompressed"):CompressionScheme);

        if (!MI.Get(Stream_Image, Pos, Image_Width).empty() ||
            !MI.Get(Stream_Image, Pos, Image_Height).empty() ||
            !MI.Get(Stream_Image, Pos, Image_ColorSpace).empty())
        {
            Node* Node_BasicImageCharacteristics=Node_Root->Add_Child("mix:BasicImageInformation")->Add_Child("mix:BasicImageCharacteristics");

            Node_BasicImageCharacteristics->Add_Child_IfNotEmpty(MI, Stream_Image, Pos, Image_Width, "mix:imageWidth");
            Node_BasicImageCharacteristics->Add_Child_IfNotEmpty(MI, Stream_Image, Pos, Image_Height, "mix:imageHeight");

            //ReferenceBlackWhite
            std::string ColorSpace=MI.Get(Stream_Image, Pos, Image_ColorSpace).To_UTF8();
            if (!ColorSpace.empty())
            {
                Node* Node_PhotometricInterpretation=Node_BasicImageCharacteristics->Add_Child("mix:PhotometricInterpretation");
                Node_PhotometricInterpretation->Add_Child("mix:colorSpace", ColorSpace);

                Node* ReferenceBlackWhite=Node_PhotometricInterpretation->Add_Child("mix:ReferenceBlackWhite");
                for (size_t i=0; i < ColorSpace.size(); i++)
                {
                    Node* Component=ReferenceBlackWhite->Add_Child("mix:Component");
                    Component->Add_Child("mix:componentPhotometricInterpretation", string(1, ColorSpace[i]));
                    Node* footroom=Component->Add_Child("mix:footroom");
                    footroom->Add_Child("mix:numerator", Ztring::ToZtring(0));
                    int8u BitDepth=MI.Get(Stream_Image, Pos, Image_BitDepth).To_int8u();
                    if (BitDepth)
                    {
                        Node* headroom=Component->Add_Child("mix:headroom");
                        headroom->Add_Child("mix:numerator", Ztring::ToZtring((1<<BitDepth)-1));
                    }
                }
            }
        }

        //ImageCaptureMetadata
        std::string Make=MI.Get(Stream_General, 0, General_Encoded_Application_CompanyName).To_UTF8();
        std::string Model=MI.Get(Stream_General, 0, General_Encoded_Library_Name).To_UTF8();
        std::string Software=MI.Get(Stream_General, 0, General_Encoded_Application_Name).To_UTF8();
        std::string Encoded_Date=MI.Get(Stream_Image, Pos, Image_Encoded_Date).To_UTF8();
        if (!Make.empty() || !Model.empty() || !Software.empty() || !Encoded_Date.empty())
        {
            Node* Node_ImageCaptureMetadata=Node_Root->Add_Child("mix:ImageCaptureMetadata");

            if (!Encoded_Date.empty())
            {
                if (Encoded_Date.size()>4 && Encoded_Date[4]==':')
                    Encoded_Date[4]='-';
                if (Encoded_Date.size()>7 && Encoded_Date[7]==':')
                    Encoded_Date[7]='-';
                if (Encoded_Date.size()>10 && Encoded_Date[10]==' ')
                    Encoded_Date[10]='T';
                Node* Node_GeneralCaptureInformation=Node_ImageCaptureMetadata->Add_Child("mix:GeneralCaptureInformation");
                Node_GeneralCaptureInformation->Add_Child("mix:dateTimeCreated", Encoded_Date);
            }

            if (!Make.empty() || !Model.empty() || !Software.empty() )
            {
                Node* Node_ScannerCapture=Node_ImageCaptureMetadata->Add_Child("mix:ScannerCapture");
                if (!Make.empty())
                    Node_ScannerCapture->Add_Child("mix:scannerManufacturer", Make);
                if (!Model.empty())
                    Node_ScannerCapture->Add_Child("mix:ScannerModel")->Add_Child("mix:scannerModelName", Model);
                if (!Software.empty())
                    Node_ScannerCapture->Add_Child("mix:ScanningSystemSoftware")->Add_Child("mix:scanningSoftwareName", Software);
            }
        }

        if (!MI.Get(Stream_Image, Pos, __T("Density_X")).empty() ||
            !MI.Get(Stream_Image, Pos, __T("Density_Y")).empty() ||
            !MI.Get(Stream_Image, Pos, Image_ColorSpace).empty())
        {
            Node* Node_ImageAssessmentMetadata=Node_Root->Add_Child("mix:ImageAssessmentMetadata");

            //SpatialMetrics
            string samplingFrequencyUnit=MI.Get(Stream_Image, Pos, __T("Density_Unit")).To_UTF8();
            Ztring xSamplingFrequency=MI.Get(Stream_Image, Pos, __T("Density_X"));
            Ztring ySamplingFrequency=MI.Get(Stream_Image, Pos, __T("Density_Y"));
            if (!xSamplingFrequency.empty() || !ySamplingFrequency.empty())
            {
                Node* Node_SpatialMetrics=Node_ImageAssessmentMetadata->Add_Child("mix:SpatialMetrics");

                if (samplingFrequencyUnit.empty())
                    Node_SpatialMetrics->Add_Child("mix:samplingFrequencyUnit", string("no absolute unit of measurement"));
                else if (samplingFrequencyUnit=="dpi")
                    Node_SpatialMetrics->Add_Child("mix:samplingFrequencyUnit", string("in."));
                else if (samplingFrequencyUnit=="dpcm")
                    Node_SpatialMetrics->Add_Child("mix:samplingFrequencyUnit", string("cm"));

                if (!xSamplingFrequency.empty())
                    ComputeSamplingFrequency(Node_SpatialMetrics->Add_Child("mix:xSamplingFrequency"), xSamplingFrequency);
                if (!ySamplingFrequency.empty())
                    ComputeSamplingFrequency(Node_SpatialMetrics->Add_Child("mix:ySamplingFrequency"), ySamplingFrequency);
            }

            size_t SamplesPerPixel=MI.Get(Stream_Image, Pos, Image_ColorSpace).length();
            if (SamplesPerPixel)
            {
                Node* Node_ImageColorEncoding=Node_ImageAssessmentMetadata->Add_Child("mix:ImageColorEncoding");

                Node* Node_BitsPerSample=new Node("mix:BitsPerSample");
                for (size_t Pos2=0; Pos2<SamplesPerPixel; ++Pos2)
                    Node_BitsPerSample->Add_Child_IfNotEmpty(MI, Stream_Image, Pos, Image_BitDepth, "mix:bitsPerSampleValue");

                if (!Node_BitsPerSample->Childs.empty())
                {
                    Node_BitsPerSample->Add_Child("mix:bitsPerSampleUnit", std::string("integer"));
                    Node_ImageColorEncoding->Childs.push_back(Node_BitsPerSample);
                }
                else
                   delete Node_BitsPerSample;

                Node_ImageColorEncoding->Add_Child("mix:samplesPerPixel", Ztring().From_Number(SamplesPerPixel).To_UTF8());
            }
        }

        if (Node_Extension)
            Node_Root->Childs.push_back(Node_Extension);

        ToReturn+=Ztring().From_UTF8(To_XML(*Node_Root, 0, true, true).c_str());
    }

    if (ToReturn.empty())
        ToReturn+=Ztring().From_UTF8(To_XML(*Transform_Header(), 0, true, true).c_str())+=__T("\n");

    //Carriage return
    if (MediaInfoLib::Config.LineSeparator_Get()!=__T("\n"))
        ToReturn.FindAndReplace(__T("\n"), MediaInfoLib::Config.LineSeparator_Get(), 0, Ztring_Recursive);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif
