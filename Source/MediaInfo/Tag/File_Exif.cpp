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
#if defined(MEDIAINFO_EXIF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Exif.h"
#if defined(MEDIAINFO_JPEG_YES)
    #include "MediaInfo/Image/File_Jpeg.h"
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
// EXIF Data Types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace Exif_Type {
    const int16u UnsignedByte     = 1;
    const int16u ASCIIStrings     = 2;
    const int16u UnsignedShort    = 3;
    const int16u UnsignedLong     = 4;
    const int16u UnsignedRational = 5;
    const int16u SignedByte       = 6;
    const int16u Undefined        = 7;
    const int16u SignedShort      = 8;
    const int16u SignedLong       = 9;
    const int16u SignedRational   = 10;
    const int16u SingleFloat      = 11;
    const int16u DoubleFloat      = 12;
}

//---------------------------------------------------------------------------
static const char* Exif_Type_Name(int32u Type)
{
    switch (Type)
    {
    case Exif_Type::UnsignedByte    : return "unsigned byte";
    case Exif_Type::ASCIIStrings    : return "ASCII strings";
    case Exif_Type::UnsignedShort   : return "unsigned short";
    case Exif_Type::UnsignedLong    : return "unsigned long";
    case Exif_Type::UnsignedRational: return "unsigned rational";
    case Exif_Type::SignedByte      : return "signed byte";
    case Exif_Type::Undefined       : return "undefined";
    case Exif_Type::SignedShort     : return "signed short";
    case Exif_Type::SignedLong      : return "signed long";
    case Exif_Type::SignedRational  : return "signed rational";
    case Exif_Type::SingleFloat     : return "single float";
    case Exif_Type::DoubleFloat     : return "double float";
    default                         : return "unknown";
    }
}

//---------------------------------------------------------------------------
static const int8u Exif_Type_Size(int32u Type)
{
    switch (Type)
    {
        case Exif_Type::UnsignedByte    : return 1;
        case Exif_Type::ASCIIStrings    : return 1;
        case Exif_Type::UnsignedShort   : return 2;
        case Exif_Type::UnsignedLong    : return 4;
        case Exif_Type::UnsignedRational: return 8;
        case Exif_Type::SignedByte      : return 1;
        case Exif_Type::Undefined       : return 1;
        case Exif_Type::SignedShort     : return 2;
        case Exif_Type::SignedLong      : return 4;
        case Exif_Type::SignedRational  : return 8;
        case Exif_Type::SingleFloat     : return 4;
        case Exif_Type::DoubleFloat     : return 8;
        default                         : return 0;
    }
}

//---------------------------------------------------------------------------
// EXIF Tags
//---------------------------------------------------------------------------

#define ELEM(_TAG,_NAME) \
    const int16u _NAME = _TAG; \

struct exif_tag_desc {
    int16u Tag_ID;
    const char* Description;
};
#define ELEM_TRACE(_NAME,_DESC) \
    {_NAME, _DESC}, \

namespace IFD0 {
    ELEM(0x0001, InteroperabilityIndex)
    ELEM(0x0002, InteroperabilityVersion)
    ELEM(0x000B, ProcessingSoftware)
    ELEM(0x00FE, SubfileType)
    ELEM(0x00FF, OldSubfileType)
    ELEM(0x0100, ImageWidth)
    ELEM(0x0101, ImageHeight)
    ELEM(0x0102, BitsPerSample)
    ELEM(0x0103, Compression)
    ELEM(0x0106, PhotometricInterpretation)
    ELEM(0x0107, Thresholding)
    ELEM(0x0108, CellWidth)
    ELEM(0x0109, CellLength)
    ELEM(0x010A, FillOrder)
    ELEM(0x010D, DocumentName)
    ELEM(0x010E, ImageDescription)
    ELEM(0x010F, Make)
    ELEM(0x0110, Model)
    ELEM(0x0111, StripOffsets)
    ELEM(0x0112, Orientation)
    ELEM(0x0115, SamplesPerPixel)
    ELEM(0x0116, RowsPerStrip)
    ELEM(0x0117, StripByteCounts)
    ELEM(0x0118, MinSampleValue)
    ELEM(0x0119, MaxSampleValue)
    ELEM(0x011A, XResolution)
    ELEM(0x011B, YResolution)
    ELEM(0x011C, PlanarConfiguration)
    ELEM(0x011D, PageName)
    ELEM(0x011E, XPosition)
    ELEM(0x011F, YPosition)
    ELEM(0x0122, GrayResponseUnit)
    ELEM(0x0128, ResolutionUnit)
    ELEM(0x0129, PageNumber)
    ELEM(0x012D, TransferFunction)
    ELEM(0x0131, Software)
    ELEM(0x0132, DateTimeFile)
    ELEM(0x013B, Artist)
    ELEM(0x013C, HostComputer)
    ELEM(0x013D, Predictor)
    ELEM(0x013E, WhitePoint)
    ELEM(0x013F, PrimaryChromaticities)
    ELEM(0x0141, HalftoneHints)
    ELEM(0x0142, TileWidth)
    ELEM(0x0143, TileLength)
    ELEM(0x014A, A100DataOffset)
    ELEM(0x014C, InkSet)
    ELEM(0x0151, TargetPrinter)
    ELEM(0x0201, ImageOffset)
    ELEM(0x0202, ImageLength)
    ELEM(0x0211, YCbCrCoefficients)
    ELEM(0x0212, YCbCrSubSampling)
    ELEM(0x0213, YCbCrPositioning)
    ELEM(0x0214, ReferenceBlackWhite)
    ELEM(0x02BC, ApplicationNotes)
    ELEM(0x4746, Rating)
    ELEM(0x4749, RatingPercent)
    ELEM(0x8298, Copyright)
    ELEM(0x830E, PixelScale)
    ELEM(0x83BB, IPTC_NAA)
    ELEM(0x8480, IntergraphMatrix)
    ELEM(0x8482, ModelTiePoint)
    ELEM(0x8546, SEMInfo)
    ELEM(0x85D8, ModelTransform)
    ELEM(0x8649, PhotoshopSettings)
    ELEM(0x8769, IFDExif)
    ELEM(0x8773, ICC_Profile)
    ELEM(0x87AF, GeoTiffDirectory)
    ELEM(0x87B0, GeoTiffDoubleParams)
    ELEM(0x87B1, GeoTiffAsciiParams)
    ELEM(0x8825, GPSInfoIFD)
    ELEM(0x935C, ImageSourceData)
    ELEM(0x9C9B, WinExpTitle)
    ELEM(0x9C9C, WinExpComment)
    ELEM(0x9C9D, WinExpAuthor)
    ELEM(0x9C9E, WinExpKeywords)
    ELEM(0x9C9F, WinExpSubject)
    ELEM(0xA480, GDALMetadata)
    ELEM(0xA481, GDALNoData)
    ELEM(0xC4A5, PrintIM)
    ELEM(0xC634, MakerNote)
    ELEM(0xC635, MakerNoteSafety)
    ELEM(0xC65A, CalibrationIlluminant1)
    ELEM(0xC65B, CalibrationIlluminant2)
    ELEM(0xC691, CurrentICCProfile)
    ELEM(0xCD49, JXLDistance)
    ELEM(0xCD4A, JXLEffort)
    ELEM(0xCD4B, JXLDecodeSpeed)
    ELEM(0xCEA1, SEAL)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(InteroperabilityIndex, "Interoperability index")
    ELEM_TRACE(InteroperabilityVersion, "Interoperability version")
    ELEM_TRACE(ProcessingSoftware, "Processing software")
    ELEM_TRACE(SubfileType, "Subfile type")
    ELEM_TRACE(OldSubfileType, "Old subfile type")
    ELEM_TRACE(ImageWidth, "Image width")
    ELEM_TRACE(ImageHeight, "Image height")
    ELEM_TRACE(BitsPerSample, "Bits per sample")
    ELEM_TRACE(Compression, "Compression")
    ELEM_TRACE(PhotometricInterpretation, "Photometric interpretation")
    ELEM_TRACE(Thresholding, "Thresholding")
    ELEM_TRACE(CellWidth, "Cell width")
    ELEM_TRACE(CellLength, "Cell length")
    ELEM_TRACE(FillOrder, "Fill order")
    ELEM_TRACE(DocumentName, "Document name")
    ELEM_TRACE(ImageDescription, "Description")
    ELEM_TRACE(Make, "Manufacturer of image input equipment")
    ELEM_TRACE(Model, "Model of image input equipment")
    ELEM_TRACE(StripOffsets, "Strip offsets")
    ELEM_TRACE(Orientation, "Orientation")
    ELEM_TRACE(SamplesPerPixel, "Samples per pixel")
    ELEM_TRACE(RowsPerStrip, "Rows per strip")
    ELEM_TRACE(StripByteCounts, "Strip byte counts")
    ELEM_TRACE(MinSampleValue, "Min sample value")
    ELEM_TRACE(MaxSampleValue, "Max sample value")
    ELEM_TRACE(XResolution, "X resolution")
    ELEM_TRACE(YResolution, "Y resolution")
    ELEM_TRACE(PlanarConfiguration, "Planar configuration")
    ELEM_TRACE(PageName, "Page name")
    ELEM_TRACE(XPosition, "X position")
    ELEM_TRACE(YPosition, "Y position")
    ELEM_TRACE(GrayResponseUnit, "Gray response unit")
    ELEM_TRACE(ResolutionUnit, "Resolution unit")
    ELEM_TRACE(PageNumber, "Page number")
    ELEM_TRACE(TransferFunction, "Transfer function")
    ELEM_TRACE(Software, "Software used")
    ELEM_TRACE(DateTimeFile, "Date and time of file")
    ELEM_TRACE(Artist, "Artist")
    ELEM_TRACE(HostComputer, "Host computer")
    ELEM_TRACE(Predictor, "Predictor")
    ELEM_TRACE(WhitePoint, "White point")
    ELEM_TRACE(PrimaryChromaticities, "Primary chromaticities")
    ELEM_TRACE(HalftoneHints, "Halftone hints")
    ELEM_TRACE(TileWidth, "Tile width")
    ELEM_TRACE(TileLength, "Tile length")
    ELEM_TRACE(A100DataOffset, "A100 IFD")
    ELEM_TRACE(InkSet, "Ink set")
    ELEM_TRACE(TargetPrinter, "Target printer")
    ELEM_TRACE(ImageOffset, "Image offset")
    ELEM_TRACE(ImageLength, "Image byte count")
    ELEM_TRACE(YCbCrCoefficients, "Transformation matrix")
    ELEM_TRACE(YCbCrSubSampling, "Chroma subsampling")
    ELEM_TRACE(YCbCrPositioning, "Chroma positioning")
    ELEM_TRACE(ReferenceBlackWhite, "Reference black and white")
    ELEM_TRACE(ApplicationNotes, "Application notes")
    ELEM_TRACE(Rating, "Rating")
    ELEM_TRACE(RatingPercent, "Rating (percent)")
    ELEM_TRACE(Copyright, "Copyright")
    ELEM_TRACE(PixelScale, "Pixel scale")
    ELEM_TRACE(IPTC_NAA, "IPTC NAA")
    ELEM_TRACE(IntergraphMatrix, "Intergraph matrix")
    ELEM_TRACE(ModelTiePoint, "Model tie point")
    ELEM_TRACE(SEMInfo, "SEM info")
    ELEM_TRACE(ModelTransform, "Model transform")
    ELEM_TRACE(PhotoshopSettings, "Photoshop settings")
    ELEM_TRACE(IFDExif, "Exif IFD")
    ELEM_TRACE(ICC_Profile, "ICC profile")
    ELEM_TRACE(GeoTiffDirectory, "GeoTiff directory")
    ELEM_TRACE(GeoTiffDoubleParams, "GeoTiff double params")
    ELEM_TRACE(GeoTiffAsciiParams, "GeoTiff ASCII params")
    ELEM_TRACE(GPSInfoIFD, "GPS IFD")
    ELEM_TRACE(ImageSourceData, "Photoshop image source data")
    ELEM_TRACE(WinExpTitle, "Title (Windows Explorer)")
    ELEM_TRACE(WinExpComment, "Comment (Windows Explorer)")
    ELEM_TRACE(WinExpAuthor, "Author (Windows Explorer)")
    ELEM_TRACE(WinExpKeywords, "Keywords (Windows Explorer)")
    ELEM_TRACE(WinExpSubject, "Subject (Windows Explorer)")
    ELEM_TRACE(GDALMetadata, "GDAL metadata")
    ELEM_TRACE(GDALNoData, "GDAL no data")
    ELEM_TRACE(PrintIM, "Print IM")
    ELEM_TRACE(MakerNote, "Manufacturer notes")
    ELEM_TRACE(MakerNoteSafety, "Manufacturer notes safety")
    ELEM_TRACE(CalibrationIlluminant1, "Calibration illuminant 1")
    ELEM_TRACE(CalibrationIlluminant2, "Calibration illuminant 2")
    ELEM_TRACE(CurrentICCProfile, "Current ICC profile")
    ELEM_TRACE(JXLDistance, "Distance (JXL)")
    ELEM_TRACE(JXLEffort, "Effort (JXL)")
    ELEM_TRACE(JXLDecodeSpeed, "Decode speed (JXL)")
    ELEM_TRACE(SEAL, "SEAL")
};
};

namespace IFDExif {
    ELEM(0x829A, ExposureTime)
    ELEM(0x829D, FNumber)
    ELEM(0x8822, ExposureProgram)
    ELEM(0x8824, SpectralSensitivity)
    ELEM(0x8827, PhotographicSensitivity)
    ELEM(0x8828, OECF)
    ELEM(0x882A, TimeZoneOffset)
    ELEM(0x882B, SelfTimerMode)
    ELEM(0x8830, SensitivityType)
    ELEM(0x8831, StandardOutputSensitivity)
    ELEM(0x8832, RecommendedExposureIndex)
    ELEM(0x8833, ISOSpeed)
    ELEM(0x8834, ISOSpeedLatitudeyyy)
    ELEM(0x8835, ISOSpeedLatitudezzz)
    ELEM(0x9000, ExifVersion)
    ELEM(0x9003, DateTimeOriginal)
    ELEM(0x9004, DateTimeDigitized)
    ELEM(0x9009, GooglePlusUploadCode)
    ELEM(0x9010, OffsetTimeFile)
    ELEM(0x9011, OffsetTimeOriginal)
    ELEM(0x9012, OffsetTimeDigitized)
    ELEM(0x9101, ComponentsConfiguration)
    ELEM(0x9102, CompressedBitsPerPixel)
    ELEM(0x9201, ShutterSpeedValue)
    ELEM(0x9202, ApertureValue)
    ELEM(0x9203, BrightnessValue)
    ELEM(0x9204, ExposureCompensation)
    ELEM(0x9205, MaxApertureValue)
    ELEM(0x9206, SubjectDistance)
    ELEM(0x9207, MeteringMode)
    ELEM(0x9208, LightSource)
    ELEM(0x9209, Flash)
    ELEM(0x920A, FocalLength)
    ELEM(0x9211, ImageNumber)
    ELEM(0x9212, SecurityClassification)
    ELEM(0x9213, ImageHistory)
    ELEM(0x9214, SubjectArea)
    ELEM(0x927C, MakerNote)
    ELEM(0x9286, UserComment)
    ELEM(0x9290, SubSecTime)
    ELEM(0x9291, SubSecTimeOriginal)
    ELEM(0x9292, SubSecTimeDigitized)
    ELEM(0x9400, AmbientTemperature)
    ELEM(0x9401, Humidity)
    ELEM(0x9402, Pressure)
    ELEM(0x9403, WaterDepth)
    ELEM(0x9404, Acceleration)
    ELEM(0x9405, CameraElevationAngle)
    ELEM(0x9999, XiaomiSettings)
    ELEM(0x9A00, XiaomiModel)
    ELEM(0xA000, FlashpixVersion)
    ELEM(0xA001, ColorSpace)
    ELEM(0xA002, PixelXDimension)
    ELEM(0xA003, PixelYDimension)
    ELEM(0xA004, RelatedSoundFile)
    ELEM(0xA005, InteroperabilityIFD)
    ELEM(0xA20B, FlashEnergy)
    ELEM(0xA20E, FocalPlaneXResolution)
    ELEM(0xA20F, FocalPlaneYResolution)
    ELEM(0xA210, FocalPlaneResolutionUnit)
    ELEM(0xA214, SubjectLocation)
    ELEM(0xA215, ExposureIndex)
    ELEM(0xA217, SensingMethod)
    ELEM(0xA300, FileSource)
    ELEM(0xA301, SceneType)
    ELEM(0xA302, CFAPattern)
    ELEM(0xA401, CustomRendered)
    ELEM(0xA402, ExposureMode)
    ELEM(0xA403, WhiteBalance)
    ELEM(0xA404, DigitalZoomRatio)
    ELEM(0xA405, FocalLengthIn35mmFormat)
    ELEM(0xA406, SceneCaptureType)
    ELEM(0xA407, GainControl)
    ELEM(0xA408, Contrast)
    ELEM(0xA409, Saturation)
    ELEM(0xA40A, Sharpness)
    ELEM(0xA40C, SubjectDistanceRange)
    ELEM(0xA420, ImageUniqueID)
    ELEM(0xA430, CameraOwnerName)
    ELEM(0xA431, BodySerialNumber)
    ELEM(0xA432, LensSpecification)
    ELEM(0xA433, LensMake)
    ELEM(0xA434, LensModel)
    ELEM(0xA435, LensSerialNumber)
    ELEM(0xA436, ImageTitle)
    ELEM(0xA437, Photographer)
    ELEM(0xA438, ImageEditor)
    ELEM(0xA439, CameraFirmware)
    ELEM(0xA43A, RAWDevelopingSoftware)
    ELEM(0xA43B, ImageEditingSoftware)
    ELEM(0xA43C, MetadataEditingSoftware)
    ELEM(0xA460, CompositeImage)
    ELEM(0xA461, CompositeImageCount)
    ELEM(0xA500, Gamma)
    ELEM(0xEA1C, Padding)
    ELEM(0xEA1D, OffsetSchema)
    ELEM(0xFDE8, Adobe_OwnerName)
    ELEM(0xFDE9, Adobe_SerialNumber)
    ELEM(0xFDEA, Adobe_Lens)
    ELEM(0xFE4C, Adobe_RawFile)
    ELEM(0xFE4D, Adobe_Converter)
    ELEM(0xFE4E, Adobe_WhiteBalance)
    ELEM(0xFE51, Adobe_Exposure)
    ELEM(0xFE52, Adobe_Shadows)
    ELEM(0xFE53, Adobe_Brightness)
    ELEM(0xFE54, Adobe_Contrast)
    ELEM(0xFE55, Adobe_Saturation)
    ELEM(0xFE56, Adobe_Sharpness)
    ELEM(0xFE57, Adobe_Smoothness)
    ELEM(0xFE58, Adobe_MoireFilter)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(ExposureTime, "Exposure time")
    ELEM_TRACE(FNumber, "F number")
    ELEM_TRACE(ExposureProgram, "Exposure program")
    ELEM_TRACE(SpectralSensitivity, "Spectral sensitivity")
    ELEM_TRACE(PhotographicSensitivity, "Photographic sensitivity")
    ELEM_TRACE(OECF, "Optoelectric coefficient")
    ELEM_TRACE(TimeZoneOffset, "Time zone offset")
    ELEM_TRACE(SelfTimerMode, "Self timer mode")
    ELEM_TRACE(SensitivityType, "Sensitivity type")
    ELEM_TRACE(StandardOutputSensitivity, "Standard output sensitivity")
    ELEM_TRACE(RecommendedExposureIndex, "Recommended exposure index")
    ELEM_TRACE(ISOSpeed, "ISOSpeed")
    ELEM_TRACE(ISOSpeedLatitudeyyy, "ISOSpeed latitude yyy")
    ELEM_TRACE(ISOSpeedLatitudezzz, "ISOSpeed Latitude zzz")
    ELEM_TRACE(ExifVersion, "Exif Version")
    ELEM_TRACE(DateTimeOriginal, "Date and time original")
    ELEM_TRACE(DateTimeDigitized, "Date and time digitized")
    ELEM_TRACE(GooglePlusUploadCode, "GooglePlusUploadCode")
    ELEM_TRACE(OffsetTimeFile, "Date and time offset file")
    ELEM_TRACE(OffsetTimeOriginal, "Date and time offset original")
    ELEM_TRACE(OffsetTimeDigitized, "Date and time offset digitized")
    ELEM_TRACE(ComponentsConfiguration, "Meaning of each component")
    ELEM_TRACE(CompressedBitsPerPixel, "Image compression mode")
    ELEM_TRACE(ShutterSpeedValue, "Shutter speed")
    ELEM_TRACE(ApertureValue, "Aperture")
    ELEM_TRACE(BrightnessValue, "Brightness")
    ELEM_TRACE(ExposureCompensation, "Exposure bias")
    ELEM_TRACE(MaxApertureValue, "Maximum lens aperture")
    ELEM_TRACE(SubjectDistance, "Subject distance")
    ELEM_TRACE(MeteringMode, "Metering mode")
    ELEM_TRACE(LightSource, "Light source")
    ELEM_TRACE(Flash, "Flash")
    ELEM_TRACE(FocalLength, "Lens focal length")
    ELEM_TRACE(ImageNumber, "Image number")
    ELEM_TRACE(SecurityClassification, "Security classification")
    ELEM_TRACE(ImageHistory, "Image history")
    ELEM_TRACE(SubjectArea, "Subject area")
    ELEM_TRACE(MakerNote, "Manufacturer notes")
    ELEM_TRACE(UserComment, "User comments")
    ELEM_TRACE(SubSecTime, "Date and time subseconds file")
    ELEM_TRACE(SubSecTimeOriginal, "Date and time subseconds original")
    ELEM_TRACE(SubSecTimeDigitized, "Date and time subseconds digitized")
    ELEM_TRACE(AmbientTemperature, "Ambient temperature")
    ELEM_TRACE(Humidity, "Humidity")
    ELEM_TRACE(Pressure, "Pressure")
    ELEM_TRACE(WaterDepth, "Water depth")
    ELEM_TRACE(Acceleration, "Acceleration")
    ELEM_TRACE(CameraElevationAngle, "Camera elevation angle")
    ELEM_TRACE(XiaomiSettings, "Settings (Xiaomi)")
    ELEM_TRACE(XiaomiModel, "Model (Xiaomi)")
    ELEM_TRACE(FlashpixVersion, "Flashpix version")
    ELEM_TRACE(ColorSpace, "Colorspace")
    ELEM_TRACE(PixelXDimension, "Image width")
    ELEM_TRACE(PixelYDimension, "Image height")
    ELEM_TRACE(RelatedSoundFile, "Related sound file")
    ELEM_TRACE(InteroperabilityIFD, "Interoperability IFD")
    ELEM_TRACE(FlashEnergy, "Flash energy")
    ELEM_TRACE(FocalPlaneXResolution, "Focal plane X resolution")
    ELEM_TRACE(FocalPlaneYResolution, "Focal plane Y resolution")
    ELEM_TRACE(FocalPlaneResolutionUnit, "Focal plane resolutionUnit")
    ELEM_TRACE(SubjectLocation, "Subject location")
    ELEM_TRACE(ExposureIndex, "Exposure index")
    ELEM_TRACE(SensingMethod, "Sensing method")
    ELEM_TRACE(FileSource, "File source")
    ELEM_TRACE(SceneType, "Scene type")
    ELEM_TRACE(CFAPattern, "CFA pattern")
    ELEM_TRACE(CustomRendered, "Custom image processing")
    ELEM_TRACE(ExposureMode, "Exposure mode")
    ELEM_TRACE(WhiteBalance, "White balance")
    ELEM_TRACE(DigitalZoomRatio, "Digital zoom ratio")
    ELEM_TRACE(FocalLengthIn35mmFormat, "Focal length in 35mm film")
    ELEM_TRACE(SceneCaptureType, "Scene capture type")
    ELEM_TRACE(GainControl, "Gain control")
    ELEM_TRACE(Contrast, "Contrast")
    ELEM_TRACE(Saturation, "Saturation")
    ELEM_TRACE(Sharpness, "Sharpness")
    ELEM_TRACE(SubjectDistanceRange, "Subject distance range")
    ELEM_TRACE(ImageUniqueID, "Unique image ID")
    ELEM_TRACE(CameraOwnerName, "Camera owner name")
    ELEM_TRACE(BodySerialNumber, "Body serial number")
    ELEM_TRACE(LensSpecification, "Lens specification")
    ELEM_TRACE(LensMake, "Lens manufacturer")
    ELEM_TRACE(LensModel, "Lens model")
    ELEM_TRACE(LensSerialNumber, "Lens serial number")
    ELEM_TRACE(ImageTitle, "Title")
    ELEM_TRACE(Photographer, "Photographer")
    ELEM_TRACE(ImageEditor, "Editor")
    ELEM_TRACE(CameraFirmware, "Camera firmware")
    ELEM_TRACE(RAWDevelopingSoftware, "RAW developing software")
    ELEM_TRACE(ImageEditingSoftware, "Editing software")
    ELEM_TRACE(MetadataEditingSoftware, "Metadata editing software")
    ELEM_TRACE(CompositeImage, "Composite iImage")
    ELEM_TRACE(CompositeImageCount, "Composite image count")
    ELEM_TRACE(Gamma, "Gamma")
    ELEM_TRACE(Padding, "Padding")
    ELEM_TRACE(OffsetSchema, "Offset schema (Microsoft)")
    ELEM_TRACE(Adobe_OwnerName, "Owner name (Adobe)")
    ELEM_TRACE(Adobe_SerialNumber, "Serial number (Adobe)")
    ELEM_TRACE(Adobe_Lens, "Lens (Adobe)")
    ELEM_TRACE(Adobe_RawFile, "Raw file (Adobe)")
    ELEM_TRACE(Adobe_Converter, "Converter (Adobe)")
    ELEM_TRACE(Adobe_WhiteBalance, "White balance 2 (Adobe)")
    ELEM_TRACE(Adobe_Exposure, "Exposure (Adobe)")
    ELEM_TRACE(Adobe_Shadows, "Shadows (Adobe)")
    ELEM_TRACE(Adobe_Brightness, "Brightness (Adobe)")
    ELEM_TRACE(Adobe_Contrast, "Contrast (Adobe)")
    ELEM_TRACE(Adobe_Saturation, "Saturation (Adobe)")
    ELEM_TRACE(Adobe_Sharpness, "Sharpness (Adobe)")
    ELEM_TRACE(Adobe_Smoothness, "Smoothness (Adobe)")
    ELEM_TRACE(Adobe_MoireFilter, "Moire filter (Adobe)")
};
};

namespace IFDGPS {
    ELEM(0x0000, GPSVersionID)
    ELEM(0x0001, GPSLatitudeRef)
    ELEM(0x0002, GPSLatitude)
    ELEM(0x0003, GPSLongitudeRef)
    ELEM(0x0004, GPSLongitude)
    ELEM(0x0005, GPSAltitudeRef)
    ELEM(0x0006, GPSAltitude)
    ELEM(0x0007, GPSTimeStamp)
    ELEM(0x0008, GPSSatellites)
    ELEM(0x0009, GPSStatus)
    ELEM(0x000A, GPSMeasureMode)
    ELEM(0x000B, GPSDOP)
    ELEM(0x000C, GPSSpeedRef)
    ELEM(0x000D, GPSSpeed)
    ELEM(0x000E, GPSTrackRef)
    ELEM(0x000F, GPSTrack)
    ELEM(0x0010, GPSImgDirectionRef)
    ELEM(0x0011, GPSImgDirection)
    ELEM(0x0012, GPSMapDatum)
    ELEM(0x0013, GPSDestLatitudeRef)
    ELEM(0x0014, GPSDestLatitude)
    ELEM(0x0015, GPSDestLongitudeRef)
    ELEM(0x0016, GPSDestLongitude)
    ELEM(0x0017, GPSDestBearingRef)
    ELEM(0x0018, GPSDestBearing)
    ELEM(0x0019, GPSDestDistanceRef)
    ELEM(0x001A, GPSDestDistance)
    ELEM(0x001B, GPSProcessingMethod)
    ELEM(0x001C, GPSAreaInformation)
    ELEM(0x001D, GPSDateStamp)
    ELEM(0x001E, GPSDifferential)
    ELEM(0x001F, GPSHPositioningError)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(GPSVersionID, "Version")
    ELEM_TRACE(GPSLatitudeRef, "North or South Latitude")
    ELEM_TRACE(GPSLatitude, "Latitude")
    ELEM_TRACE(GPSLongitudeRef, "East or West Longitude")
    ELEM_TRACE(GPSLongitude, "Longitude")
    ELEM_TRACE(GPSAltitudeRef, "Altitude reference")
    ELEM_TRACE(GPSAltitude, "Altitude")
    ELEM_TRACE(GPSTimeStamp, "Time (atomic clock)")
    ELEM_TRACE(GPSSatellites, "satellites used for measurement")
    ELEM_TRACE(GPSStatus, "Receiver status")
    ELEM_TRACE(GPSMeasureMode, "Measurement mode")
    ELEM_TRACE(GPSDOP, "Measurement precision")
    ELEM_TRACE(GPSSpeedRef, "Speed unit")
    ELEM_TRACE(GPSSpeed, "Speed of GPS receiver")
    ELEM_TRACE(GPSTrackRef, "Reference for direction of movement")
    ELEM_TRACE(GPSTrack, "Direction of movement")
    ELEM_TRACE(GPSImgDirectionRef, "Reference for direction of image")
    ELEM_TRACE(GPSImgDirection, "Direction of image")
    ELEM_TRACE(GPSMapDatum, "Geodetic survey data used")
    ELEM_TRACE(GPSDestLatitudeRef, "Reference for latitude of destination")
    ELEM_TRACE(GPSDestLatitude, "Latitude of destination")
    ELEM_TRACE(GPSDestLongitudeRef, "Reference for longitude of destination")
    ELEM_TRACE(GPSDestLongitude, "Longitude of destination")
    ELEM_TRACE(GPSDestBearingRef, "Reference for bearing of destination")
    ELEM_TRACE(GPSDestBearing, "Bearing of destination")
    ELEM_TRACE(GPSDestDistanceRef, "Reference for distance to destination")
    ELEM_TRACE(GPSDestDistance, "Distance to destination")
    ELEM_TRACE(GPSProcessingMethod, "Name of processing method")
    ELEM_TRACE(GPSAreaInformation, "Name of area")
    ELEM_TRACE(GPSDateStamp, "Date")
    ELEM_TRACE(GPSDifferential, "Differential correction")
    ELEM_TRACE(GPSHPositioningError, "Horizontal positioning error")
};
};

//---------------------------------------------------------------------------
struct exif_tag_desc_size
{
    exif_tag_desc* Table;
    size_t Size;
    const char* Description;
};
#define DESC_TABLE(_TABLE,_DESC) { _TABLE::Desc, sizeof(_TABLE::Desc) / sizeof(*_TABLE::Desc), _DESC },
enum kind_of_ifd
{
    Kind_IFD0,
    Kind_IFD1,
    Kind_Exif,
    Kind_GPS,
    Kind_ParsingThumbnail,
};
exif_tag_desc_size Exif_Descriptions[] =
{
    DESC_TABLE(IFD0, "IFD0 (primary image)")
    DESC_TABLE(IFD0, "IFD1 (thumbnail)")
    DESC_TABLE(IFDExif, "Exif")
    DESC_TABLE(IFDGPS, "GPS")
};
static string Exif_Tag_Description(int8u NameSpace, int16u Tag_ID)
{
    for (size_t Pos = 0; Pos < Exif_Descriptions[NameSpace].Size; ++Pos)
    {
        if (Exif_Descriptions[NameSpace].Table[Pos].Tag_ID == Tag_ID)
            return Exif_Descriptions[NameSpace].Table[Pos].Description;
    }
    return Ztring::ToZtring_From_CC2(Tag_ID).To_UTF8();
}

//---------------------------------------------------------------------------
namespace Exif_IFD0_Orientation {
    const int16u Horizontal                   = 1;
    const int16u MirrorHorizontal             = 2;
    const int16u Rotate180                    = 3;
    const int16u MirrorVertical               = 4;
    const int16u MirrorHorizontalRotate270CW  = 5;
    const int16u Rotate90CW                   = 6;
    const int16u MirrorHorizontalRotate90CW   = 7;
    const int16u Rotate270CW                  = 8;
}

//---------------------------------------------------------------------------
static const char* Exif_IFD0_Orientation_Name(int16u orientation)
{
    switch (orientation)
    {
    case Exif_IFD0_Orientation::Horizontal: return "Horizontal (normal)";
    case Exif_IFD0_Orientation::MirrorHorizontal: return "Mirror horizontal";
    case Exif_IFD0_Orientation::Rotate180: return "Rotate 180";
    case Exif_IFD0_Orientation::MirrorVertical: return "Mirror vertical";
    case Exif_IFD0_Orientation::MirrorHorizontalRotate270CW: return "Mirror horizontal and rotate 270 CW";
    case Exif_IFD0_Orientation::Rotate90CW: return "Rotate 90 CW";
    case Exif_IFD0_Orientation::MirrorHorizontalRotate90CW: return "Mirror horizontal and rotate 90 CW";
    case Exif_IFD0_Orientation::Rotate270CW: return "Rotate 270 CW";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFD0_Compression_Name(int16u compression)
{
    switch (compression)
    {
    case 1: return "Uncompressed";
    case 2: return "CCITT 1D";
    case 3: return "T4/Group 3 Fax";
    case 4: return "T6/Group 4 Fax";
    case 5: return "LZW";
    case 6: return "JPEG (old-style)";
    case 7: return "JPEG";
    case 8: return "Adobe Deflate";
    case 9: return "JBIG B&W";
    case 10: return "JBIG Color";
    case 99: return "JPEG";
    case 262: return "Kodak 262";
    case 32766: return "Next";
    case 32767: return "Sony ARW Compressed";
    case 32769: return "Packed RAW";
    case 32770: return "Samsung SRW Compressed";
    case 32771: return "CCIRLEW";
    case 32772: return "Samsung SRW Compressed 2";
    case 32773: return "PackBits";
    case 32809: return "Thunderscan";
    case 32867: return "Kodak KDC Compressed";
    case 32895: return "IT8CTPAD";
    case 32896: return "IT8LW";
    case 32897: return "IT8MP";
    case 32898: return "IT8BL";
    case 32908: return "PixarFilm";
    case 32909: return "PixarLog";
    case 32946: return "Deflate";
    case 32947: return "DCS";
    case 33003: return "Aperio JPEG 2000 YCbCr";
    case 33005: return "Aperio JPEG 2000 RGB";
    case 34661: return "JBIG";
    case 34676: return "SGILog";
    case 34677: return "SGILog24";
    case 34712: return "JPEG 2000";
    case 34713: return "Nikon NEF Compressed";
    case 34715: return "JBIG2 TIFF FX";
    case 34718: return "Microsoft Document Imaging (MDI) Binary Level Codec";
    case 34719: return "Microsoft Document Imaging (MDI) Progressive Transform Codec";
    case 34720: return "Microsoft Document Imaging (MDI) Vector";
    case 34887: return "ESRI Lerc";
    case 34892: return "Lossy JPEG";
    case 34925: return "LZMA2";
    case 34926: return "Zstd (old)";
    case 34927: return "WebP (old)";
    case 34933: return "PNG";
    case 34934: return "JPEG XR";
    case 50000: return "Zstd";
    case 50001: return "WebP";
    case 50002: return "JPEG XL (old)";
    case 52546: return "JPEG XL";
    case 65000: return "Kodak DCR Compressed";
    case 65535: return "Pentax PEF Compressed";
    default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Exif_ExifIFD_Tag_LightSource_Name(int tagID) {
    switch (tagID) {
    case 0: return "Unknown";
    case 1: return "Daylight";
    case 2: return "Fluorescent";
    case 3: return "Tungsten (Incandescent)";
    case 4: return "Flash";
    case 9: return "Fine Weather";
    case 10: return "Cloudy";
    case 11: return "Shade";
    case 12: return "Daylight Fluorescent";
    case 13: return "Day White Fluorescent";
    case 14: return "Cool White Fluorescent";
    case 15: return "White Fluorescent";
    case 16: return "Warm White Fluorescent";
    case 17: return "Standard Light A";
    case 18: return "Standard Light B";
    case 19: return "Standard Light C";
    case 20: return "D55";
    case 21: return "D65";
    case 22: return "D75";
    case 23: return "D50";
    case 24: return "ISO Studio Tungsten";
    case 255: return "Other";
    default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Exif_IFDExif_Flash_Name(int flashValue) {
    switch (flashValue) {
    case 0x0:  return "No Flash";
    case 0x1:  return "Fired";
    case 0x5:  return "Fired, Return not detected";
    case 0x7:  return "Fired, Return detected";
    case 0x8:  return "On, Did not fire";
    case 0x9:  return "On, Fired";
    case 0xd:  return "On, Return not detected";
    case 0xf:  return "On, Return detected";
    case 0x10: return "Off, Did not fire";
    case 0x14: return "Off, Did not fire, Return not detected";
    case 0x18: return "Auto, Did not fire";
    case 0x19: return "Auto, Fired";
    case 0x1d: return "Auto, Fired, Return not detected";
    case 0x1f: return "Auto, Fired, Return detected";
    case 0x20: return "No flash function";
    case 0x30: return "Off, No flash function";
    case 0x41: return "Fired, Red-eye reduction";
    case 0x45: return "Fired, Red-eye reduction, Return not detected";
    case 0x47: return "Fired, Red-eye reduction, Return detected";
    case 0x49: return "On, Red-eye reduction";
    case 0x4d: return "On, Red-eye reduction, Return not detected";
    case 0x4f: return "On, Red-eye reduction, Return detected";
    case 0x50: return "Off, Red-eye reduction";
    case 0x58: return "Auto, Did not fire, Red-eye reduction";
    case 0x59: return "Auto, Fired, Red-eye reduction";
    case 0x5d: return "Auto, Fired, Red-eye reduction, Return not detected";
    case 0x5f: return "Auto, Fired, Red-eye reduction, Return detected";
    default:   return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDExif_ColorSpace_Name(int16u Tag_ID)
{
    switch (Tag_ID)
    {
    case 0x1: return "sRGB";
    case 0x2: return "Adobe RGB";
    case 0xfffd: return "Wide Gamut RGB";
    case 0xfffe: return "ICC Profile";
    case 0xffff: return "Uncalibrated";
    default: return "";
    }
}


//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Exif::File_Exif() : LittleEndian{}, currentIFD(Kind_IFD0)
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Streams_Finish()
{
    const auto Infos_Image_It = Infos.find(Kind_IFD0);
    const auto Infos_Thumbnail_It = Infos.find(Kind_IFD1);
    const auto Infos_Exif_It = Infos.find(Kind_Exif);
    const auto Infos_GPS_It = Infos.find(Kind_GPS);

    if (Infos_Image_It != Infos.end()) {
        const auto Infos_Image = Infos_Image_It->second;
        const auto Make = Infos_Image.find(IFD0::Make); if (Make != Infos_Image.end()) Fill(Stream_General, 0, General_Encoded_Hardware_CompanyName, Make->second.Read());
        const auto Model = Infos_Image.find(IFD0::Model); if (Model != Infos_Image.end()) Fill(Stream_General, 0, General_Encoded_Hardware_Model, Model->second.Read());
        const auto Software = Infos_Image.find(IFD0::Software); if (Software != Infos_Image.end()) Fill(Stream_General, 0, General_Encoded_Application, Software->second.Read());
        const auto Description = Infos_Image.find(IFD0::ImageDescription); if (Description != Infos_Image.end()) Fill(Stream_General, 0, General_Description, Description->second.Read());
        const auto Copyright = Infos_Image.find(IFD0::Copyright); if (Copyright != Infos_Image.end()) Fill(Stream_General, 0, General_Copyright, Copyright->second.Read());
    }
    if (Infos_Exif_It != Infos.end()) {
        const auto Infos_Exif = Infos_Exif_It->second;
        const auto DateTimeOriginal = Infos_Exif.find(IFDExif::DateTimeOriginal);
        const auto SubSecTimeOriginal = Infos_Exif.find(IFDExif::SubSecTimeOriginal);
        const auto OffsetTimeOriginal = Infos_Exif.find(IFDExif::OffsetTimeOriginal);
        Ztring datetime;
        if (DateTimeOriginal != Infos_Exif.end())
            datetime += DateTimeOriginal->second.Read();
        if (SubSecTimeOriginal != Infos_Exif.end())
            datetime += __T(".") + SubSecTimeOriginal->second.Read();
        if (OffsetTimeOriginal != Infos_Exif.end())
            datetime += OffsetTimeOriginal->second.Read();
        Fill(Stream_General, 0, General_Recorded_Date, datetime);
    }
    if (Infos_GPS_It != Infos.end()) {
        const auto Infos_GPS = Infos_GPS_It->second;
        const auto GPSLatitude = Infos_GPS.find(IFDGPS::GPSLatitude);
        const auto GPSLatitudeRef = Infos_GPS.find(IFDGPS::GPSLatitudeRef);
        const auto GPSLongitude = Infos_GPS.find(IFDGPS::GPSLongitude);
        const auto GPSLongitudeRef = Infos_GPS.find(IFDGPS::GPSLongitudeRef);
        const auto GPSAltitude = Infos_GPS.find(IFDGPS::GPSAltitude);
        if (GPSLatitude != Infos_GPS.end() && GPSLatitude->second.size() == 3 && GPSLatitudeRef != Infos_GPS.end() && GPSLongitude != Infos_GPS.end() && GPSLongitude->second.size() == 3 && GPSLongitudeRef != Infos_GPS.end()) {
            Ztring location = GPSLatitude->second.at(0) + __T("\u00B0") + GPSLatitude->second.at(1) + __T("'") + GPSLatitude->second.at(2) + __T("\"") + GPSLatitudeRef->second.at(0) + __T(" ")
                + GPSLongitude->second.at(0) + __T("\u00B0") + GPSLongitude->second.at(1) + __T("'") + GPSLongitude->second.at(2) + __T("\"") + GPSLongitudeRef->second.at(0) + __T(" ");
            if (GPSAltitude != Infos_GPS.end())
                location += GPSAltitude->second.Read() + __T("m");
            Fill(Stream_General, 0, General_Recorded_Location, location);
        }
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::FileHeader_Parse()
{
    //Parsing
    int32u Alignment;
    if (FromHeif) {
        int32u Size;
        Get_B4 (Size,                                           "Size");
        string Identifier;
        Get_String(Size, Identifier,                            "Identifier");
        if (Size != 6 || strncmp(Identifier.c_str(), "Exif\0", 6)) {
            Reject();
            return;
        }
        OffsetFromContainer = 10;
    }
    Get_C4 (Alignment,                                          "Alignment");
    if (Alignment == 0x49492A00)
        LittleEndian = true;
    else if (Alignment == 0x4D4D002A)
        LittleEndian = false;
    else
    {
        Reject();
        return;
    }

    //The only IFD that is known at forehand is the first one, it's offset is placed byte 4-7 in the file.
    Get_IFDOffset(Kind_IFD0);

    FILLING_BEGIN();
        if (IFD_Offsets.empty()) {
            Reject();
            return;
        }
        auto FirstIFDOffset = IFD_Offsets.begin()->first;
        IFD_Offsets.erase(IFD_Offsets.begin());
        if (FirstIFDOffset == (int32u)-1) {
            Reject();
            return;
        }
        Accept();
        Stream_Prepare(Stream_Image);

        //Initial IFD
        GoToOffset(FirstIFDOffset);
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Header_Parse()
{
    //Handling remaining IFD data
    if (!IfdItems.empty())
    {
        if (Buffer_Offset - OffsetFromContainer != IfdItems.begin()->first) {
            IfdItems.clear(); //There was a problem during the seek, trashing remaining positions from last IFD
            Finish();
            return;
        }
        #ifdef MEDIAINFO_TRACE
        Header_Fill_Code(IfdItems.begin()->second.Tag, Exif_Tag_Description(currentIFD, IfdItems.begin()->second.Tag).c_str());
        #else //MEDIAINFO_TRACE
        Header_Fill_Code(IfdItems.begin()->second.Tag);
        #endif //MEDIAINFO_TRACE
        Header_Fill_Size(Exif_Type_Size(IfdItems.begin()->second.Type) * IfdItems.begin()->second.Count);
        return;
    }

    //Thumbnail
    if (currentIFD == Kind_ParsingThumbnail) {
        Header_Fill_Code(0xFFFFFFFF, "Thumbnail");
        Header_Fill_Size(Buffer_Size - Buffer_Offset);
        return;
    }

    //Get number of directories for this IFD
    int16u NrOfDirectories;
    Get_X2 (NrOfDirectories,                                    "NrOfDirectories");
    
    //Filling
    Header_Fill_Code(0xFFFFFFFF, "IFD"); //OxFFFFFFFF can not be a Tag, so using it as a magic value
    Header_Fill_Size(2 + 12 * ((int64u)NrOfDirectories) + 4); //2 for header + 12 per directory + 4 for next IFD offset
}

//---------------------------------------------------------------------------
void File_Exif::Data_Parse()
{
    if (IfdItems.empty())
    {
        if (currentIFD == Kind_ParsingThumbnail) {
            Thumbnail();
        }
        else {
            #ifdef MEDIAINFO_TRACE
            Element_Info1(Exif_Descriptions[currentIFD].Description);
            #endif //MEDIAINFO_TRACE

            //Default values
            Infos[currentIFD].clear();

            //Parsing new IFD
            int32u IFDOffset;
            while (Element_Offset + 12 < Element_Size)
                Read_Directory();
            Get_IFDOffset(currentIFD == Kind_IFD0 ? Kind_IFD1 : (int8u)-1);
        }
    }
    else
    {
        //Handling remaining IFD data from a previous IFD
        GetValueOffsetu(IfdItems.begin()->second); //Parsing the IFD item
        IfdItems.erase(IfdItems.begin()->first); //Removing IFD item from the list of IFD items to parse
    }

    //Some items are not inside the directory, jumping to the offset
    if (!IfdItems.empty())
        GoToOffset(IfdItems.begin()->first);
    else
    {
        //This IFD is finished, going to the next IFD
        if (!IFD_Offsets.empty()) {
            GoToOffset(IFD_Offsets.begin()->first);
            currentIFD = IFD_Offsets.begin()->second;
            IFD_Offsets.erase(IFD_Offsets.begin());
            return;
        }

        //Thumbnail
        if (currentIFD != Kind_ParsingThumbnail) {
            const auto Infos_Thumbnail_It = Infos.find(Kind_IFD1);
            if (Infos_Thumbnail_It != Infos.end()) {
                const auto Infos_Thumbnail = Infos_Thumbnail_It->second;
                const auto ImageOffset = Infos_Thumbnail.find(IFD0::ImageOffset);
                if (ImageOffset != Infos_Thumbnail.end() && ImageOffset->second.size() == 1) {
                    int32u IFD_Offset = ImageOffset->second.at(0).To_int32u();
                    GoToOffset(IFD_Offset);
                    currentIFD = Kind_ParsingThumbnail;
                    return;
                }
            }
        }

        Finish(); //No more IFDs
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Read_Directory()
{
    // Each directory consist of 4 fields
    // Get information for this directory
    Element_Begin0();
    ifditem IfdItem;
    Get_X2 (IfdItem.Tag,                                        "TagID"); Param_Info1(Exif_Tag_Description(currentIFD, IfdItem.Tag));
    Get_X2 (IfdItem.Type,                                       "Data type"); Param_Info1(Exif_Type_Name(IfdItem.Type));
    Get_X4 (IfdItem.Count,                                      "Number of components");
    

    #ifdef MEDIAINFO_TRACE
    Element_Name(Exif_Tag_Description(currentIFD, IfdItem.Tag).c_str());
    #endif //MEDIAINFO_TRACE


    int32u Size = Exif_Type_Size(IfdItem.Type) * IfdItem.Count;

    if (Size <= 4)
    {
        if (IfdItem.Tag == IFD0::IFDExif) {
            Get_IFDOffset(Kind_Exif);
        }
        else if (IfdItem.Tag == IFD0::GPSInfoIFD) {
            Get_IFDOffset(Kind_GPS);
        }
        else {
            GetValueOffsetu(IfdItem);

            //Padding up, skip dummy bytes
            if (Size < 4)
                Skip_XX(4 - Size,                               "Padding");
        }
    }
    else
    {
        int32u IFDOffset;
        Get_X4 (IFDOffset,                                      "IFD offset");
        IfdItems[IFDOffset] = IfdItem;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Exif::UserComment(ZtringList& Info)
{
    //Parsing
    Ztring Value;
    int64u CharacterCode;
    Get_C8(CharacterCode,                                       "Character Code");
    auto Size = Element_Size - Element_Offset;
    switch (CharacterCode) {
        case 0x0000000000000000LL: {
            //TODO: check if ASCII
            Get_ISO_8859_1(Size, Value,                         "Value");
            break;
        }
        case 0x4153434949000000LL:
            Get_ISO_8859_1(Size, Value,                         "Value");
            break;
        case 0x554E49434F444500LL:
            if (LittleEndian)
                Get_UTF16L(Size, Value,                         "Value");
            else
                Get_UTF16B(Size, Value,                         "Value");
            break;
        default:
            Skip_XX(Size,                                       "(Unknown Character Code)");
    }
    if (!Value.empty()) {
        Info.push_back(Value);
        Element_Info1(Value);
    }
}

//---------------------------------------------------------------------------
void File_Exif::Thumbnail()
{
    Stream_Prepare(Stream_Image);

    File__Analyze* Parser = nullptr;
    const auto Infos_Thumbnail_It = Infos.find(Kind_IFD1);
    if (Infos_Thumbnail_It != Infos.end()) {
        const auto Infos_Thumbnail = Infos_Thumbnail_It->second;
        const auto Compression_It = Infos_Thumbnail.find(IFD0::Compression);
        if (Compression_It != Infos_Thumbnail.end() && Compression_It->second.size() == 1) {
            int32u Compression = Compression_It->second.at(0).To_int32u();
            switch (Compression) {
            case 6:
            case 7:
                Parser = new File_Jpeg;
                break;
            }
        }
    }

    if (Parser) {
        Open_Buffer_Init(Parser);
        Open_Buffer_Continue(Parser);
        Open_Buffer_Finalize(Parser);
        Merge(*Parser, Stream_Image, 0, 1, false);
    }
    else {
        //No parser available, skipping
        Skip_XX(Element_Size,                                   "(Not parsed)");
    }
}
    
//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Get_X2(int16u& Info, const char* Name)
{
    if (LittleEndian)
        Get_L2(Info, Name);
    else
        Get_B2(Info, Name);
}

//---------------------------------------------------------------------------
void File_Exif::Get_X4(int32u& Info, const char* Name)
{
    if (LittleEndian)
        Get_L4(Info, Name);
    else
        Get_B4(Info, Name);
}

//---------------------------------------------------------------------------
void File_Exif::Get_IFDOffset(int8u KindOfIFD)
{
    int32u IFDOffset;
    Get_X4 (IFDOffset,                                          "IFD offset");
    if (IFDOffset && (IFDOffset < 8 || IFDOffset > File_Size - File_Offset))
        IFDOffset = 0;
    if (KindOfIFD != (int8u)-1 && IFDOffset)
        IFD_Offsets[IFDOffset] = KindOfIFD;
}

//---------------------------------------------------------------------------
void File_Exif::GetValueOffsetu(ifditem &IfdItem)
{
    ZtringList& Info = Infos[currentIFD][IfdItem.Tag]; Info.clear(); Info.Separator_Set(0, __T(" /"));

    if (IfdItem.Type!=Exif_Type::ASCIIStrings && IfdItem.Type!=Exif_Type::Undefined && IfdItem.Count>=1000)
    {
        //Too many data, we don't currently need it and we skip it
        Skip_XX(Exif_Type_Size(IfdItem.Type)*IfdItem.Count,     "Data");
        return;
    }

    switch (IfdItem.Type)
    {
    case Exif_Type::UnsignedByte:                /* 8-bit unsigned integer. */
                for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
                {
                    int8u Ret8;
                    #if MEDIAINFO_TRACE
                            Get_B1 (Ret8,                       "Data"); //L1 and B1 are same
                        Element_Info1(Ztring::ToZtring(Ret8));
                    #else //MEDIAINFO_TRACE
                        if (Element_Offset+1>Element_Size)
                        {
                            Trusted_IsNot();
                            break;
                        }
                        Ret8=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset); //LittleEndian2int8u and BigEndian2int8u are same
                        Element_Offset++;
                    #endif //MEDIAINFO_TRACE
                    Info.push_back(Ztring::ToZtring(Ret8));
                }
                break;
        case Exif_Type::ASCIIStrings:                /* ASCII */
                {
                    string Data;
                    Get_String(IfdItem.Count, Data,             "Data"); Element_Info1(Data.c_str()); //TODO: multiple strings separated by NULL
                    Info.push_back(Ztring().From_UTF8(Data.c_str()));
                }
                break;
        case Exif_Type::UnsignedShort:                /* 16-bit (2-byte) unsigned integer. */
                for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
                {
                    int16u Ret16;
                    #if MEDIAINFO_TRACE
                        if (LittleEndian)
                            Get_L2 (Ret16,                      "Data");
                        else
                            Get_B2 (Ret16,                      "Data");
                        switch (IfdItem.Tag)
                        {
                            
                            default : Element_Info1(Ztring::ToZtring(Ret16));
                        }
                    #else //MEDIAINFO_TRACE
                        if (Element_Offset+2>Element_Size)
                        {
                            Trusted_IsNot();
                            break;
                        }
                        if (LittleEndian)
                            Ret16=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        else
                            Ret16=BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        Element_Offset+=2;
                    #endif //MEDIAINFO_TRACE
                    Param_Info1C((currentIFD == Kind_IFD0 || currentIFD == Kind_IFD1) && IfdItem.Tag == IFD0::Orientation, Exif_IFD0_Orientation_Name(Ret16));
                    Param_Info1C((currentIFD == Kind_IFD0 || currentIFD == Kind_IFD1) && IfdItem.Tag == IFD0::Compression, Exif_IFD0_Compression_Name(Ret16));
                    Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::LightSource, Exif_ExifIFD_Tag_LightSource_Name(Ret16));
                    Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::Flash, Exif_IFDExif_Flash_Name(Ret16));
                    Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::ColorSpace, Exif_IFDExif_ColorSpace_Name(Ret16));
                    Info.push_back(Ztring::ToZtring(Ret16));
                }
                break;

        case Exif_Type::UnsignedLong:                /* 32-bit (4-byte) unsigned integer */
                for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
                {
                    int32u Ret32;
                    #if MEDIAINFO_TRACE
                        if (LittleEndian)
                            Get_L4 (Ret32,                      "Data");
                        else
                            Get_B4 (Ret32,                      "Data");
                        Element_Info1(Ztring::ToZtring(Ret32));
                    #else //MEDIAINFO_TRACE
                        if (Element_Offset+4>Element_Size)
                        {
                            Trusted_IsNot();
                            break;
                        }
                        if (LittleEndian)
                            Ret32=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        else
                            Ret32=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        Element_Offset+=4;
                    #endif //MEDIAINFO_TRACE
                    Info.push_back(Ztring::ToZtring(Ret32));
                }
                break;

        case Exif_Type::UnsignedRational:                /* 2x32-bit (2x4-byte) unsigned integers */
                for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
                {
                    int32u N, D;
                    #if MEDIAINFO_TRACE
                        if (LittleEndian)
                        {
                            Get_L4 (N,                          "Numerator");
                            Get_L4 (D,                          "Denominator");
                        }
                        else
                        {
                            Get_B4 (N,                          "Numerator");
                            Get_B4 (D,                          "Denominator");
                        }
                        if (D)
                            Element_Info1(Ztring::ToZtring(((float64)N)/D));
                    #else //MEDIAINFO_TRACE
                        if (Element_Offset+8>Element_Size)
                        {
                            Trusted_IsNot();
                            break;
                        }
                        if (LittleEndian)
                        {
                            N=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                            D=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        }
                        else
                        {
                            N=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                            D=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        }
                        Element_Offset+=8;
                    #endif //MEDIAINFO_TRACE
                    if (D)
                        Info.push_back(Ztring::ToZtring(((float64)N)/D, D==1?0:3));
                    else
                        Info.push_back(Ztring()); // Division by zero, undefined
                }
                break;
        case Exif_Type::SignedRational:                /* 2x32-bit (2x4-byte) signed integers */
                for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
                {
                    int32u NU, DU;
                    int32s N, D;
                    #if MEDIAINFO_TRACE
                        if (LittleEndian)
                        {
                            Get_L4 (NU,                         "Numerator");
                            Get_L4 (DU,                         "Denominator");
                        }
                        else
                        {
                            Get_B4 (NU,                         "Numerator");
                            Get_B4 (DU,                         "Denominator");
                        }
                        N = (int32s)NU;
                        D = (int32s)DU;
                        if (D)
                            Element_Info1(Ztring::ToZtring(((float64)N)/D));
                    #else //MEDIAINFO_TRACE
                        if (Element_Offset+8>Element_Size)
                        {
                            Trusted_IsNot();
                            break;
                        }
                        if (LittleEndian)
                        {
                            N=LittleEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                            D=LittleEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        }
                        else
                        {
                            N=BigEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                            D=BigEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                        }
                        Element_Offset+=8;
                    #endif //MEDIAINFO_TRACE
                    if (D)
                        Info.push_back(Ztring::ToZtring(((float64)N)/D, D==1?0:3));
                    else
                        Info.push_back(Ztring()); // Division by zero, undefined
                }
                break;
        default:            //Unknown
                if (IfdItem.Tag == IFDExif::UserComment)
                    UserComment(Info);
                else
                Skip_XX(Exif_Type_Size(IfdItem.Type)*IfdItem.Count, "Data");
    }
}

//---------------------------------------------------------------------------
void File_Exif::GoToOffset(int64u GoTo_)
{
    Element_Offset = GoTo_ - (Buffer_Offset - OffsetFromContainer);
}

} //NameSpace

#endif //MEDIAINFO_EXIF_YES
