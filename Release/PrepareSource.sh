# MediaInfoLib/Release/PrepareSource.sh
# Prepare the source of MediaInfoLib

# Copyright (c) MediaArea.net SARL. All Rights Reserved.
# Use of this source code is governed by a BSD-style license that can
# be found in the License.html file in the root of the source tree.

function _get_source () {

	# Determine where are the sources of the project
	if [ $(b.opt.get_opt --source-path) ]; then
		MIL_source=$(sanitize_arg $(b.opt.get_opt --source-path))
	else	
		if [ $(b.opt.get_opt --repo-url) ]; then
			RepoURL=$(sanitize_arg $(b.opt.get_opt --repo-url))
		else
			RepoURL="https://github.com/MediaArea/"
		fi
		getRepo MediaInfoLib $RepoURL $Path
		MIL_source=$Path/$Project
	fi

	# Dependency : ZenLib
	if [ $(b.opt.get_opt --repo-url) ]; then
		RepoURL=$(sanitize_arg $(b.opt.get_opt --repo-url))
	else
		RepoURL="https://github.com/MediaArea/"
	fi
	echo
	getRepo ZenLib $RepoURL $Path
	ZL_source=$Path/ZenLib

	# Dependency : zlib
	echo
	getRepo zlib https://github.com/madler/ $Path
	zlib_source=$Path/zlib

}

function _linux_compil () {

	echo
	echo "Create the archive for compilation on Linux:"
	echo "1: copy what is wanted..."

	cd $Path
	mkdir MediaInfo_DLL_${Version}_GNU_FromSource
	cd MediaInfo_DLL_${Version}_GNU_FromSource

	cp -r $ZL_source .
	cp -r $MIL_source .
	mv MediaInfoLib/Project/GNU/Library/AddThisToRoot_DLL_compile.sh SO_Compile.sh
	# TODO: call the function who build a clean ZenLib compil archive

	mkdir -p Shared/Project/_Common
	cp -r $zlib_source Shared/Project
	#cp -r $curl_source Shared/Project

	echo "2: remove what isn't wanted..."
	cd MediaInfoLib
		rm -f .cvsignore .gitignore
		rm -fr .git
		rm -f ToDo.txt
		rm -f ReadMe.txt
		rm -fr debian
		cd Project
			rm -f GNU/libmediainfo.dsc GNU/libmediainfo.spec
			rm -fr Solaris
			rm -fr BCB CMake CodeBlocks Coverity Delphi Java NetBeans
			rm -fr MSCS2008 MSCS2010 MSJS MSVB MSVB2010
			rm -fr MSVC2005 MSVC2008 MSVC2010 MSVC2012 MSVC2013
			rm -fr PureBasic
		cd ..
		rm -fr Contrib
		rm -fr Release
		#cd Release
		#	rm -f CleanUp.bat Example.ogg ReadMe_DLL_Windows.txt
		#	rm -f Release_DLL_GNU_Prepare.bat Release_Lib_GNU_Prepare.bat
		#	rm -f Release_DLL_Windows_i386.bat Release_DLL_Windows_x64.bat
		#	rm -f Release_Source.bat UpgradeVersion.sh
		#cd ..
		cd Source
			rm -f Doc/setlocale.txt
			rm -fr Install
			rm -fr PreRelease
			rm -fr RegressionTest
			rm -fr Resource
			rm -f MediaInfoDLL/MediaInfoDLL.def
			rm -f MediaInfoDLL/MediaInfoDLL.jsl
			rm -f MediaInfoDLL/MediaInfoDLL.pas
			rm -f MediaInfoDLL/MediaInfoDLL.pb
			rm -f MediaInfoDLL/MediaInfoDLL.vb
			rm -f ThirdParty/aes-gladman/aes_amd64.asm
			rm -f ThirdParty/aes-gladman/aes.txt
			rm -f ThirdParty/aes-gladman/aes_x86_v1.asm
			rm -f ThirdParty/aes-gladman/aes_x86_v2.asm
			rm -f ThirdParty/aes-gladman/via_ace.txt
		cd ..
	cd ..

	if $MakeArchives; then
		echo "3: compressing..."
		cd $Path
		mkdir archives
		# To specify the compression level
		#$(/bin/tar -cf MediaInfo_Lib_Source.tar MediaInfoLib/)
		(GZIP=-9 tar -czf archives/MediaInfo_DLL_${Version}_GNU_FromSource.tgz MediaInfo_DLL_${Version}_GNU_FromSource)
		(BZIP=-9 tar -cjf archives/MediaInfo_DLL_${Version}_GNU_FromSource.tbz MediaInfo_DLL_${Version}_GNU_FromSource)
		(XZ_OPT=-9e tar -cJf archives/MediaInfo_DLL_${Version}_GNU_FromSource.txz MediaInfo_DLL_${Version}_GNU_FromSource)
	fi

}

function btask.PrepareSource.run () {

	Project=MediaInfoLib
	Path=/tmp/ma

	LinuxCompil=false
	if b.opt.has_flag? --linux-compil; then
		LinuxCompil=true
	fi
	WindowsCompil=false
	if b.opt.has_flag? --windows-compil; then
		WindowsCompil=true
	fi
	LinuxPackages=false
	if b.opt.has_flag? --linux-packages; then
		LinuxPackages=true
	fi
	AllTarget=false
	if b.opt.has_flag? --all; then
		AllTarget=true
	fi
	MakeArchives=true
	if b.opt.has_flag? --no-archives; then
		MakeArchives=false
	fi
	#CleanUp=true
	#if [ $(b.opt.get_opt --no-cleanup) ]; then
	#	CleanUp=false
	#fi

	#mkdir $Path
	cd $Path
	rm -fr archives
	rm -fr MediaInfo_DLL_${Version}_GNU_FromSource

	if $LinuxCompil || $WindowsCompil || $LinuxPackages || $AllTarget; then
		_get_source
	else
		echo "Besides --project and --version, you must specify at least"
		echo "one of this options:"
		echo
		echo "--linux-compil|-lc"
		echo "              Create the archive for compilation on Linux"
		echo
		echo "--windows-compil|-wc"
		echo "              Create the archive for compilation on Windows"
		echo
		echo "--linux-packages|-lp|--linux-package"
		echo "              Create the archive for Linux packages creation"
		echo
		echo "--all|-a"
		echo "              Create all the targets for this project"
	fi
	if $LinuxCompil; then
		_linux_compil
	fi
	if $WindowsCompil; then
		echo _windows_compil
	fi
	if $LinuxPackages; then
		echo _linux_packages
	fi
	if $AllTarget; then
		_linux_compil
		echo _windows_compil
		echo _linux_packages
	fi

	#..\..\Shared\Binary\Windows_i386\7-zip\7z a -r -t7z -mx9 libmediainfo__AllInclusive.7z MediaInfoLib\* ZenLib\* zlib\*
	# unix (look the man):
	#7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on archive.7z dir1

	
	# Clean up
	# TODO: ??? existing in Release_Source.bat but not in Release_Source.sh

    #unset -v MIL_files index MIL_source
}
