/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Rust wrapper for MediaInfo Library
// See MediaInfo.h for help
//
// Requires the requirements of rust-bindgen
// https://rust-lang.github.io/rust-bindgen/requirements.html
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

//! Rust wrapper for MediaInfo Library
//!
//! See `MediaInfo.h` for help

use std::ffi::{CStr, CString};

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub struct MediaInfo {
    handle: *mut ::std::os::raw::c_void,
}

impl MediaInfo {
    pub fn new() -> Self {
        let handle = unsafe { MediaInfoA_New() };
        unsafe {
            MediaInfoA_Option(
                handle,
                CString::new("CharSet").unwrap().as_ptr(),
                CString::new("UTF-8").unwrap().as_ptr(),
            );
            MediaInfoA_Option(
                handle,
                CString::new("setlocale_LC_CTYPE").unwrap().as_ptr(),
                CString::new("").unwrap().as_ptr(),
            );
        }
        Self { handle }
    }

    /// Open a file and collect information about it (technical information and tags)
    ///
    /// @brief Open a file
    ///
    /// @param File Full name of file to open
    ///
    /// @retval 0 File not opened
    ///
    /// @retval 1 File opened
    pub fn open(&self, file: &str) -> usize {
        unsafe { MediaInfoA_Open(self.handle, CString::new(file).unwrap().as_ptr()) }
    }

    /// Open a Buffer (Begin and end of the stream) and collect information about it (technical information and tags)
    ///
    /// @brief Open a buffer
    ///
    /// @param Begin First bytes of the buffer
    ///
    /// @param Begin_Size Size of Begin
    ///
    /// @param End Last bytes of the buffer
    ///
    /// @param End_Size Size of End
    ///
    /// @param File_Size Total size of the file
    ///
    /// @retval 0 File not opened
    ///
    /// @retval 1 File opened
    pub fn open_buffer(
        &self,
        begin: &[u8],
        begin_size: usize,
        end: &[u8],
        end_size: usize,
    ) -> usize {
        let begin_ptr = begin.as_ptr();
        let end_ptr = end.as_ptr();
        unsafe {
            MediaInfoA_Open_Buffer(
                self.handle,
                begin_ptr as *mut MediaInfo_int8u,
                begin_size,
                end_ptr as *mut MediaInfo_int8u,
                end_size,
            )
        }
    }

    /// Open a stream and collect information about it (technical information and tags)
    ///
    /// @brief Open a stream (Init)
    ///
    /// @param File_Size Estimated file size
    ///
    /// @param File_Offset Offset of the file (if we don't have the beginning of the file)
    ///
    /// @retval 0 File not opened
    ///
    /// @retval 1 File opened
    pub fn open_buffer_init(&self, file_size: u64, file_offset: u64) -> usize {
        unsafe { MediaInfoA_Open_Buffer_Init(self.handle, file_size, file_offset) }
    }

    /// Open a stream and collect information about it (technical information and tags)
    ///
    /// @brief Open a stream (Continue)
    ///
    /// @param Buffer pointer to the stream
    ///
    /// @param Buffer_Size Count of bytes to read
    ///
    /// @return a bitfield <br>
    ///         bit 0: Is Accepted  (format is known) <br>
    ///         bit 1: Is Filled    (main data is collected) <br>
    ///         bit 2: Is Updated   (some data have beed updated, example: duration for a real time MPEG-TS stream) <br>
    ///         bit 3: Is Finalized (No more data is needed, will not use further data) <br>
    ///         bit 4-15: Reserved <br>
    ///         bit 16-31: User defined
    pub fn open_buffer_continue(&self, buffer: &[u8], buffer_size: usize) -> usize {
        let buffer_ptr = buffer.as_ptr();
        unsafe {
            MediaInfoA_Open_Buffer_Continue(
                self.handle,
                buffer_ptr as *mut MediaInfo_int8u,
                buffer_size,
            )
        }
    }

    /// Open a stream and collect information about it (technical information and tags)
    ///
    /// @brief Open a stream (Get the needed file Offset)
    ///
    /// @return the needed offset of the file <br>
    ///         File size if no more bytes are needed
    pub fn open_buffer_continue_goto_get(&self) -> u64 {
        unsafe { MediaInfoA_Open_Buffer_Continue_GoTo_Get(self.handle) }
    }

    /// Open a stream and collect information about it (technical information and tags)
    ///
    /// @brief Open a stream (Finalize)
    ///
    /// @retval 0 failed
    ///
    /// @retval 1 succeed
    pub fn open_buffer_finalize(&self) -> usize {
        unsafe { MediaInfoA_Open_Buffer_Finalize(self.handle) }
    }

    /// If Open() is used in "PerPacket" mode, parse only one packet and return
    ///
    /// @brief Read one packet (if "PerPacket" mode is set)
    ///
    /// @return a bitfield <br>
    ///         bit 0: A packet was read
    pub fn open_next_packet(&self) -> usize {
        unsafe { MediaInfoA_Open_NextPacket(self.handle) }
    }

    /// Close a file opened before with Open() (without saving)
    ///
    /// @brief Close a file
    ///
    /// @warning without have saved before, modifications are lost
    pub fn close(&self) {
        unsafe { MediaInfoA_Close(self.handle) }
    }

    /// Get all details about a file in one string
    ///
    /// @brief Get all details about a file
    ///
    /// @pre You can change default presentation with option("Inform", ...)
    ///
    /// @return Text with information about the file
    pub fn inform(&self) -> String {
        unsafe {
            let ret = MediaInfoA_Inform(self.handle, 0);
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Get a piece of information about a file (parameter is a string)
    ///
    /// @brief Get a piece of information about a file (parameter is a string)
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in Kind of stream (first, second...)
    ///
    /// @param Parameter Parameter you are looking for in the stream (Codec, width, bitrate...), in string format ("Codec", "Width"...) <br>
    ///        See MediaInfo::Option("Info_Parameters") to have the full list
    ///
    /// @param InfoKind Kind of information you want about the parameter (the text, the measure, the help...)
    ///
    /// @param SearchKind Where to look for the parameter
    ///
    /// @return a string about information you search <br>
    ///         an empty string if there is a problem
    pub fn get(
        &self,
        stream_kind: MediaInfo_stream_t,
        stream_number: usize,
        parameter: &str,
        info_kind: MediaInfo_info_t,
        search_kind: MediaInfo_info_t,
    ) -> String {
        unsafe {
            let ret = MediaInfoA_Get(
                self.handle,
                stream_kind,
                stream_number,
                CString::new(parameter).unwrap().as_ptr(),
                info_kind,
                search_kind,
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Get a piece of information about a file (parameter is an integer)
    ///
    /// @brief Get a piece of information about a file (parameter is an integer)
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in Kind of stream (first, second...)
    ///
    /// @param Parameter Parameter you are looking for in the stream (Codec, width, bitrate...), in integer format (first parameter, second parameter...) <br>
    ///        This integer is arbitarily assigned by the library, so its consistency should not be relied on, but is useful when looping through all the parameters
    ///
    /// @param InfoKind Kind of information you want about the parameter (the text, the measure, the help...)
    ///
    /// @return a string about information you search <br>
    ///         an empty string if there is a problem
    pub fn get_i(
        &self,
        stream_kind: MediaInfo_stream_t,
        stream_number: usize,
        parameter: usize,
        info_kind: MediaInfo_info_t,
    ) -> String {
        unsafe {
            let ret = MediaInfoA_GetI(
                self.handle,
                stream_kind,
                stream_number,
                parameter,
                info_kind,
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Configure or get information about MediaInfoLib
    ///
    /// @param Option The name of option
    ///
    /// @param Value The value of option
    ///
    /// @return Depend of the option: by default "" (nothing) means No, other means Yes
    pub fn option(&self, option: &str, value: &str) -> String {
        unsafe {
            let ret = MediaInfoA_Option(
                self.handle,
                CString::new(option).unwrap().as_ptr(),
                CString::new(value).unwrap().as_ptr(),
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// @brief Count of streams of a stream kind (StreamNumber not filled), or count of piece of information in this stream
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in this kind of stream (first, second...)
    ///
    /// @return The count of fields for this stream kind / stream number if stream number is provided, else the count of streams for this stream kind
    pub fn count_get(&self, stream_kind: MediaInfo_stream_t, stream_number: usize) -> usize {
        unsafe { MediaInfoA_Count_Get(self.handle, stream_kind, stream_number) }
    }

    /// @brief Get the state of the library
    ///
    /// @retval <1000 No information is available for the file yet
    ///
    /// @retval >=1000<=5000 Only local (into the file) information is available, getting Internet information (titles only) is no finished yet
    ///
    /// @retval 5000 (only if Internet connection is accepted) User interaction is needed (use Option() with "Internet_Title_Get") <br>
    ///              Warning: even there is only one possible, user interaction (or the software) is needed
    ///
    /// @retval >5000<10000 Only local (into the file) information is available, getting Internet information (all) is no finished yet
    ///
    /// @retval =10000 Done
    pub fn state_get(&self) -> usize {
        unsafe { MediaInfoA_State_Get(self.handle) }
    }

    /// Output the written size when "File_Duplicate" option is used.
    ///
    /// @brief Output the written size when "File_Duplicate" option is used.
    ///
    /// @param Value The unique name of the duplicated stream (begin with "memory://")
    ///
    /// @return The size of the used buffer
    pub fn output_buffer_get(&self, value: &str) -> usize {
        unsafe { MediaInfoA_Output_Buffer_Get(self.handle, CString::new(value).unwrap().as_ptr()) }
    }

    /// Output the written size when "File_Duplicate" option is used.
    ///
    /// @brief Output the written size when "File_Duplicate" option is used.
    ///
    /// @param Pos The order of calling
    ///
    /// @return The size of the used buffer
    pub fn output_buffer_get_i(&self, pos: usize) -> usize {
        unsafe { MediaInfoA_Output_Buffer_GetI(self.handle, pos) }
    }
}

impl Default for MediaInfo {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for MediaInfo {
    fn drop(&mut self) {
        unsafe { MediaInfoA_Delete(self.handle) }
    }
}

pub struct MediaInfoList {
    handle: *mut ::std::os::raw::c_void,
}

impl MediaInfoList {
    pub fn new() -> Self {
        let handle = unsafe { MediaInfoListA_New() };
        unsafe {
            MediaInfoListA_Option(
                handle,
                CString::new("CharSet").unwrap().as_ptr(),
                CString::new("UTF-8").unwrap().as_ptr(),
            );
            MediaInfoListA_Option(
                handle,
                CString::new("setlocale_LC_CTYPE").unwrap().as_ptr(),
                CString::new("").unwrap().as_ptr(),
            );
        }
        Self { handle }
    }

    /// Open one or more files and collect information about them (technical information and tags)
    ///
    /// @brief Open files
    ///
    /// @param File Full name of file(s) to open <br>
    ///             or Full name of folder(s) to open
    ///
    /// @param Options:  FileOption_Nothing = no option <br>
    ///                  FileOption_NoRecursive = Recursive mode for folders <br>
    ///                  FileOption_CloseAll = Close all already opened files before
    ///
    /// @return Number of files successfuly added
    pub fn open(&self, files: &str, options: MediaInfo_fileoptions_t) -> usize {
        unsafe { MediaInfoListA_Open(self.handle, CString::new(files).unwrap().as_ptr(), options) }
    }

    /// Open a Buffer (Begin and end of the stream) and collect information about it (technical information and tags)
    ///
    /// @brief Open a buffer
    ///
    /// @param Begin First bytes of the buffer
    ///
    /// @param Begin_Size Size of Begin
    ///
    /// @param End Last bytes of the buffer
    ///
    /// @param End_Size Size of End
    ///
    /// @param File_Size Total size of the file
    ///
    /// @retval 0 File not opened
    ///
    /// @retval 1 File opened
    pub fn open_buffer(
        &self,
        begin: &[u8],
        begin_size: usize,
        end: &[u8],
        end_size: usize,
    ) -> usize {
        let begin_ptr = begin.as_ptr();
        let end_ptr = end.as_ptr();
        unsafe {
            MediaInfoListA_Open_Buffer(
                self.handle,
                begin_ptr as *mut MediaInfo_int8u,
                begin_size,
                end_ptr as *mut MediaInfo_int8u,
                end_size,
            )
        }
    }

    /// Close a file opened before with Open() (without saving)
    ///
    /// @brief Close a file
    ///
    /// @param FilePos File position <br>
    ///        (you can know the position in searching the filename with MediaInfoList::Get(FilePos, 0, 0, "CompleteName") ) <br>
    ///        usize::MAX to close all files
    ///
    /// @warning without have saved before, modifications are lost
    pub fn close(&self, file_pos: usize) {
        unsafe { MediaInfoListA_Close(self.handle, file_pos) }
    }

    /// Get all details about a file in one string
    ///
    /// @brief Get all details about a file
    ///
    /// @param FilePos File position <br>
    ///        (you can know the position in searching the filename with MediaInfoList::Get(FilePos, 0, 0, "CompleteName") ) <br>
    ///        usize::MAX for all files
    ///
    /// @param Reserved Deprecated, do not use it anymore
    ///
    /// @pre You can change default presentation with Inform_Set()
    ///
    /// @return Text with information about the file
    pub fn inform(&self, file_pos: usize) -> String {
        unsafe {
            let ret = MediaInfoListA_Inform(self.handle, file_pos, 0);
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Get a piece of information about a file (parameter is a string)
    ///
    /// @brief Get a piece of information about a file (parameter is a string)
    ///
    /// @param FilePos File position <br>
    ///        (you can know the position in searching the filename with MediaInfoList::Get(FilePos, 0, 0, "CompleteName") )
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in Kind of stream (first, second...)
    ///
    /// @param Parameter Parameter you are looking for in the stream (Codec, width, bitrate...), in string format ("Codec", "Width"...) <br>
    ///        See MediaInfo::Option("Info_Parameters") to have the full list
    ///
    /// @param KindOfInfo Kind of information you want about the parameter (the text, the measure, the help...)
    ///
    /// @param KindOfSearch Where to look for the parameter
    ///
    /// @return a string about information you search <br>
    ///         an empty string if there is a problem
    pub fn get(
        &self,
        file_pos: usize,
        stream_kind: MediaInfo_stream_t,
        stream_number: usize,
        parameter: &str,
        info_kind: MediaInfo_info_t,
        search_kind: MediaInfo_info_t,
    ) -> String {
        unsafe {
            let ret = MediaInfoListA_Get(
                self.handle,
                file_pos,
                stream_kind,
                stream_number,
                CString::new(parameter).unwrap().as_ptr(),
                info_kind,
                search_kind,
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Get a piece of information about a file (parameter is an integer)
    ///
    /// @brief Get a piece of information about a file (parameter is an integer)
    ///
    /// @param FilePos File position <br>
    ///        (you can know the position in searching the filename with MediaInfoList::Get(FilePos, 0, 0, "CompleteName") )
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in Kind of stream (first, second...)
    ///
    /// @param Parameter Parameter you are looking for in the stream (Codec, width, bitrate...), in integer format (first parameter, second parameter...)
    ///
    /// @param KindOfInfo Kind of information you want about the parameter (the text, the measure, the help...)
    ///
    /// @return a string about information you search <br>
    ///         an empty string if there is a problem
    pub fn get_i(
        &self,
        file_pos: usize,
        stream_kind: MediaInfo_stream_t,
        stream_number: usize,
        parameter: usize,
        info_kind: MediaInfo_info_t,
    ) -> String {
        unsafe {
            let ret = MediaInfoListA_GetI(
                self.handle,
                file_pos,
                stream_kind,
                stream_number,
                parameter,
                info_kind,
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// Configure or get information about MediaInfoLib
    ///
    /// @param Option The name of option
    ///
    /// @param Value The value of option
    ///
    /// @return Depend of the option: by default "" (nothing) means No, other means Yes
    ///
    /// @post Known options are: See MediaInfo::Option()
    pub fn option(&self, option: &str, value: &str) -> String {
        unsafe {
            let ret = MediaInfoListA_Option(
                self.handle,
                CString::new(option).unwrap().as_ptr(),
                CString::new(value).unwrap().as_ptr(),
            );
            CStr::from_ptr(ret).to_str().unwrap().to_string()
        }
    }

    /// @brief Count of streams, or count of piece of information in this stream
    ///
    /// @param FilePos File position <br>
    ///        (you can know the position in searching the filename with MediaInfoList::Get(FilePos, 0, 0, "CompleteName") )
    ///
    /// @param StreamKind Kind of stream (general, video, audio...)
    ///
    /// @param StreamNumber Stream number in this kind of stream (first, second...)
    ///
    /// @return The count of fields for this stream kind / stream number if stream number is provided, else the count of streams for this stream kind
    ///
    pub fn count_get(
        &self,
        file_pos: usize,
        stream_kind: MediaInfo_stream_t,
        stream_number: usize,
    ) -> usize {
        unsafe { MediaInfoListA_Count_Get(self.handle, file_pos, stream_kind, stream_number) }
    }

    /// @brief Get the count of opened files
    ///
    /// @return Count of files opened
    pub fn count_get_files(&self) -> usize {
        unsafe { MediaInfoListA_Count_Get_Files(self.handle) }
    }

    /// @brief Get the state of the library
    ///
    /// @retval <1000 No information is available for the file yet
    ///
    /// @retval >=1000<=5000 Only local (into the file) information is available, getting Internet information (titles only) is no finished yet
    ///
    /// @retval 5000 (only if Internet connection is accepted) User interaction is needed (use Option() with "Internet_Title_Get") <br>
    ///              Warning: even there is only one possible, user interaction (or the software) is needed
    ///
    /// @retval >5000<10000 Only local (into the file) information is available, getting Internet information (all) is no finished yet
    ///
    /// @retval =10000 Done
    pub fn state_get(&self) -> usize {
        unsafe { MediaInfoListA_State_Get(self.handle) }
    }
}

impl Default for MediaInfoList {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for MediaInfoList {
    fn drop(&mut self) {
        unsafe { MediaInfoListA_Delete(self.handle) }
    }
}

#[cfg(test)]
mod test_mediainfo {
    use crate::*;
    use sequential_test::sequential;
    use std::{fs::File, io::Write};

    static PNG_BYTES: &[u8] = &[
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44,
        0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90,
        0x77, 0x53, 0xDE, 0x00, 0x00, 0x00, 0x0F, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x01, 0x04,
        0x00, 0xFB, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x05, 0xFE, 0x02, 0xFE, 0x49, 0x66, 0x6E, 0x2B,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    ];

    #[test]
    #[sequential]
    fn test_option() {
        let mi = MediaInfo::new();
        let ret = mi.option("info_url", "");
        assert_eq!(ret, "http://MediaArea.net/MediaInfo");
    }

    #[test]
    #[sequential]
    fn test_open() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        let ret = mi.open("target/test.png");
        assert_eq!(ret, 1);
    }

    #[test]
    #[sequential]
    fn test_inform() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/test.png");
        mi.option("Inform", "General;%Format/Info%");
        let ret = mi.inform();
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_unicode() {
        let mut file = File::create("target/测试 テスト 테스트.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/测试 テスト 테스트.png");
        mi.option("Inform", "General;%CompleteName%");
        let ret = mi.inform();
        assert_eq!(ret, "target/测试 テスト 테스트.png");
    }

    #[test]
    #[sequential]
    fn test_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/test.png");
        let ret = mi.get(
            MediaInfo_stream_t::MediaInfo_Stream_General,
            0,
            "Format/Info",
            MediaInfo_info_t::MediaInfo_Info_Text,
            MediaInfo_info_t::MediaInfo_Info_Name,
        );
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_get_i() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/test.png");
        let ret = mi.get_i(
            MediaInfo_stream_t::MediaInfo_Stream_General,
            0,
            62,
            MediaInfo_info_t::MediaInfo_Info_Text,
        );
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_count_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/test.png");
        let ret = mi.count_get(MediaInfo_stream_t::MediaInfo_Stream_Image, usize::MAX);
        assert_eq!(ret, 1);
        let ret2 = mi.count_get(MediaInfo_stream_t::MediaInfo_Stream_Image, 0);
        assert!(ret2 > 100);
    }

    #[test]
    #[sequential]
    fn test_close() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        mi.open("target/test.png");
        let ret = mi.count_get(MediaInfo_stream_t::MediaInfo_Stream_Image, usize::MAX);
        assert_eq!(ret, 1);
        mi.close();
        let ret2 = mi.count_get(MediaInfo_stream_t::MediaInfo_Stream_Image, usize::MAX);
        assert_eq!(ret2, 0);
    }

    #[test]
    #[sequential]
    fn test_state_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfo::new();
        let ret = mi.state_get();
        assert!(ret < 1000);
        mi.open("target/test.png");
        let ret2 = mi.state_get();
        assert_eq!(ret2, 10000);
    }

    #[test]
    #[sequential]
    fn test_buffer() {
        let file_size = PNG_BYTES.len() as u64;
        let mut ptr = 0;
        let mi = MediaInfo::new();
        mi.open_buffer_init(file_size, 0);

        loop {
            let buffer_size = if file_size - ptr > 10 {
                10
            } else {
                file_size - ptr
            };
            let buffer = &PNG_BYTES[ptr as usize..(ptr + buffer_size) as usize];
            let status = mi.open_buffer_continue(buffer, buffer_size as usize);
            if status & 0x08 == 0x08 {
                break;
            }
            if mi.open_buffer_continue_goto_get() != u64::MAX {
                ptr = mi.open_buffer_continue_goto_get();
                mi.open_buffer_init(file_size, ptr);
            } else {
                ptr += buffer_size;
            }
            if buffer_size == 0 {
                break;
            }
        }

        mi.open_buffer_finalize();
        let ret = mi.get(
            MediaInfo_stream_t::MediaInfo_Stream_General,
            0,
            "Format/Info",
            MediaInfo_info_t::MediaInfo_Info_Text,
            MediaInfo_info_t::MediaInfo_Info_Name,
        );
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_list_option() {
        let mi = MediaInfoList::new();
        let ret = mi.option("info_url", "");
        assert_eq!(ret, "http://MediaArea.net/MediaInfo");
    }

    #[test]
    #[sequential]
    fn test_list_open() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        let ret = mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        assert_eq!(ret, 1);
        let ret = mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        assert_eq!(ret, 2);
    }

    #[test]
    #[sequential]
    fn test_list_close_count_get_files() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        for i in 0..10 {
            mi.open(
                "target/test.png",
                MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
            );
        }
        let ret = mi.count_get_files();
        assert_eq!(ret, 10);
        mi.close(2);
        mi.close(5);
        let ret = mi.count_get_files();
        assert_eq!(ret, 8);
        mi.close(usize::MAX);
        let ret = mi.count_get_files();
        assert_eq!(ret, 0);
    }

    #[test]
    #[sequential]
    fn test_list_inform_unicode() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();
        let mut file = File::create("target/测试 テスト 테스트.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        mi.open(
            "target/测试 テスト 테스트.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        mi.option("Inform", "General;%CompleteName% ");
        let ret = mi.inform(usize::MAX);
        assert_eq!(ret, "target/test.png target/测试 テスト 테스트.png ");
        let ret = mi.inform(0);
        assert_eq!(ret, "target/test.png ");
        let ret = mi.inform(1);
        assert_eq!(ret, "target/测试 テスト 테스트.png ");
    }

    #[test]
    #[sequential]
    fn test_list_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        let ret = mi.get(
            0,
            MediaInfo_stream_t::MediaInfo_Stream_General,
            0,
            "Format/Info",
            MediaInfo_info_t::MediaInfo_Info_Text,
            MediaInfo_info_t::MediaInfo_Info_Name,
        );
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_list_get_i() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        let ret = mi.get_i(
            0,
            MediaInfo_stream_t::MediaInfo_Stream_General,
            0,
            62,
            MediaInfo_info_t::MediaInfo_Info_Text,
        );
        assert_eq!(ret, "Portable Network Graphic");
    }

    #[test]
    #[sequential]
    fn test_list_count_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        let ret = mi.count_get(0, MediaInfo_stream_t::MediaInfo_Stream_Image, usize::MAX);
        assert_eq!(ret, 1);
        let ret2 = mi.count_get(0, MediaInfo_stream_t::MediaInfo_Stream_Image, 0);
        assert!(ret2 > 100);
    }

    #[test]
    #[sequential]
    fn test_list_state_get() {
        let mut file = File::create("target/test.png").unwrap();
        file.write_all(PNG_BYTES).unwrap();

        let mi = MediaInfoList::new();
        let ret = mi.state_get();
        assert!(ret < 1000);
        mi.open(
            "target/test.png",
            MediaInfo_fileoptions_t::MediaInfo_FileOption_Nothing,
        );
        let ret2 = mi.state_get();
        assert_eq!(ret2, 10000);
    }
}
