/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Example for MediaInfoLib
// Command line version
//
// Execute 'cargo run --release' to build and run with default 'Example.ogg'
// or 'cargo run --release -- <path to a file>' to use other file
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

use mediainfo_rs::{
    MediaInfo, MediaInfo_fileoptions_t::*, MediaInfo_info_t::*, MediaInfo_stream_t::*,
    MediaInfoList,
};
use std::{
    env,
    fs::File,
    io::{self, Read, Seek, SeekFrom, Write},
};

fn main() {
    // Get optional command-line argument for filename
    let args: Vec<String> = env::args().collect();
    let optional_arg = if args.len() > 1 { Some(&args[1]) } else { None };
    let file = match optional_arg {
        Some(arg_value) => arg_value,   //Use user-provided file if any
        None => "../../../Example.ogg", //else use Example file
    };

    // Information about MediaInfo
    let mi = MediaInfo::new();
    let mut to_display = mi.option("Info_Version", "0.7.13;MediaInfoDLL_Example_MSVC;0.7.13");

    to_display += "\r\n\r\nInfo_Parameters\r\n";
    to_display += &mi.option("Info_Parameters", "");

    to_display += "\r\n\r\nInfo_Codecs\r\n";
    to_display += &mi.option("Info_Codecs", "");

    // An example of how to use the library
    to_display += "\r\n\r\nOpen\r\n";
    mi.open(file);

    to_display += "\r\n\r\nInform with Complete=false\r\n";
    mi.option("Complete", "");
    to_display += &mi.inform();

    to_display += "\r\n\r\nInform with Complete=true\r\n";
    mi.option("Complete", "1");
    to_display += &mi.inform();

    to_display += "\r\n\r\nCustom Inform\r\n";
    mi.option("Inform", "General;Example : FileSize=%FileSize%");
    to_display += &mi.inform();

    to_display += "\r\n\r\nGet with Stream=General and Parameter=\"FileSize\"\r\n";
    to_display += &mi.get(
        MediaInfo_Stream_General,
        0,
        "FileSize",
        MediaInfo_Info_Text,
        MediaInfo_Info_Name,
    );

    to_display += "\r\n\r\nGetI with Stream=General and Parameter=90\r\n";
    to_display += &mi.get_i(MediaInfo_Stream_General, 0, 90, MediaInfo_Info_Text);

    to_display += "\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n";
    to_display += &mi.count_get(MediaInfo_Stream_Audio, usize::MAX).to_string();

    to_display += "\r\n\r\nGet with Stream=General and Parameter=\"AudioCount\"\r\n";
    to_display += &mi.get(
        MediaInfo_Stream_General,
        0,
        "AudioCount",
        MediaInfo_Info_Text,
        MediaInfo_Info_Name,
    );

    to_display += "\r\n\r\nGet with Stream=General and Parameter=\"StreamCount\"\r\n";
    to_display += &mi.get(
        MediaInfo_Stream_General,
        0,
        "StreamCount",
        MediaInfo_Info_Text,
        MediaInfo_Info_Name,
    );

    to_display += "\r\n\r\nClose\r\n";
    mi.close();

    println!("{to_display}");

    // By buffer example
    by_buffer_example(file).unwrap();

    // MediaInfoList example
    //mediainfo_list();

    // File_Duplicate by buffer example
    //file_duplicate_buffer().unwrap();
}

//***************************************************************************
// By buffer example
//***************************************************************************
#[allow(dead_code)] //suppress warnings when this function is not called by main
fn by_buffer_example(file: &str) -> io::Result<()> {
    //From: preparing an example file for reading
    let mut f = File::open(file)?;

    //From: preparing a memory buffer for reading
    let mut from_buffer = [0; 7 * 188]; //Note: you can do your own buffer
    let mut from_buffer_size; //The size of the read file buffer

    //From: retrieving file size
    f.seek(SeekFrom::End(0))?;
    let f_size = f.stream_position()?;
    f.seek(SeekFrom::Start(0))?;

    //Initializing MediaInfo
    let mi = MediaInfo::new();

    //Preparing to fill MediaInfo with a buffer
    mi.open_buffer_init(f_size, 0);

    //The parsing loop
    loop {
        //Reading data somewhere, do what you want for this.
        from_buffer_size = f.read(&mut from_buffer)?;

        //Sending the buffer to MediaInfo
        let status = mi.open_buffer_continue(&from_buffer, from_buffer_size);
        if status & 0x08 == 0x08 {
            //Bit3=Finished
            break;
        }

        //Testing if there is a MediaInfo request to go elsewhere
        if mi.open_buffer_continue_goto_get() != u64::MAX {
            f.seek(SeekFrom::Start(mi.open_buffer_continue_goto_get()))?; //Position the file
            mi.open_buffer_init(f_size, f.stream_position()?); //Informing MediaInfo we have seek
        }

        if from_buffer_size == 0 {
            break;
        }
    }

    //Finalizing
    mi.open_buffer_finalize(); //This is the end of the stream, MediaInfo must finnish some work

    //Get() example
    let to_display = mi.get(
        MediaInfo_Stream_General,
        0,
        "Format",
        MediaInfo_Info_Text,
        MediaInfo_Info_Name,
    );

    println!("{to_display}");

    Ok(())
}

//***************************************************************************
// MediaInfoList example
//***************************************************************************
#[allow(dead_code)] //suppress warnings when this function is not called by main
fn mediainfo_list() {
    // New instance of MediaInfoList
    let mi = MediaInfoList::new();

    // Open a folder containing multiple media files (change the path to a folder containing files before running)
    // You can also open multiple files/folders, one file/folder at a time, with multiple calls to mi.open
    mi.open(r#"C:\Windows\Web\Wallpaper"#, MediaInfo_FileOption_Nothing);

    // Get count of files opened
    let file_count = mi.count_get_files();
    println!("\n{file_count} files were opened.\n");

    // Show some information about the files
    // You can use similar methods as MediaInfo but most methods now have a file_pos parameter.
    println!(
        "{: <50} | {: <10} | {: >10} | {: >10} | {: >10}",
        "Filename", "Format", "File size", "Width", "Height"
    );
    println!(
        "------------------------------------------------------------------------------------------------------"
    );
    for i in 0..file_count {
        println!(
            "{: <50} | {: <10} | {: >10} | {: >10} | {: >10}",
            mi.get(
                i,
                MediaInfo_Stream_General,
                0,
                "FileName",
                MediaInfo_Info_Text,
                MediaInfo_Info_Name,
            ),
            mi.get(
                i,
                MediaInfo_Stream_General,
                0,
                "Format",
                MediaInfo_Info_Text,
                MediaInfo_Info_Name,
            ),
            mi.get(
                i,
                MediaInfo_Stream_General,
                0,
                "FileSize_String",
                MediaInfo_Info_Text,
                MediaInfo_Info_Name,
            ),
            mi.get(
                i,
                if mi.count_get(i, MediaInfo_Stream_Video, usize::MAX) > 0 {
                    MediaInfo_Stream_Video
                } else {
                    MediaInfo_Stream_Image
                },
                0,
                "Width",
                MediaInfo_Info_Text,
                MediaInfo_Info_Name,
            ),
            mi.get(
                i,
                if mi.count_get(i, MediaInfo_Stream_Video, usize::MAX) > 0 {
                    MediaInfo_Stream_Video
                } else {
                    MediaInfo_Stream_Image
                },
                0,
                "Height",
                MediaInfo_Info_Text,
                MediaInfo_Info_Name,
            )
        );
    }

    // Close a file (0 means the first file in the list)
    mi.close(0);
    println!(
        "\n{} files remain open after closing a file.\n",
        mi.count_get_files()
    );

    // Close all files
    mi.close(usize::MAX);
    println!(
        "{} files remain open after closing all files.\n",
        mi.count_get_files()
    );
}

//***************************************************************************
// File_Duplicate by buffer example
// https://mediaarea.net/en/MediaInfo/Support/SDK/Duplicate
//***************************************************************************
#[allow(dead_code)] //suppress warnings when this function is not called by main
fn file_duplicate_buffer() -> io::Result<()> {
    //From: preparing an example file for reading
    //For this example, MPTS.ts is a file generated with:
    //  ffmpeg -f lavfi -i "testsrc=duration=10:size=1280x720:rate=25" \
    //    -f lavfi -i "sine=frequency=440:duration=10" \
    //    -f lavfi -i "testsrc=duration=10:size=640x480:rate=30" \
    //    -f lavfi -i "sine=frequency=880:duration=10" \
    //    -map 0:v:0 -map 1:a:0 -map 2:v:0 -map 3:a:0 \
    //    -program title=ProgOne:st=0:st=1 -program title=ProgTwo:st=2:st=3 \
    //    -c:v libx264 -c:a aac -f mpegts MPTS.ts
    let mut f = File::open("MPTS.ts")?;

    //From: preparing a memory buffer for reading
    let mut from_buffer = [0u8; 102400]; //Note: you can do your own buffer
    let mut from_buffer_size; //The size of the read file buffer

    //To: preparing a memory buffer for writing
    let to_buffer = [0u8; 102400]; //Note: you can do your own buffer
    let mut to_buffer_size_written; //The size of the output buffer that is written by MediaInfo

    //Destination file
    let mut of = File::create("MPTS_File_Duplicate.ts")?;

    //From: retrieving file size
    f.seek(SeekFrom::End(0))?;
    let f_size = f.stream_position()?;
    f.seek(SeekFrom::Start(0))?;

    //Initializing MediaInfo
    let mi = MediaInfo::new();

    //Registering for duplication
    //The streams has multiple programs, one of these program is named "1".
    //The user wants to have this program, and only this one, in the output.
    mi.option(
        "File_Duplicate",
        &format!(
            "memory://{}:{};program_number={}",
            to_buffer.as_ptr() as usize,
            to_buffer.len(),
            1
        ),
    );

    //Setting MediaInfo as parsing a non seakable file, because we want the complete stream
    mi.option("File_IsSeekable", "0");

    //Preparing to fill MediaInfo with a buffer
    mi.open_buffer_init(f_size, 0);

    //The parsing loop
    let mut can_write_only_if_parsing_is_ok = false;
    loop {
        //Reading data somewhere, do what you want for this.
        from_buffer_size = f.read(&mut from_buffer)?;

        //Sending the buffer to MediaInfo
        let status = mi.open_buffer_continue(&from_buffer, from_buffer_size);
        if status & 0x1 == 0x1 && !can_write_only_if_parsing_is_ok {
            can_write_only_if_parsing_is_ok = true;
        }

        if can_write_only_if_parsing_is_ok {
            //Retrieving data written in memory
            to_buffer_size_written = mi.output_buffer_get(&format!(
                "memory://{}:{}",
                to_buffer.as_ptr() as usize,
                to_buffer.len()
            ));

            //Writing data to somewhere, do what you want for this.
            of.write_all(&to_buffer[0..to_buffer_size_written])?;
        }

        if from_buffer_size == 0 {
            break;
        }
    }

    //Finalizing
    mi.open_buffer_finalize(); //This is the end of the stream, MediaInfo must finnish some work

    //Inform
    mi.option("Inform", ""); //reset to default due to previous changes in examnple main()
    mi.option("Complete", ""); //reset to default due to previous changes in examnple main()
    println!("\nOriginal:\n\n{}", mi.inform());
    let mi2 = MediaInfo::new();
    mi2.open("MPTS_File_Duplicate.ts");
    println!("\nDuplicated::\n\n{}", mi2.inform());

    Ok(())
}
