##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 #
 #  Use of this source code is governed by a BSD-style license that can
 #  be found in the License.html file in the root of the source tree.
 ##

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# Ruby example
#
# To make this example working, you must put the MediaInfoDLL.rb and
# example.ogg in the same folder, then run:
# ruby HowToUse_Dll.rb
# The MediaInfo library must reside in a standard library path (e.g.
# C:\Windows\System32\MediaInfo.dll on Windows), if this isn't the case set
# the LIBMEDIAINFO_PATH environment variable to the right library path.
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


require './MediaInfoDLL.rb'

mi=MediaInfo::MediaInfo.new

#Information about MediaInfo
puts "Info_Parameters"
puts mi.Option "Info_Parameters"

puts ""
puts "Info_Codecs"
puts mi.Option "Info_Codecs"

#An example of how to use the library
puts ""
puts "Open"
mi.Open "Example.ogg"

puts ""
puts "Inform with Complete=false"
mi.Option "Complete"
puts mi.Inform

puts ""
puts "Inform with Complete=true"
puts mi.Option "Complete", "1"
puts mi.Inform

puts ""
puts "Custom Inform"
mi.Option "Inform", "General;Example : FileSize=%FileSize%"
puts mi.Inform

puts ""
puts "Get with Stream=General and Parameter='FileSize'"
puts mi.Get :General, 0, "FileSize"

puts ""
puts "Get with Stream=General and Parameter=46"
puts mi.Get :General, 0, 46

puts ""
puts "Count_Get with StreamKind=Stream_Audio"
puts mi.Count_Get :Audio

puts ""
puts "Get with Stream=General and Parameter='AudioCount'"
puts mi.Get :General, 0, "AudioCount"

puts ""
puts "Get with Stream=Audio and Parameter='StreamCount'"
puts mi.Get :Audio, 0, "StreamCount"

puts ""
puts "Close"
mi.Close

#By buffer example

#Open example file for reading
File.open("Example.ogg", "r") do |file|
    puts ""
    puts "Open_Buffer_Init"
    mi.Open_Buffer_Init file.size, 0

    puts ""
    puts "Parsing loop"
    while true
        buffer=file.read(7*188)
        if buffer
            #Send the buffer to MediaInfo
            status=mi.Open_Buffer_Continue buffer, buffer.bytesize

            if (status & 0x08) #Bit3=Finished
                break
            end

            #Test if there is a MediaInfo request to go elsewhere
            seek=mi.Open_Buffer_Continue_GoTo_Get
            if seek != -1 + (2 ** 64) # equal to (uint64)-1 in C/C++
                file.seek seek #Seek the file
                mi.Open_Buffer_Init file.size, file.tell #Inform MediaInfo we have seek
            end        
        else
            break
        end
    end

    puts ""
    puts "Open_Buffer_Finalize"
    mi.Open_Buffer_Finalize

    puts ""
    puts "Get with Stream=General and Parameter='Format'"
    puts mi.Get :General, 0, "Format"
end
