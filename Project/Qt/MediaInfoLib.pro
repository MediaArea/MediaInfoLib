# MediaInfo library built with QMake
# in order to build with qmake just copy this file to the sources
# then open with QtCreator and build
TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = mediainfo

INCLUDEPATH += \
        ../../Source \
        ../../Source/ThirdParty/tinyxml2 \
        ../../Source/ThirdParty/aes-gladman \
        ../../Source/ThirdParty/base64 \
        ../../Source/ThirdParty/md5 \
        ../../Source/ThirdParty/sha1-gladman \
        ../../Source/ThirdParty/sha2-gladman \
        ../../Source/ThirdParty/hmac-gladman \
        ../../../ZenLib/Source

# Note: UNICODE is a must
QMAKE_CXXFLAGS +=  -D_UNICODE -DMEDIAINFO_LIBMMS_NO -DMEDIAINFO_LIBCURL_NO -DMEDIAINFO_GRAPHVIZ_NO

# surpressing way too many warnings here, with a heavy sigh
# these should be looked at one group at a time
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall

win32 {
# ATTENTION! Change line bellow according local machine configuration for zlib path location.
INCLUDEPATH += ../../../zlib/include
}

!win32-msvc* {
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter \
-Wno-ignored-qualifiers \
-Wno-missing-braces \
-Wno-parentheses
}


HEADERS += \
        ../../Source/MediaInfo/Archive/File_7z.h \
        ../../Source/MediaInfo/Archive/File_Ace.h \
        ../../Source/MediaInfo/Archive/File_Bzip2.h \
        ../../Source/MediaInfo/Archive/File_Elf.h \
        ../../Source/MediaInfo/Archive/File_Gzip.h \
        ../../Source/MediaInfo/Archive/File_Iso9660.h \
        ../../Source/MediaInfo/Archive/File_Mz.h \
        ../../Source/MediaInfo/Archive/File_Rar.h \
        ../../Source/MediaInfo/Archive/File_Tar.h \
        ../../Source/MediaInfo/Archive/File_Zip.h \
        ../../Source/MediaInfo/Audio/File_Aac.h \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio.h \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.h \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr_Ps.h \
        ../../Source/MediaInfo/Audio/File_Ac3.h \
        ../../Source/MediaInfo/Audio/File_Ac4.h \
        ../../Source/MediaInfo/Audio/File_Adpcm.h \
        ../../Source/MediaInfo/Audio/File_Als.h \
        ../../Source/MediaInfo/Audio/File_Amr.h \
        ../../Source/MediaInfo/Audio/File_Amv.h \
        ../../Source/MediaInfo/Audio/File_Ape.h \
        ../../Source/MediaInfo/Audio/File_Aptx100.h \
        ../../Source/MediaInfo/Audio/File_Au.h \
        ../../Source/MediaInfo/Audio/File_Caf.h \
        ../../Source/MediaInfo/Audio/File_Celt.h \
        ../../Source/MediaInfo/Audio/File_ChannelGrouping.h \
        ../../Source/MediaInfo/Audio/File_Dat.h \
        ../../Source/MediaInfo/Audio/File_DolbyE.h \
        ../../Source/MediaInfo/Audio/File_Dts.h \
        ../../Source/MediaInfo/Audio/File_DtsUhd.h \
        ../../Source/MediaInfo/Audio/File_ExtendedModule.h \
        ../../Source/MediaInfo/Audio/File_Flac.h \
        ../../Source/MediaInfo/Audio/File_ImpulseTracker.h \
        ../../Source/MediaInfo/Audio/File_La.h \
        ../../Source/MediaInfo/Audio/File_Mga.h \
        ../../Source/MediaInfo/Audio/File_Midi.h \
        ../../Source/MediaInfo/Audio/File_Module.h \
        ../../Source/MediaInfo/Audio/File_Mpc.h \
        ../../Source/MediaInfo/Audio/File_MpcSv8.h \
        ../../Source/MediaInfo/Audio/File_Mpega.h \
        ../../Source/MediaInfo/Audio/File_OpenMG.h \
        ../../Source/MediaInfo/Audio/File_Opus.h \
        ../../Source/MediaInfo/Audio/File_Pcm.h \
        ../../Source/MediaInfo/Audio/File_Pcm_M2ts.h \
        ../../Source/MediaInfo/Audio/File_Pcm_Vob.h \
        ../../Source/MediaInfo/Audio/File_Ps2Audio.h \
        ../../Source/MediaInfo/Audio/File_Rkau.h \
        ../../Source/MediaInfo/Audio/File_ScreamTracker3.h \
        ../../Source/MediaInfo/Audio/File_SmpteSt0302.h \
        ../../Source/MediaInfo/Audio/File_SmpteSt0331.h \
        ../../Source/MediaInfo/Audio/File_SmpteSt0337.h \
        ../../Source/MediaInfo/Audio/File_Speex.h \
        ../../Source/MediaInfo/Audio/File_Tak.h \
        ../../Source/MediaInfo/Audio/File_Tta.h \
        ../../Source/MediaInfo/Audio/File_TwinVQ.h \
        ../../Source/MediaInfo/Audio/File_Usac.h \
        ../../Source/MediaInfo/Audio/File_Vorbis.h \
        ../../Source/MediaInfo/Audio/File_Wvpk.h \
        ../../Source/MediaInfo/Duplicate/File__Duplicate__Base.h \
        ../../Source/MediaInfo/Duplicate/File__Duplicate__Writer.h \
        ../../Source/MediaInfo/Duplicate/File__Duplicate_MpegTs.h \
        ../../Source/MediaInfo/Export/Export_EbuCore.h \
        ../../Source/MediaInfo/Export/Export_Niso.h \
        ../../Source/MediaInfo/Export/Export_Graph.h \
        ../../Source/MediaInfo/Export/Export_Graph_gvc_Include.h \
        ../../Source/MediaInfo/Export/Export_Fims.h \
        ../../Source/MediaInfo/Export/Export_Mpeg7.h \
        ../../Source/MediaInfo/Export/Export_PBCore.h \
        ../../Source/MediaInfo/Export/Export_PBCore2.h \
        ../../Source/MediaInfo/Export/Export_reVTMD.h \
        ../../Source/MediaInfo/File__Analyse_Automatic.h \
        ../../Source/MediaInfo/File__Analyze.h \
        ../../Source/MediaInfo/File__Analyze_Element.h \
        ../../Source/MediaInfo/File__Analyze_MinimizeSize.h \
        ../../Source/MediaInfo/File__Base.h \
        ../../Source/MediaInfo/File__Duplicate.h \
        ../../Source/MediaInfo/File__MultipleParsing.h \
        ../../Source/MediaInfo/File_Dummy.h \
        ../../Source/MediaInfo/File_Other.h \
        ../../Source/MediaInfo/File_Unknown.h \
        ../../Source/MediaInfo/HashWrapper.h \
        ../../Source/MediaInfo/Image/File_ArriRaw.h \
        ../../Source/MediaInfo/Image/File_Bmp.h \
        ../../Source/MediaInfo/Image/File_Bpg.h \
        ../../Source/MediaInfo/Image/File_Dds.h \
        ../../Source/MediaInfo/Image/File_Dpx.h \
        ../../Source/MediaInfo/Image/File_Exr.h \
        ../../Source/MediaInfo/Image/File_Gif.h \
        ../../Source/MediaInfo/Image/File_Ico.h \
        ../../Source/MediaInfo/Image/File_Jpeg.h \
        ../../Source/MediaInfo/Image/File_Pcx.h \
        ../../Source/MediaInfo/Image/File_Png.h \
        ../../Source/MediaInfo/Image/File_Psd.h \
        ../../Source/MediaInfo/Image/File_Rle.h \
        ../../Source/MediaInfo/Image/File_Tga.h \
        ../../Source/MediaInfo/Image/File_Tiff.h \
        ../../Source/MediaInfo/MediaInfo.h \
        ../../Source/MediaInfo/MediaInfo_Config.h \
        ../../Source/MediaInfo/MediaInfo_Config_MediaInfo.h \
        ../../Source/MediaInfo/MediaInfo_Config_PerPackage.h \
        ../../Source/MediaInfo/MediaInfo_Const.h \
        ../../Source/MediaInfo/MediaInfo_Events.h \
        ../../Source/MediaInfo/MediaInfo_Events_Internal.h \
        ../../Source/MediaInfo/MediaInfo_Internal.h \
        ../../Source/MediaInfo/MediaInfo_Internal_Const.h \
        ../../Source/MediaInfo/MediaInfoList.h \
        ../../Source/MediaInfo/MediaInfoList_Internal.h \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper.h \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper_Resource.h \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence.h \
        ../../Source/MediaInfo/Multiple/File_Aaf.h \
        ../../Source/MediaInfo/Multiple/File_Ancillary.h \
        ../../Source/MediaInfo/Multiple/File_Bdmv.h \
        ../../Source/MediaInfo/Multiple/File_Cdxa.h \
        ../../Source/MediaInfo/Multiple/File_DashMpd.h \
        ../../Source/MediaInfo/Multiple/File_DcpAm.h \
        ../../Source/MediaInfo/Multiple/File_DcpCpl.h \
        ../../Source/MediaInfo/Multiple/File_DcpPkl.h \
        ../../Source/MediaInfo/Multiple/File_Dpg.h \
        ../../Source/MediaInfo/Multiple/File_DvDif.h \
        ../../Source/MediaInfo/Multiple/File_Dvdv.h \
        ../../Source/MediaInfo/Multiple/File_Dxw.h \
        ../../Source/MediaInfo/Multiple/File_Flv.h \
        ../../Source/MediaInfo/Multiple/File_Gxf.h \
        ../../Source/MediaInfo/Multiple/File_Gxf_TimeCode.h \
        ../../Source/MediaInfo/Multiple/File_HdsF4m.h \
        ../../Source/MediaInfo/Multiple/File_Hls.h \
        ../../Source/MediaInfo/Multiple/File_Ibi.h \
        ../../Source/MediaInfo/Multiple/File_Ibi_Creation.h \
        ../../Source/MediaInfo/Multiple/File_Ism.h \
        ../../Source/MediaInfo/Multiple/File_Ivf.h \
        ../../Source/MediaInfo/Multiple/File_Lxf.h \
        ../../Source/MediaInfo/Multiple/File_Mk.h \
        ../../Source/MediaInfo/Multiple/File_Mpeg4.h \
        ../../Source/MediaInfo/Multiple/File_Mpeg4_Descriptors.h \
        ../../Source/MediaInfo/Multiple/File_Mpeg4_TimeCode.h \
        ../../Source/MediaInfo/Multiple/File_Mpeg_Descriptors.h \
        ../../Source/MediaInfo/Multiple/File_Mpeg_Psi.h \
        ../../Source/MediaInfo/Multiple/File_MpegPs.h \
        ../../Source/MediaInfo/Multiple/File_MpegTs.h \
        ../../Source/MediaInfo/Multiple/File_Mxf.h \
        ../../Source/MediaInfo/Multiple/File_Mxf_Automated.h \
        ../../Source/MediaInfo/Multiple/File_Nut.h \
        ../../Source/MediaInfo/Multiple/File_Ogg.h \
        ../../Source/MediaInfo/Multiple/File_Ogg_SubElement.h \
        ../../Source/MediaInfo/Multiple/File_P2_Clip.h \
        ../../Source/MediaInfo/Multiple/File_Pmp.h \
        ../../Source/MediaInfo/Multiple/File_Ptx.h \
        ../../Source/MediaInfo/Multiple/File_Riff.h \
        ../../Source/MediaInfo/Multiple/File_Rm.h \
        ../../Source/MediaInfo/Multiple/File_SequenceInfo.h \
        ../../Source/MediaInfo/Multiple/File_Skm.h \
        ../../Source/MediaInfo/Multiple/File_Swf.h \
        ../../Source/MediaInfo/Multiple/File_Umf.h \
        ../../Source/MediaInfo/Multiple/File_Vbi.h \
        ../../Source/MediaInfo/Multiple/File_Wm.h \
        ../../Source/MediaInfo/Multiple/File_Wtv.h \
        ../../Source/MediaInfo/Multiple/File_Xdcam_Clip.h \
        ../../Source/MediaInfo/PreComp.h \
        ../../Source/MediaInfo/Reader/Reader__Base.h \
        ../../Source/MediaInfo/Reader/Reader_Directory.h \
        ../../Source/MediaInfo/Reader/Reader_File.h \
        ../../Source/MediaInfo/Reader/Reader_libcurl.h \
        ../../Source/MediaInfo/Reader/Reader_libcurl_Include.h \
        ../../Source/MediaInfo/Reader/Reader_libmms.h \
        ../../Source/MediaInfo/Setup.h \
        ../../Source/MediaInfo/Tag/File__Tags.h \
        ../../Source/MediaInfo/Tag/File_ApeTag.h \
        ../../Source/MediaInfo/Tag/File_Id3.h \
        ../../Source/MediaInfo/Tag/File_Id3v2.h \
        ../../Source/MediaInfo/Tag/File_Lyrics3.h \
        ../../Source/MediaInfo/Tag/File_Lyrics3v2.h \
        ../../Source/MediaInfo/Tag/File_PropertyList.h \
        ../../Source/MediaInfo/Tag/File_VorbisCom.h \
        ../../Source/MediaInfo/Tag/File_Xmp.h \
        ../../Source/MediaInfo/Text/File_AribStdB24B37.h \
        ../../Source/MediaInfo/Text/File_Cdp.h \
        ../../Source/MediaInfo/Text/File_Cmml.h \
        ../../Source/MediaInfo/Text/File_DtvccTransport.h \
        ../../Source/MediaInfo/Text/File_DvbSubtitle.h \
        ../../Source/MediaInfo/Text/File_Eia608.h \
        ../../Source/MediaInfo/Text/File_Eia708.h \
        ../../Source/MediaInfo/Text/File_Kate.h \
        ../../Source/MediaInfo/Text/File_N19.h \
        ../../Source/MediaInfo/Text/File_OtherText.h \
        ../../Source/MediaInfo/Text/File_Pac.h \
        ../../Source/MediaInfo/Text/File_Pac_Codepages.h \
        ../../Source/MediaInfo/Text/File_Pdf.h \
        ../../Source/MediaInfo/Text/File_Pgs.h \
        ../../Source/MediaInfo/Text/File_Scc.h \
        ../../Source/MediaInfo/Text/File_Scte20.h \
        ../../Source/MediaInfo/Text/File_Sdp.h \
        ../../Source/MediaInfo/Text/File_SubRip.h \
        ../../Source/MediaInfo/Text/File_Teletext.h \
        ../../Source/MediaInfo/Text/File_TimedText.h \
        ../../Source/MediaInfo/Text/File_Ttml.h \
        ../../Source/MediaInfo/TimeCode.h \
        ../../Source/MediaInfo/Video/File_AfdBarData.h \
        ../../Source/MediaInfo/Video/File_Aic.h \
        ../../Source/MediaInfo/Video/File_Avc.h \
        ../../Source/MediaInfo/Video/File_AvsV.h \
        ../../Source/MediaInfo/Video/File_Avs3V.h \
        ../../Source/MediaInfo/Video/File_Canopus.h \
        ../../Source/MediaInfo/Video/File_Dirac.h \
        ../../Source/MediaInfo/Video/File_Ffv1.h \
        ../../Source/MediaInfo/Video/File_Flic.h \
        ../../Source/MediaInfo/Video/File_Fraps.h \
        ../../Source/MediaInfo/Video/File_H263.h \
        ../../Source/MediaInfo/Video/File_HdrVividMetadata.h \
        ../../Source/MediaInfo/Video/File_Hevc.h \
        ../../Source/MediaInfo/Video/File_HuffYuv.h \
        ../../Source/MediaInfo/Video/File_Lagarith.h \
        ../../Source/MediaInfo/Video/File_Mpeg4v.h \
        ../../Source/MediaInfo/Video/File_Mpegv.h \
        ../../Source/MediaInfo/Video/File_ProRes.h \
        ../../Source/MediaInfo/Video/File_Theora.h \
        ../../Source/MediaInfo/Video/File_Vc1.h \
        ../../Source/MediaInfo/Video/File_Vc3.h \
        ../../Source/MediaInfo/Video/File_Vp8.h \
        ../../Source/MediaInfo/Video/File_Vp9.h \
        ../../Source/MediaInfo/Video/File_Vvc.h \
        ../../Source/MediaInfo/Video/File_Y4m.h \
        ../../Source/MediaInfo/XmlUtils.h \
        ../../Source/MediaInfo/OutputHelpers.h \
        ../../Source/MediaInfo/ExternalCommandHelpers.h

SOURCES += \
        ../../Source/MediaInfo/Archive/File_7z.cpp \
        ../../Source/MediaInfo/Archive/File_Ace.cpp \
        ../../Source/MediaInfo/Archive/File_Bzip2.cpp \
        ../../Source/MediaInfo/Archive/File_Elf.cpp \
        ../../Source/MediaInfo/Archive/File_Gzip.cpp \
        ../../Source/MediaInfo/Archive/File_Iso9660.cpp \
        ../../Source/MediaInfo/Archive/File_Mz.cpp \
        ../../Source/MediaInfo/Archive/File_Rar.cpp \
        ../../Source/MediaInfo/Archive/File_Tar.cpp \
        ../../Source/MediaInfo/Archive/File_Zip.cpp \
        ../../Source/MediaInfo/Audio/File_Aac.cpp \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio.cpp \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.cpp \
        ../../Source/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr_Ps.cpp \
        ../../Source/MediaInfo/Audio/File_Aac_Main.cpp \
        ../../Source/MediaInfo/Audio/File_Aac_Others.cpp \
        ../../Source/MediaInfo/Audio/File_Ac3.cpp \
        ../../Source/MediaInfo/Audio/File_Ac4.cpp \
        ../../Source/MediaInfo/Audio/File_Adm.cpp \
        ../../Source/MediaInfo/Audio/File_Adpcm.cpp \
        ../../Source/MediaInfo/Audio/File_Als.cpp \
        ../../Source/MediaInfo/Audio/File_Amr.cpp \
        ../../Source/MediaInfo/Audio/File_Amv.cpp \
        ../../Source/MediaInfo/Audio/File_Ape.cpp \
        ../../Source/MediaInfo/Audio/File_Aptx100.cpp \
        ../../Source/MediaInfo/Audio/File_Au.cpp \
        ../../Source/MediaInfo/Audio/File_Caf.cpp \
        ../../Source/MediaInfo/Audio/File_Celt.cpp \
        ../../Source/MediaInfo/Audio/File_ChannelGrouping.cpp \
        ../../Source/MediaInfo/Audio/File_ChannelSplitting.cpp \
        ../../Source/MediaInfo/Audio/File_Dat.cpp \
        ../../Source/MediaInfo/Audio/File_Dsdiff.cpp \
        ../../Source/MediaInfo/Audio/File_Dsf.cpp \
        ../../Source/MediaInfo/Audio/File_Dts.cpp \
        ../../Source/MediaInfo/Audio/File_DtsUhd.cpp \
        ../../Source/MediaInfo/Audio/File_DolbyAudioMetadata.cpp \
        ../../Source/MediaInfo/Audio/File_DolbyE.cpp \
        ../../Source/MediaInfo/Audio/File_ExtendedModule.cpp \
        ../../Source/MediaInfo/Audio/File_Flac.cpp \
        ../../Source/MediaInfo/Audio/File_Iab.cpp \
        ../../Source/MediaInfo/Audio/File_ImpulseTracker.cpp \
        ../../Source/MediaInfo/Audio/File_La.cpp \
        ../../Source/MediaInfo/Audio/File_Mga.cpp \
        ../../Source/MediaInfo/Audio/File_Midi.cpp \
        ../../Source/MediaInfo/Audio/File_Module.cpp \
        ../../Source/MediaInfo/Audio/File_Mpc.cpp \
        ../../Source/MediaInfo/Audio/File_MpcSv8.cpp \
        ../../Source/MediaInfo/Audio/File_Mpega.cpp \
        ../../Source/MediaInfo/Audio/File_Mpegh3da.cpp \
        ../../Source/MediaInfo/Audio/File_OpenMG.cpp \
        ../../Source/MediaInfo/Audio/File_Opus.cpp \
        ../../Source/MediaInfo/Audio/File_Pcm.cpp \
        ../../Source/MediaInfo/Audio/File_Pcm_M2ts.cpp \
        ../../Source/MediaInfo/Audio/File_Pcm_Vob.cpp \
        ../../Source/MediaInfo/Audio/File_Ps2Audio.cpp \
        ../../Source/MediaInfo/Audio/File_Rkau.cpp \
        ../../Source/MediaInfo/Audio/File_ScreamTracker3.cpp \
        ../../Source/MediaInfo/Audio/File_SmpteSt0302.cpp \
        ../../Source/MediaInfo/Audio/File_SmpteSt0331.cpp \
        ../../Source/MediaInfo/Audio/File_SmpteSt0337.cpp \
        ../../Source/MediaInfo/Audio/File_Speex.cpp \
        ../../Source/MediaInfo/Audio/File_Tak.cpp \
        ../../Source/MediaInfo/Audio/File_Tta.cpp \
        ../../Source/MediaInfo/Audio/File_TwinVQ.cpp \
        ../../Source/MediaInfo/Audio/File_Usac.cpp \
        ../../Source/MediaInfo/Audio/File_Vorbis.cpp \
        ../../Source/MediaInfo/Audio/File_Wvpk.cpp \
        ../../Source/MediaInfo/Duplicate/File__Duplicate__Base.cpp \
        ../../Source/MediaInfo/Duplicate/File__Duplicate__Writer.cpp \
        ../../Source/MediaInfo/Duplicate/File__Duplicate_MpegTs.cpp \
        ../../Source/MediaInfo/Export/Export_EbuCore.cpp \
        ../../Source/MediaInfo/Export/Export_Niso.cpp \
        ../../Source/MediaInfo/Export/Export_Graph.cpp \
        ../../Source/MediaInfo/Export/Export_Fims.cpp \
        ../../Source/MediaInfo/Export/Export_Mpeg7.cpp \
        ../../Source/MediaInfo/Export/Export_PBCore.cpp \
        ../../Source/MediaInfo/Export/Export_PBCore2.cpp \
        ../../Source/MediaInfo/Export/Export_reVTMD.cpp \
        ../../Source/MediaInfo/File__Analyze.cpp \
        ../../Source/MediaInfo/File__Analyze_Buffer.cpp \
        ../../Source/MediaInfo/File__Analyze_Buffer_MinimizeSize.cpp \
        ../../Source/MediaInfo/File__Analyze_Element.cpp \
        ../../Source/MediaInfo/File__Analyze_Streams.cpp \
        ../../Source/MediaInfo/File__Analyze_Streams_Finish.cpp \
        ../../Source/MediaInfo/File__Base.cpp \
        ../../Source/MediaInfo/File__Duplicate.cpp \
        ../../Source/MediaInfo/File__MultipleParsing.cpp \
        ../../Source/MediaInfo/File_Dummy.cpp \
        ../../Source/MediaInfo/File_Other.cpp \
        ../../Source/MediaInfo/File_Unknown.cpp \
        ../../Source/MediaInfo/HashWrapper.cpp \
        ../../Source/MediaInfo/Image/File_ArriRaw.cpp \
        ../../Source/MediaInfo/Image/File_Bmp.cpp \
        ../../Source/MediaInfo/Image/File_Bpg.cpp \
        ../../Source/MediaInfo/Image/File_Dds.cpp \
        ../../Source/MediaInfo/Image/File_Dpx.cpp \
        ../../Source/MediaInfo/Image/File_Exr.cpp \
        ../../Source/MediaInfo/Image/File_Gif.cpp \
        ../../Source/MediaInfo/Image/File_Ico.cpp \
        ../../Source/MediaInfo/Image/File_Jpeg.cpp \
        ../../Source/MediaInfo/Image/File_Pcx.cpp \
        ../../Source/MediaInfo/Image/File_Png.cpp \
        ../../Source/MediaInfo/Image/File_Psd.cpp \
        ../../Source/MediaInfo/Image/File_Rle.cpp \
        ../../Source/MediaInfo/Image/File_Tiff.cpp \
        ../../Source/MediaInfo/Image/File_Tga.cpp \
        ../../Source/MediaInfo/Image/File_WebP.cpp \
        ../../Source/MediaInfo/MediaInfo.cpp \
        ../../Source/MediaInfo/MediaInfo_Config.cpp \
        ../../Source/MediaInfo/MediaInfo_Config_Automatic.cpp \
        ../../Source/MediaInfo/MediaInfo_Config_MediaInfo.cpp \
        ../../Source/MediaInfo/MediaInfo_Config_PerPackage.cpp \
        ../../Source/MediaInfo/MediaInfo_File.cpp \
        ../../Source/MediaInfo/MediaInfo_Inform.cpp \
        ../../Source/MediaInfo/MediaInfo_Internal.cpp \
        ../../Source/MediaInfo/MediaInfoList.cpp \
        ../../Source/MediaInfo/MediaInfoList_Internal.cpp \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper.cpp \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper_Resource.cpp \
        ../../Source/MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence.cpp \
        ../../Source/MediaInfo/Multiple/File_Aaf.cpp \
        ../../Source/MediaInfo/Multiple/File_Ancillary.cpp \
        ../../Source/MediaInfo/Multiple/File_Bdmv.cpp \
        ../../Source/MediaInfo/Multiple/File_Cdxa.cpp \
        ../../Source/MediaInfo/Multiple/File_DashMpd.cpp \
        ../../Source/MediaInfo/Multiple/File_DcpAm.cpp \
        ../../Source/MediaInfo/Multiple/File_DcpCpl.cpp \
        ../../Source/MediaInfo/Multiple/File_DcpPkl.cpp \
        ../../Source/MediaInfo/Multiple/File_Dpg.cpp \
        ../../Source/MediaInfo/Multiple/File_DvDif.cpp \
        ../../Source/MediaInfo/Multiple/File_DvDif_Analysis.cpp \
        ../../Source/MediaInfo/Multiple/File_Dvdv.cpp \
        ../../Source/MediaInfo/Multiple/File_Dxw.cpp \
        ../../Source/MediaInfo/Multiple/File_Flv.cpp \
        ../../Source/MediaInfo/Multiple/File_Gxf.cpp \
        ../../Source/MediaInfo/Multiple/File_Gxf_TimeCode.cpp \
        ../../Source/MediaInfo/Multiple/File_HdsF4m.cpp \
        ../../Source/MediaInfo/Multiple/File_Hls.cpp \
        ../../Source/MediaInfo/Multiple/File_Ibi.cpp \
        ../../Source/MediaInfo/Multiple/File_Ibi_Creation.cpp \
        ../../Source/MediaInfo/Multiple/File_Ism.cpp \
        ../../Source/MediaInfo/Multiple/File_Ivf.cpp \
        ../../Source/MediaInfo/Multiple/File_Lxf.cpp \
        ../../Source/MediaInfo/Multiple/File_Mk.cpp \
        ../../Source/MediaInfo/Multiple/File_MiXml.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg4.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg4_Descriptors.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg4_Elements.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg4_TimeCode.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg_Descriptors.cpp \
        ../../Source/MediaInfo/Multiple/File_Mpeg_Psi.cpp \
        ../../Source/MediaInfo/Multiple/File_MpegPs.cpp \
        ../../Source/MediaInfo/Multiple/File_MpegTs.cpp \
        ../../Source/MediaInfo/Multiple/File_MpegTs_Duplicate.cpp \
        ../../Source/MediaInfo/Multiple/File_Mxf.cpp \
        ../../Source/MediaInfo/Multiple/File_Nsv.cpp \
        ../../Source/MediaInfo/Multiple/File_Nut.cpp \
        ../../Source/MediaInfo/Multiple/File_Ogg.cpp \
        ../../Source/MediaInfo/Multiple/File_Ogg_SubElement.cpp \
        ../../Source/MediaInfo/Multiple/File_P2_Clip.cpp \
        ../../Source/MediaInfo/Multiple/File_Pmp.cpp \
        ../../Source/MediaInfo/Multiple/File_Ptx.cpp \
        ../../Source/MediaInfo/Multiple/File_Riff.cpp \
        ../../Source/MediaInfo/Multiple/File_Riff_Elements.cpp \
        ../../Source/MediaInfo/Multiple/File_Rm.cpp \
        ../../Source/MediaInfo/Multiple/File_SequenceInfo.cpp \
        ../../Source/MediaInfo/Multiple/File_Skm.cpp \
        ../../Source/MediaInfo/Multiple/File_Swf.cpp \
        ../../Source/MediaInfo/Multiple/File_Umf.cpp \
        ../../Source/MediaInfo/Multiple/File_Vbi.cpp \
        ../../Source/MediaInfo/Multiple/File_Wm.cpp \
        ../../Source/MediaInfo/Multiple/File_Wm_Elements.cpp \
        ../../Source/MediaInfo/Multiple/File_Wtv.cpp \
        ../../Source/MediaInfo/Multiple/File_Xdcam_Clip.cpp \
        ../../Source/MediaInfo/PreComp.cpp \
        ../../Source/MediaInfo/Reader/Reader_Directory.cpp \
        ../../Source/MediaInfo/Reader/Reader_File.cpp \
        ../../Source/MediaInfo/Reader/Reader_libcurl.cpp \
        ../../Source/MediaInfo/Reader/Reader_libmms.cpp \
        ../../Source/MediaInfo/Tag/File__Tags.cpp \
        ../../Source/MediaInfo/Tag/File_ApeTag.cpp \
        ../../Source/MediaInfo/Tag/File_Icc.cpp \
        ../../Source/MediaInfo/Tag/File_Id3.cpp \
        ../../Source/MediaInfo/Tag/File_Id3v2.cpp \
        ../../Source/MediaInfo/Tag/File_Lyrics3.cpp \
        ../../Source/MediaInfo/Tag/File_Lyrics3v2.cpp \
        ../../Source/MediaInfo/Tag/File_PropertyList.cpp \
        ../../Source/MediaInfo/Tag/File_VorbisCom.cpp \
        ../../Source/MediaInfo/Tag/File_Xmp.cpp \
        ../../Source/MediaInfo/Text/File_AribStdB24B37.cpp \
        ../../Source/MediaInfo/Text/File_Cdp.cpp \
        ../../Source/MediaInfo/Text/File_Cmml.cpp \
        ../../Source/MediaInfo/Text/File_DtvccTransport.cpp \
        ../../Source/MediaInfo/Text/File_DvbSubtitle.cpp \
        ../../Source/MediaInfo/Text/File_Eia608.cpp \
        ../../Source/MediaInfo/Text/File_Eia708.cpp \
        ../../Source/MediaInfo/Text/File_Kate.cpp \
        ../../Source/MediaInfo/Text/File_N19.cpp \
        ../../Source/MediaInfo/Text/File_OtherText.cpp \
        ../../Source/MediaInfo/Text/File_Pac.cpp \
        ../../Source/MediaInfo/Text/File_Pdf.cpp \
        ../../Source/MediaInfo/Text/File_Pgs.cpp \
        ../../Source/MediaInfo/Text/File_Scc.cpp \
        ../../Source/MediaInfo/Text/File_Scte20.cpp \
        ../../Source/MediaInfo/Text/File_Sdp.cpp \
        ../../Source/MediaInfo/Text/File_SubRip.cpp \
        ../../Source/MediaInfo/Text/File_Teletext.cpp \
        ../../Source/MediaInfo/Text/File_TimedText.cpp \
        ../../Source/MediaInfo/Text/File_Ttml.cpp \
        ../../Source/MediaInfo/TimeCode.cpp \
        ../../Source/MediaInfo/Video/File_AfdBarData.cpp \
        ../../Source/MediaInfo/Video/File_Aic.cpp \
        ../../Source/MediaInfo/Video/File_Avc.cpp \
        ../../Source/MediaInfo/Video/File_Avc_Duplicate.cpp \
        ../../Source/MediaInfo/Video/File_AvsV.cpp \
        ../../Source/MediaInfo/Video/File_Avs3V.cpp \
        ../../Source/MediaInfo/Video/File_Canopus.cpp \
        ../../Source/MediaInfo/Video/File_CineForm.cpp \
        ../../Source/MediaInfo/Video/File_Dirac.cpp \
        ../../Source/MediaInfo/Video/File_DolbyVisionMetadata.cpp \
        ../../Source/MediaInfo/Video/File_Ffv1.cpp \
        ../../Source/MediaInfo/Video/File_Flic.cpp \
        ../../Source/MediaInfo/Video/File_Fraps.cpp \
        ../../Source/MediaInfo/Video/File_H263.cpp \
        ../../Source/MediaInfo/Video/File_HdrVividMetadata.cpp \
        ../../Source/MediaInfo/Video/File_Hevc.cpp \
        ../../Source/MediaInfo/Video/File_HuffYuv.cpp \
        ../../Source/MediaInfo/Video/File_Lagarith.cpp \
        ../../Source/MediaInfo/Video/File_Mpeg4v.cpp \
        ../../Source/MediaInfo/Video/File_Mpegv.cpp \
        ../../Source/MediaInfo/Video/File_ProRes.cpp \
        ../../Source/MediaInfo/Video/File_Theora.cpp \
        ../../Source/MediaInfo/Video/File_Vc1.cpp \
        ../../Source/MediaInfo/Video/File_Vc3.cpp \
        ../../Source/MediaInfo/Video/File_Vp8.cpp \
        ../../Source/MediaInfo/Video/File_Vp9.cpp \
        ../../Source/MediaInfo/Video/File_Vvc.cpp \
        ../../Source/MediaInfo/Video/File_Y4m.cpp \
        ../../Source/MediaInfo/XmlUtils.cpp \
        ../../Source/MediaInfo/OutputHelpers.cpp \
        ../../Source/MediaInfo/ExternalCommandHelpers.cpp

SOURCES += \
        ../../Source/ThirdParty/aes-gladman/aes_modes.c \
        ../../Source/ThirdParty/aes-gladman/aes_ni.c \
        ../../Source/ThirdParty/aes-gladman/aescrypt.c \
        ../../Source/ThirdParty/aes-gladman/aeskey.c \
        ../../Source/ThirdParty/aes-gladman/aestab.c \
        ../../Source/ThirdParty/hmac-gladman/hmac.c \
        ../../Source/ThirdParty/md5/md5.c \
        ../../Source/ThirdParty/sha1-gladman/sha1.c \
        ../../Source/ThirdParty/sha2-gladman/sha2.c \
        ../../Source/ThirdParty/tinyxml2/tinyxml2.cpp

HEADERS += \
        ../../Source/ThirdParty/aes-gladman/aes.h \
        ../../Source/ThirdParty/aes-gladman/aes_ni.h \
        ../../Source/ThirdParty/aes-gladman/aes_via_ace.h \
        ../../Source/ThirdParty/aes-gladman/aescpp.h \
        ../../Source/ThirdParty/aes-gladman/aesopt.h \
        ../../Source/ThirdParty/aes-gladman/aestab.h \
        ../../Source/ThirdParty/aes-gladman/brg_endian.h \
        ../../Source/ThirdParty/aes-gladman/brg_types.h \
        ../../Source/ThirdParty/base64/base64.h \
        ../../Source/ThirdParty/hmac-gladman/hmac.h \
        ../../Source/ThirdParty/md5/md5.h \
        ../../Source/ThirdParty/sha1-gladman/sha1.h \
        ../../Source/ThirdParty/sha2-gladman/sha2.h \
        ../../Source/ThirdParty/tinyxml2/tinyxml2.h

