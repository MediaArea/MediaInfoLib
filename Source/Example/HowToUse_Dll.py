##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 #
 #  Use of this source code is governed by a BSD-style license that can
 #  be found in the License.html file in the root of the source tree.
 ##

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# Python example
#
# To make this example working, you must put MediaInfo.Dll, MediaInfoDLL.py
# and example.ogg in the same folder
#
# HowToUse_Dll.py and HowToUse_Dll3.py are same
# MediaInfoDLL.py and MediaInfoDLL3.py are same
# but all files are kept in order to not break programs calling them.
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

from MediaInfoDLL import *

MI = MediaInfo()

Version=MI.Option_Static("Info_Version", "0.7.7.0;MediaInfoDLL_Example_Python;0.7.7.0")
if Version=="":
    print("MediaInfo.Dll: this version of the DLL is not compatible")
    exit


#Information about MediaInfo
print("Info_Parameters")
MI.Option_Static("Info_Parameters")

print("")
print("Info_Capacities")
print(MI.Option_Static("Info_Capacities"))

print("")
print("Info_Codecs")
print(MI.Option_Static("Info_Codecs"))


#An example of how to use the library
print("")
print("Open")
MI.Open("Example.ogg")

print("")
print("Inform with Complete=false")
MI.Option_Static("Complete")
print(MI.Inform())

print("")
print("Inform with Complete=true")
MI.Option_Static("Complete", "1")
print(MI.Inform())

print("")
print("Custom Inform")
MI.Option_Static("Inform", "General;Example : FileSize=%FileSize%")
print(MI.Inform())

print("")
print("Get with Stream=General and Parameter='FileSize'")
print(MI.Get(Stream.General, 0, "FileSize"))

print("")
print("GetI with Stream=General and Parameter=46")
print(MI.GetI(Stream.General, 0, 46))

print("")
print("Count_Get with StreamKind=Stream_Audio")
print(MI.Count_Get(Stream.Audio))

print("")
print("Get with Stream=General and Parameter='AudioCount'")
print(MI.Get(Stream.General, 0, "AudioCount"))

print("")
print("Get with Stream=Audio and Parameter='StreamCount'")
print(MI.Get(Stream.Audio, 0, "StreamCount"))

print("")
print("Close")
MI.Close()

#By buffer example

#Open example file for reading
try:
    File=open('Example.ogg', 'rb')
except IOError:
    exit(1)

#Get file size
File.seek(0,2)
Size=File.tell()
File.seek(0)

print("")
print("Open_Buffer_Init")
MI.Open_Buffer_Init(Size, 0)

print("")
print("Parsing loop")
while True:
    Buffer=File.read(7*188)
    if Buffer:
        #Send the buffer to MediaInfo
        Status=c_size_t(MI.Open_Buffer_Continue(Buffer, len(Buffer))).value
        if Status & 0x08: #Bit3=Finished
            break

        #Test if there is a MediaInfo request to go elsewhere
        Seek = c_longlong(MI.Open_Buffer_Continue_GoTo_Get()).value
        if  Seek != -1:
            File.seek(Seek) #Seek the file
            MI.Open_Buffer_Init(Size, File.tell()) #Inform MediaInfo we have seek
    else:
        break

print("")
print("Open_Buffer_Finalize")
MI.Open_Buffer_Finalize()

print("")
print("Get with Stream=General and Parameter='Format'")
print(MI.Get(Stream.General, 0, "Format"))
