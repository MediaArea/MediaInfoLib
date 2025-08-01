cmake_minimum_required(VERSION 3.5.0)

project(MediaInfoLib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set(MediaInfoLib_MAJOR_VERSION 25)
set(MediaInfoLib_MINOR_VERSION 07)
set(MediaInfoLib_PATCH_VERSION 0)
set(MediaInfoLib_VERSION ${MediaInfoLib_MAJOR_VERSION}.${MediaInfoLib_MINOR_VERSION})

if (MediaInfoLib_PATCH_VERSION)
  set(MediaInfoLib_VERSION ${MediaInfoLib_VERSION}.${MediaInfoLib_PATCH_VERSION})
endif()

set(MediaInfoLib_SOVERSION 0)

if (NOT WIN32)
  option(BUILD_ZLIB "Build bundled zlib" OFF)
  option(BUILD_ZENLIB "Build bundled ZenLib" OFF)
else()
  option(BUILD_ZLIB "Build bundled zlib" ON)
  option(BUILD_ZENLIB "Build bundled ZenLib" ON)
endif()

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE STRING "Directory where binary will install")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where library will install")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The directory the headers are installed in")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

if(NOT TARGET uninstall)
    add_custom_target(uninstall
   "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

if (NOT BUILD_ZENLIB)
  find_package(ZenLib REQUIRED)
else()
  set(_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../ZenLib/Project/CMake ${CMAKE_CURRENT_BINARY_DIR}/ZenLib)
  set(BUILD_SHARED_LIBS ${_BUILD_SHARED_LIBS})
  set(ZenLib_LIBRARY zen)
  unset(_BUILD_SHARED_LIBS)
  if(UNIX AND BUILD_SHARED_LIBS)
    target_compile_options(zen PRIVATE -fPIC)
  endif()
endif()

if (NOT BUILD_ZLIB)
  find_package(ZLIB REQUIRED)
else()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
  if(NOT BUILD_SHARED_LIBS)
    set(ZLIB_LIBRARIES "${LIB_INSTALL_DIR}/$<TARGET_LINKER_FILE_NAME:zlibstatic>")
  endif()
  if(UNIX AND BUILD_SHARED_LIBS)
    target_compile_options(zlibstatic PRIVATE -fPIC)
  endif()
  set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
endif()

if(VCPKG_TARGET_TRIPLET)
  set(MediaInfoLib_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/mediainfolib")
elseif(MSVC)
  set(MediaInfoLib_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/cmake")
else()
  set(MediaInfoLib_CONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/mediainfolib")
endif()

set(MediaInfoLib_SOURCES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../Source)

# On Windows debug library should have 'd' postfix.
if(WIN32)
  set(CMAKE_DEBUG_POSTFIX "d")
elseif(APPLE)
  set(CMAKE_DEBUG_POSTFIX "_debug")
endif()

# use bundled tinyxml only if no system
find_package(TinyXML)

# use system curl if is present
find_package(CURL)

include_directories(${MediaInfoLib_SOURCES_PATH})
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/md5/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/base64/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha1-gladman/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha2-gladman/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/hmac-gladman/)
include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/)
include_directories(${ZLIB_INCLUDE_DIRS})

set(MediaInfoLib_PUBLIC_HDRS
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo.h
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfoList.h
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Const.h
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Events.h
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoDLL.h
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoDLL_Static.h
)

set(MediaInfoLib_SRCS
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/md5/md5.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/aes_modes.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/aes_ni.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/aescrypt.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/aeskey.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/aestab.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha1-gladman/sha1.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha2-gladman/sha2.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/hmac-gladman/hmac.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/tfsxml/tfsxml.c
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/fmt/format.cc
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze_Buffer.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze_Buffer_MinimizeSize.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze_Element.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze_Streams.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Analyze_Streams_Finish.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Base.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__MultipleParsing.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__Duplicate.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File__HasReferences.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File_Dummy.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File_Other.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/File_Unknown.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/HashWrapper.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Config.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Config_Automatic.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Config_MediaInfo.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Config_PerPackage.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_File.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Inform.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfo_Internal.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfoList.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/MediaInfoList_Internal.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/TimeCode.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_7z.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Ace.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Bzip2.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Elf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Gzip.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Iso9660.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_MachO.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Mz.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Rar.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Tar.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Archive/File_Zip.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac_GeneralAudio.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac_GeneralAudio_Sbr_Ps.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac_Main.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aac_Others.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Ac3.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Ac4.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Adm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Adpcm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Als.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Amr.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Amv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Ape.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Aptx100.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Au.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Caf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Celt.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_ChannelGrouping.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_ChannelSplitting.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Dat.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Dsdiff.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Dsf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Dts.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_DtsUhd.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_DolbyAudioMetadata.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_DolbyE.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_ExtendedModule.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Flac.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Iamf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Iab.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_ImpulseTracker.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_La.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Mga.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Midi.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Module.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Mpc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_MpcSv8.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Mpega.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Mpegh3da.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_OpenMG.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Opus.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Pcm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Pcm_M2ts.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Pcm_Vob.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Ps2Audio.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Rkau.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_ScreamTracker3.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_SmpteSt0302.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_SmpteSt0337.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_SmpteSt0331.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Speex.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Tak.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Tta.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_TwinVQ.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Usac.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Vorbis.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Audio/File_Wvpk.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Duplicate/File__Duplicate__Base.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Duplicate/File__Duplicate__Writer.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Duplicate/File__Duplicate_MpegTs.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_EbuCore.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_Niso.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_Graph.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_Fims.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_Mpeg7.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_PBCore.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_PBCore2.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Export/Export_reVTMD.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_ArriRaw.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Bmp.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Bpg.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Dds.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Dpx.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Exr.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Gif.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Ico.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_GainMap.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Jpeg.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Pcx.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Png.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Psd.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Rle.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Tiff.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_Tga.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Image/File_WebP.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File__ReferenceFilesHelper.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File__ReferenceFilesHelper_Resource.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Aaf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ancillary.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Bdmv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Cdxa.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DashMpd.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DcpAm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DcpCpl.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DcpPkl.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Dpg.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DvDif.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_DvDif_Analysis.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Dvdv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Dxw.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Flv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Gxf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Gxf_TimeCode.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_HdsF4m.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Hls.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ibi.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ibi_Creation.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ism.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ivf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Lxf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mk.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_MiXml.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg4.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg4_Descriptors.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg4_Elements.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg4_TimeCode.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg_Descriptors.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mpeg_Psi.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_MpegPs.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_MpegTs.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_MpegTs_Duplicate.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Mxf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Nsv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Nut.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ogg.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ogg_SubElement.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_P2_Clip.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Pmp.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Ptx.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Riff.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Riff_Elements.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Rm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_SequenceInfo.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Skm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Swf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Umf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Vbi.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Wm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Wm_Elements.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Wtv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Multiple/File_Xdcam_Clip.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Reader/Reader_Directory.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Reader/Reader_File.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Reader/Reader_libcurl.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Reader/Reader_libmms.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File__Tags.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_ApeTag.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_C2pa.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Exif.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Icc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Id3.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Id3v2.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Iim.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Lyrics3.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Lyrics3v2.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_PropertyList.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_SphericalVideo.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_VorbisCom.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Tag/File_Xmp.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_AribStdB24B37.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Cdp.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Cmml.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_DvbSubtitle.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_DtvccTransport.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Eia608.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Eia708.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Kate.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_N19.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_OtherText.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Pac.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Pdf.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Pgs.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Scc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Scte20.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Sdp.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_SubRip.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Teletext.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_TimedText.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Text/File_Ttml.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Aic.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_AfdBarData.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Av1.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Avc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Avc_Duplicate.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_AvsV.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Avs3V.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Canopus.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_CineForm.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Dirac.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_DolbyVisionMetadata.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Ffv1.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Flic.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Fraps.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_H263.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_HdrVividMetadata.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Hevc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_HuffYuv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Lagarith.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Mpeg4v.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Mpegv.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_ProRes.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Theora.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Vc1.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Vc3.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Vp8.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Vp9.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Vvc.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/Video/File_Y4m.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/XmlUtils.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/OutputHelpers.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfo/ExternalCommandHelpers.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoJNI.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoDLL.cpp
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoDLL.def
  ${MediaInfoLib_SOURCES_PATH}/MediaInfoDLL/MediaInfoDLL.rc
)

if(NOT TinyXML_FOUND)
  include_directories(${MediaInfoLib_SOURCES_PATH}/ThirdParty/tinyxml2/)
  list(APPEND MediaInfoLib_SRCS ${MediaInfoLib_SOURCES_PATH}/ThirdParty/tinyxml2/tinyxml2.cpp)
endif()

add_library(mediainfo ${MediaInfoLib_SRCS})
target_include_directories(mediainfo INTERFACE ${INCLUDE_INSTALL_DIR})

if(BUILD_ZLIB)
  add_dependencies(mediainfo zlibstatic)
endif()

# disable some features. Maybe it should be enabled.
target_compile_definitions(mediainfo PRIVATE MEDIAINFO_LIBMMS_NO)
target_compile_definitions(mediainfo PRIVATE FMT_UNICODE=0)
if(NOT CURL_FOUND)
  target_compile_definitions(mediainfo PRIVATE MEDIAINFO_LIBCURL_NO)
else()
  include_directories(${CURL_INCLUDE_DIRS})
  target_link_libraries(mediainfo ${CURL_LIBRARIES})
  set(CURL_PC " libcurl")
  set(CURL_LIB " -lcurl")
endif()

# enable Graphviz runtime
target_compile_definitions(mediainfo PRIVATE MEDIAINFO_GRAPHVIZ_DLL_RUNTIME)

target_include_directories(mediainfo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ZenLib/Source
  ${MediaInfoLib_SOURCES_PATH}
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/md5/
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/base64/
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/aes-gladman/
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha1-gladman/
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/sha2-gladman/
  ${MediaInfoLib_SOURCES_PATH}/ThirdParty/hmac-gladman/)

if(NOT TinyXML_FOUND)
  target_include_directories(mediainfo PRIVATE ${MediaInfoLib_SOURCES_PATH}/ThirdParty/tinyxml2/)
else()
  target_link_libraries(mediainfo "${TinyXML_LIBRARIES}")
endif()

if(BUILD_ZENLIB)
  target_include_directories(mediainfo PRIVATE ${ZLIB_INCLUDE_DIRS})
endif()

target_link_libraries(mediainfo ${ZenLib_LIBRARY} ${ZLIB_LIBRARIES})

if(MSVC AND BUILD_SHARED_LIBS)
  install(FILES $<TARGET_PDB_FILE:mediainfo> DESTINATION ${BIN_INSTALL_DIR} OPTIONAL)
endif()

if (APPLE)
  set_target_properties(mediainfo PROPERTIES
    VERSION "${MediaInfoLib_SOVERSION}.${MediaInfoLib_MAJOR_VERSION}.${MediaInfoLib_MINOR_VERSION}"
    SOVERSION ${MediaInfoLib_SOVERSION})
else()
  set_target_properties(mediainfo PROPERTIES
    VERSION "${MediaInfoLib_SOVERSION}.${MediaInfoLib_MAJOR_VERSION}${MediaInfoLib_MINOR_VERSION}.${MediaInfoLib_PATCH_VERSION}"
    SOVERSION ${MediaInfoLib_SOVERSION})
endif()

set_target_properties(mediainfo PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS NO
  CXX_STANDARD_REQUIRED YES
)

install(TARGETS mediainfo EXPORT mediainfo-export
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR})

install(EXPORT mediainfo-export DESTINATION ${MediaInfoLib_CONFIG_INSTALL_DIR} FILE MediaInfoLibTargets.cmake)

foreach(HEADER ${MediaInfoLib_PUBLIC_HDRS})
  string(REPLACE ${MediaInfoLib_SOURCES_PATH} "" DIR ${HEADER})
  string(REGEX REPLACE "/[^/]*$" "" DIR ${DIR})
  install(FILES ${HEADER} DESTINATION include/${DIR})
endforeach(HEADER ${MediaInfoLib_PUBLIC_HDRS})

# pkg-config
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmediainfo.pc.in
                 ${CMAKE_CURRENT_BINARY_DIR}/libmediainfo.pc
                 @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmediainfo.pc
          DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif()

# cmake-modules
include(CMakePackageConfigHelpers)
configure_package_config_file(
  MediaInfoLibConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/MediaInfoLibConfig.cmake"
  INSTALL_DESTINATION ${MediaInfoLib_CONFIG_INSTALL_DIR}
  PATH_VARS BIN_INSTALL_DIR
            LIB_INSTALL_DIR
            INCLUDE_INSTALL_DIR)

write_basic_config_version_file(MediaInfoLibConfigVersion.cmake VERSION ${MediaInfoLib_VERSION} COMPATIBILITY AnyNewerVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/MediaInfoLibConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/MediaInfoLibConfigVersion.cmake
  DESTINATION ${MediaInfoLib_CONFIG_INSTALL_DIR})
