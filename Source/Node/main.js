/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

const Native = require('bindings')('mediainfolib');

const Stream = {
    General: 0,
    Video: 1,
    Audio: 2,
    Text: 3,
    Other: 4,
    Image: 5,
    Menu: 6,
    Max: 7
};

const Info = {
    Name: 0,
    Text: 1,
    Measure: 2,
    Options: 3,
    Name_Text: 4,
    Measure_Text: 5,
    Info: 6,
    HowTo: 7,
    Max: 8
};

const InfoOptions = {
    ShowInInform: 0,
    Reserved: 1,
    ShowInSupported: 2,
    TypeOfValue: 3,
    Max: 4
}

const FileOptions = {
    Nothing: 0x00,
    NoRecursive: 0x01,
    CloseAll: 0x02,
    Max: 0x04
};

module.exports = {
    MediaInfo: Native.MediaInfo,
    MediaInfoList: Native.MediaInfoList,
    Stream: Stream,
    Info: Info,
    InfoOptions: InfoOptions,
    FileOptions: FileOptions
}
