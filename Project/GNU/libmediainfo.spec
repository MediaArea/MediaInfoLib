%global libmediainfo_version      21.09
%global libmediainfo_version_major      21
%global libmediainfo_version_minor      09
%global libzen_version            0.4.39
%global libzen_version_major      0
%global libzen_version_minor      4
%global libzen_version_release    39

%if 0%{?fedora_version} || 0%{?centos_version} >= 600 || 0%{?rhel_version} >= 600
%global package_with_0_ending 0
%global libmediainfo_name libmediainfo
%else
%global package_with_0_ending 1
%global libmediainfo_name libmediainfo0
%endif

%global name_without_0_ending libmediainfo

%global libzen_suffix %{libzen_version_major}%{libzen_version_minor}%{libzen_version_release}
%global libmediainfo_suffix %{libmediainfo_version_major}%{libmediainfo_version_minor}

Name:           %{libmediainfo_name}
Version:        %{libmediainfo_version}
Release:        1
Summary:        Most relevant technical and tag data for video and audio files -- runtime

Group:          System/Libraries
License:        BSD-2-Clause
URL:            http://MediaArea.net/MediaInfo
Packager:       MediaArea.net SARL <info@mediaarea.net>
Source0:        %{name_without_0_ending}_%{version}.tar.gz
%if !%{package_with_0_ending}
Provides:       %{name_without_0_ending}0 = %{version}
Obsoletes:      %{name_without_0_ending}0 < %{version}
%endif

Prefix:         %{_prefix}
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  gcc-c++
BuildRequires:  libzen-devel >= %{libzen_version}
BuildRequires:  pkgconfig
BuildRequires:  zlib-devel
BuildRequires:  doxygen
BuildRequires:  libtool
BuildRequires:  automake
BuildRequires:  autoconf
%if ! 0%{?rhel_version} && ! 0%{?centos_version} && ((! 0%{?sles_version} && ! 0%{?sle_version}) || 0%{?sle_version} >= 150000)
%if ! (0%{?sle_version} == 120300 && 0%{?is_opensuse})
BuildRequires: python2-devel
%endif
BuildRequires: python3-devel
%endif

%if 0%{?rhel_version} || 0%{?centos_version}
%if 0%{?rhel_version} >= 800 || 0%{?centos_version} >= 800
BuildRequires:  gdb
%endif
%if 0%{?rhel_version} > 599
BuildRequires:  libcurl-devel
%endif
%if 0%{?centos_version} > 599
BuildRequires:  libcurl-devel
%endif
%else
BuildRequires:  libcurl-devel
%endif

%if 0%{?mageia} > 6
%ifarch x86_64
BuildRequires: lib64openssl-devel
%else
BuildRequires: libopenssl-devel
%endif
%endif

%if 0%{?rhel}
%package        -n %{libmediainfo_name}%{libmediainfo_suffix}
Summary:        Most relevant technical and tag data for video and audio files -- slot version
Requires:       libzen%{libzen_suffix} >= %{libzen_version}
%endif

%global libmediainfo_description MediaInfo is a convenient unified display of the most relevant technical\
and tag data for video and audio files.\
\
What information can I get from MediaInfo?\
* General: title, author, director, album, track number, date, duration...\
* Video: codec, aspect, fps, bitrate...\
* Audio: codec, sample rate, channels, language, bitrate...\
* Text: language of subtitle\
* Chapters: number of chapters, list of chapters\
\
DivX, XviD, H263, H.263, H264, x264, ASP, AVC, iTunes, MPEG-1,\
MPEG1, MPEG-2, MPEG2, MPEG-4, MPEG4, MP4, M4A, M4V, QuickTime,\
RealVideo, RealAudio, RA, RM, MSMPEG4v1, MSMPEG4v2, MSMPEG4v3,\
VOB, DVD, WMA, VMW, ASF, 3GP, 3GPP, 3GP2\
\
What format (container) does MediaInfo support?\
* Video: MKV, OGM, AVI, DivX, WMV, QuickTime, Real, MPEG-1,\
  MPEG-2, MPEG-4, DVD (VOB) (Codecs: DivX, XviD, MSMPEG4, ASP,\
  H.264, AVC...)\
* Audio: OGG, MP3, WAV, RA, AC3, DTS, AAC, M4A, AU, AIFF\
* Subtitles: SRT, SSA, ASS, SAMI\
\
This package contains the shared library for MediaInfo.

%description
%{libmediainfo_description}

%if 0%{?rhel}
%description -n %{libmediainfo_name}%{libmediainfo_suffix}
%{libmediainfo_description}
%endif

%package        -n %{name_without_0_ending}-doc
Summary:        Most relevant technical and tag data for video and audio files -- documentation
Group:          Development/Libraries
Requires:       %{name} = %{version}
%if !0%{?suse_version} || 0%{?suse_version} >= 1200
BuildArch:      noarch
%endif

%if 0%{?rhel}
%package        -n %{name_without_0_ending}%{libmediainfo_suffix}-doc
Summary:        Most relevant technical and tag data for video and audio files -- documentation
Group:          Development/Libraries
Requires:       %{libmediainfo_name}%{libmediainfo_suffix} = %{version}
%endif

%global doc_description MediaInfo is a convenient unified display of the most relevant technical\
and tag data for video and audio files.\
\
What information can I get from MediaInfo?\
* General: title, author, director, album, track number, date, duration...\
* Video: codec, aspect, fps, bitrate...\
* Audio: codec, sample rate, channels, language, bitrate...\
* Text: language of subtitle\
* Chapters: number of chapters, list of chapters\
\
DivX, XviD, H263, H.263, H264, x264, ASP, AVC, iTunes, MPEG-1,\
MPEG1, MPEG-2, MPEG2, MPEG-4, MPEG4, MP4, M4A, M4V, QuickTime,\
RealVideo, RealAudio, RA, RM, MSMPEG4v1, MSMPEG4v2, MSMPEG4v3,\
VOB, DVD, WMA, VMW, ASF, 3GP, 3GPP, 3GP2\
\
What format (container) does MediaInfo support?\
* Video: MKV, OGM, AVI, DivX, WMV, QuickTime, Real, MPEG-1,\
  MPEG-2, MPEG-4, DVD (VOB) (Codecs: DivX, XviD, MSMPEG4, ASP,\
  H.264, AVC...)\
* Audio: OGG, MP3, WAV, RA, AC3, DTS, AAC, M4A, AU, AIFF\
* Subtitles: SRT, SSA, ASS, SAMI\
\
This package contains the documentation

%description    -n %{name_without_0_ending}-doc
%{doc_description}

%if 0%{?rhel}
%description    -n %{name_without_0_ending}%{libmediainfo_suffix}-doc
%{doc_description}
%endif

%package        -n %{name_without_0_ending}-devel
Summary:        Most relevant technical and tag data for video and audio files -- development
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}
Requires:       libzen-devel%{?_isa} >= %{libzen_version}
%if 0%{?rhel_version} || 0%{?centos_version}
%if 0%{?rhel_version} > 599
Requires:  libcurl-devel
%endif
%if 0%{?centos_version} > 599
Requires:  libcurl-devel
%endif
%else
Requires:  libcurl-devel
%endif

%if 0%{?rhel}
%package        -n %{name_without_0_ending}%{libmediainfo_suffix}-devel
Summary:        Most relevant technical and tag data for video and audio files -- development
Group:          Development/Libraries
Requires:       %{libmediainfo_name}%{libmediainfo_suffix}%{?_isa} = %{version}
Requires:       libcurl-devel
%endif

%global devel_description MediaInfo is a convenient unified display of the most relevant technical\
and tag data for video and audio files.\
\
What information can I get from MediaInfo?\
* General: title, author, director, album, track number, date, duration...\
* Video: codec, aspect, fps, bitrate...\
* Audio: codec, sample rate, channels, language, bitrate...\
* Text: language of subtitle\
* Chapters: number of chapters, list of chapters\
\
DivX, XviD, H263, H.263, H264, x264, ASP, AVC, iTunes, MPEG-1,\
MPEG1, MPEG-2, MPEG2, MPEG-4, MPEG4, MP4, M4A, M4V, QuickTime,\
RealVideo, RealAudio, RA, RM, MSMPEG4v1, MSMPEG4v2, MSMPEG4v3,\
VOB, DVD, WMA, VMW, ASF, 3GP, 3GPP, 3GP2\
\
What format (container) does MediaInfo support?\
* Video: MKV, OGM, AVI, DivX, WMV, QuickTime, Real, MPEG-1,\
  MPEG-2, MPEG-4, DVD (VOB) (Codecs: DivX, XviD, MSMPEG4, ASP,\
  H.264, AVC...)\
* Audio: OGG, MP3, WAV, RA, AC3, DTS, AAC, M4A, AU, AIFF\
* Subtitles: SRT, SSA, ASS, SAMI\
\
This package contains the include files and mandatory libraries\
for development.

%description    -n %{name_without_0_ending}-devel
%{devel_description}

%if 0%{?rhel}
%description    -n %{name_without_0_ending}%{libmediainfo_suffix}-devel
%{devel_description}
%endif

%if ! 0%{?rhel_version} && ! 0%{?centos_version} && ((! 0%{?sles_version} && ! 0%{?sle_version}) || 0%{?sle_version} >= 150000)
%if ! (0%{?sle_version} == 120300 && 0%{?is_opensuse})
%package        -n python2-mediainfo
Summary:        Most relevant technical and tag data for video and audio files -- python2 binding
Group:          Development/Libraries
Requires:       %{libmediainfo_name}%{?_isa} = %{version}

%description    -n python2-mediainfo
MediaInfo is a convenient unified display of the most relevant technical
and tag data for video and audio files.

What information can I get from MediaInfo?
* General: title, author, director, album, track number, date, duration...
* Video: codec, aspect, fps, bitrate...
* Audio: codec, sample rate, channels, language, bitrate...
* Text: language of subtitle
* Chapters: number of chapters, list of chapters

DivX, XviD, H263, H.263, H264, x264, ASP, AVC, iTunes, MPEG-1,
MPEG1, MPEG-2, MPEG2, MPEG-4, MPEG4, MP4, M4A, M4V, QuickTime,
RealVideo, RealAudio, RA, RM, MSMPEG4v1, MSMPEG4v2, MSMPEG4v3,
VOB, DVD, WMA, VMW, ASF, 3GP, 3GPP, 3GP2

What format (container) does MediaInfo support?
* Video: MKV, OGM, AVI, DivX, WMV, QuickTime, Real, MPEG-1,
  MPEG-2, MPEG-4, DVD (VOB) (Codecs: DivX, XviD, MSMPEG4, ASP,
  H.264, AVC...)
* Audio: OGG, MP3, WAV, RA, AC3, DTS, AAC, M4A, AU, AIFF
* Subtitles: SRT, SSA, ASS, SAMI

This package contains the python2 wrapper of the library.
%endif

%package        -n python3-mediainfo
Summary:        Most relevant technical and tag data for video and audio files -- python3 binding
Group:          Development/Libraries
Requires:       %{libmediainfo_name}%{?_isa} = %{version}

%description    -n python3-mediainfo
MediaInfo is a convenient unified display of the most relevant technical
and tag data for video and audio files.

What information can I get from MediaInfo?
* General: title, author, director, album, track number, date, duration...
* Video: codec, aspect, fps, bitrate...
* Audio: codec, sample rate, channels, language, bitrate...
* Text: language of subtitle
* Chapters: number of chapters, list of chapters

DivX, XviD, H263, H.263, H264, x264, ASP, AVC, iTunes, MPEG-1,
MPEG1, MPEG-2, MPEG2, MPEG-4, MPEG4, MP4, M4A, M4V, QuickTime,
RealVideo, RealAudio, RA, RM, MSMPEG4v1, MSMPEG4v2, MSMPEG4v3,
VOB, DVD, WMA, VMW, ASF, 3GP, 3GPP, 3GP2

What format (container) does MediaInfo support?
* Video: MKV, OGM, AVI, DivX, WMV, QuickTime, Real, MPEG-1,
  MPEG-2, MPEG-4, DVD (VOB) (Codecs: DivX, XviD, MSMPEG4, ASP,
  H.264, AVC...)
* Audio: OGG, MP3, WAV, RA, AC3, DTS, AAC, M4A, AU, AIFF
* Subtitles: SRT, SSA, ASS, SAMI

This package contains the python3 wrapper of the library.
%endif

%prep
%setup -q -n MediaInfoLib
cp           Release/ReadMe_DLL_Linux.txt ReadMe.txt
mv           History_DLL.txt History.txt
sed -i 's/\r$//g' *.txt Source/Example/*

find . -type f -exec chmod 644 {} ';'

pushd Project/GNU/Library
    autoreconf -i
popd

%build
export CFLAGS="-g %{optflags}"
export CPPFLAGS="-g %{optflags}"
export CXXFLAGS="%{optflags}"

pushd Source/Doc/
    doxygen -u Doxyfile
    doxygen Doxyfile
popd
cp Source/Doc/*.html ./

pushd Project/GNU/Library
%if 0%{?rhel} && 0%{?rhel} < 6
%configure --enable-shared --disable-static --enable-visibility
%else
%if 0%{?mageia} > 5
%configure --enable-shared --disable-static --enable-visibility --with-libcurl --disable-dependency-tracking
%else
%configure --enable-shared --disable-static --enable-visibility --with-libcurl
%endif
%endif

make %{?_smp_mflags}
popd

%install
pushd Project/GNU/Library/
    make install DESTDIR=%{buildroot}
popd

# MediaInfoDLL headers
install -dm 755 %{buildroot}%{_includedir}/MediaInfo
install -m 644 Source/MediaInfo/MediaInfo.h %{buildroot}%{_includedir}/MediaInfo
install -m 644 Source/MediaInfo/MediaInfoList.h %{buildroot}%{_includedir}/MediaInfo
install -m 644 Source/MediaInfo/MediaInfo_Const.h %{buildroot}%{_includedir}/MediaInfo
install -m 644 Source/MediaInfo/MediaInfo_Events.h %{buildroot}%{_includedir}/MediaInfo
install -dm 755 %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.cs %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.h %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL_Static.h %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.JNA.java %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.JNI.java %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.JNative.java %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.py %{buildroot}%{_includedir}/MediaInfoDLL
install -m 644 Source/MediaInfoDLL/MediaInfoDLL3.py %{buildroot}%{_includedir}/MediaInfoDLL

# Python modules
%if ! 0%{?rhel_version} && ! 0%{?centos_version} && ((! 0%{?sles_version} && ! 0%{?sle_version}) || 0%{?sle_version} >= 150000)
%if ! (0%{?sle_version} == 120300 && 0%{?is_opensuse})
install -dm 755 %{buildroot}%{python2_sitelib}
install -m 644 Source/MediaInfoDLL/MediaInfoDLL.py %{buildroot}%{python2_sitelib}
%endif
install -dm 755 %{buildroot}/%{python3_sitelib}
install -m 644 Source/MediaInfoDLL/MediaInfoDLL3.py %{buildroot}%{python3_sitelib}
%endif

rm -f %{buildroot}%{_libdir}/%{name_without_0_ending}.la

%post

%postun

%define libmediainfo_files %defattr(-,root,root,-)\
%doc History.txt ReadMe.txt\
%if 0%{?fedora_version} || 0%{?centos_version} >= 700 || 0%{?rhel_version} >= 700\
%license License.html\
%else\
%doc License.html\
%endif\
%{_libdir}/%{name_without_0_ending}.so.*

%files
%{libmediainfo_files}

%if 0%{?rhel} == 5
%exclude %{_usr}/lib/debug
%exclude %{_usr}/src/debug
%endif

%if 0%{?rhel}
%files -n %{libmediainfo_name}%{libmediainfo_suffix}
%{libmediainfo_files}
%endif

%define doc_files %defattr(-,root,root,-)\
%doc Changes.txt Documentation.html Doc Source/Example

%files     -n %{name_without_0_ending}-doc
%{doc_files}

%if 0%{?rhel}
%files -n %{name_without_0_ending}%{libmediainfo_suffix}-doc
%{doc_files}
%endif

%define devel_files %defattr(-,root,root,-)\
%{_includedir}/MediaInfo\
%{_includedir}/MediaInfoDLL\
%{_libdir}/pkgconfig/*.pc\
%{_libdir}/%{name_without_0_ending}.so

%files     -n %{name_without_0_ending}-devel
%{devel_files}

%if 0%{?rhel}
%files -n %{name_without_0_ending}%{libmediainfo_suffix}-devel
%{devel_files}
%endif

%if ! 0%{?rhel_version} && ! 0%{?centos_version} && ((! 0%{?sles_version} && ! 0%{?sle_version}) || 0%{?sle_version} >= 150000)
%if ! (0%{?sle_version} == 120300 && 0%{?is_opensuse})
%files     -n python2-mediainfo
%{python2_sitelib}/*
%endif

%files     -n python3-mediainfo
%{python3_sitelib}/*
%endif

%changelog
* Sun Jan 01 2012 MediaArea.net SARL <info@mediaarea.net> - 21.09-0
- See History.txt for more info and real dates
- Previous packages made by Toni Graffy <toni@links2linux.de>
- Fedora style made by Vasiliy N. Glazov <vascom2@gmail.com>
