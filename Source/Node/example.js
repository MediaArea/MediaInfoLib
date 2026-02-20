/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

const fs = require('fs')
const miModule = require('./main.js')

const mi = miModule.MediaInfo() // or miModule.MediaInfo('MediaInfo library path')

//Information about MediaInfo
console.log("Info_Parameters")
console.log(mi.Option("Info_Parameters"))

console.log("")
console.log("Info_Codecs")
console.log(mi.Option("Info_Codecs"))

//An example of how to use the library
console.log("")
console.log("Open")
mi.Open("Example.ogg")

console.log("")
console.log("Inform with Complete=false")
mi.Option("Complete")
console.log(mi.Inform())

console.log("")
console.log("Inform with Complete=true")
mi.Option("Complete", "1")
console.log(mi.Inform())

console.log("")
console.log("Custom Inform")
mi.Option("Inform", "General;Example : FileSize=%FileSize%")
console.log(mi.Inform())

console.log("")
console.log("Get with Stream=General and Parameter='FileSize'")
console.log(mi.Get(miModule.Stream.General, 0, "FileSize"))

console.log("")
console.log("Get with Stream=General and Parameter=46")
console.log(mi.Get(miModule.Stream.General, 0, 46))

console.log("")
console.log("Count_Get with StreamKind=Stream_Audio")
console.log(mi.Count_Get(miModule.Stream.Audio))

console.log("")
console.log("Get with Stream=General and Parameter='AudioCount'")
console.log(mi.Get(miModule.Stream.General, 0, "AudioCount"))

console.log("")
console.log("Get with Stream=Audio and Parameter='StreamCount'")
console.log(mi.Get(miModule.Stream.Audio, 0, "StreamCount"))

console.log("")
console.log("Close")
mi.Close()

//By buffer example
const bufferSize = 7 * 188
var buffer = new Buffer(bufferSize)

const file = fs.openSync('Example.ogg', 'r')
const stats = fs.statSync('Example.ogg')

console.log("")
console.log("Open_Buffer_Init")
mi.Open_Buffer_Init(stats.size, 0)

console.log("")
console.log("Parsing loop")

var seek=null
while (count=fs.readSync(file, buffer, 0, bufferSize, seek)) {
    // Send the buffer to MediaInfo
    var status = mi.Open_Buffer_Continue(buffer, count)
    if (status&0x08) //Bit3=Finished
        break


    // Test if there is a MediaInfo request to go elsewhere
    seek = mi.Open_Buffer_Continue_GoTo_Get()
    if (seek != -1) {
       mi.Open_Buffer_Init(stats.size, seek) // Inform MediaInfo we have seek
    } else {
        seek = null
    }
}

fs.closeSync(file)

console.log("")
console.log("Open_Buffer_Finalize")
mi.Open_Buffer_Finalize()

console.log("")
console.log("Get with Stream=General and Parameter='Format'")
console.log(mi.Get(miModule.Stream.General, 0, "Format"))
