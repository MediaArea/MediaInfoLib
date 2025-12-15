/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

use std::path::{Path, PathBuf};
use std::{env, fs};

fn main() {
    // Tell cargo to look for shared libraries in the specified directory
    let dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let target_arch =
        std::env::var("CARGO_CFG_TARGET_ARCH").expect("CARGO_CFG_TARGET_ARCH not set");
    let target_os =
        std::env::var("CARGO_CFG_TARGET_OS").expect("CARGO_CFG_TARGET_OS not found in environment");
    let arch = match target_arch.as_str() {
        "x86" => "Win32",
        "x86_64" => "x64",
        "aarch64" => "ARM64",
        _ => "",
    };
    println!(
        "cargo:rustc-link-search=native={}",
        Path::new(&dir)
            .join("../../../Project/MSVC2026")
            .join(arch)
            .join("Release")
            .display()
    );
    println!(
        "cargo:rustc-link-search=native={}",
        Path::new(&dir)
            .join("../../../Project/MSVC2022")
            .join(arch)
            .join("Release")
            .display()
    );
    println!("cargo:rustc-link-search=native=/usr/local/lib");

    // Tell cargo to tell rustc to link the shared library.
    println!("cargo:rustc-link-lib=mediainfo");

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate bindings for.
        .header(
            Path::new(&dir)
                .join("../MediaInfoDLL_Static.h")
                .display()
                .to_string(),
        )
        // Use a regex to apply `rustified_enum()` only to enums
        // that start with "MediaInfo_".
        .rustified_enum("^MediaInfo_.*")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    // Copy DLL to executable directory so that we can 'cargo run' right away
    if target_os == "windows" {
        let dll_name = "MediaInfo.dll";
        let possible_paths = [
            // For release package
            Path::new(&dir).join("../../../..").join(dll_name),
            // For self-built
            Path::new(&dir)
                .join("../../../Project/MSVC2026")
                .join(arch)
                .join("Release")
                .join(dll_name),
            Path::new(&dir)
                .join("../../../Project/MSVC2022")
                .join(arch)
                .join("Release")
                .join(dll_name),
        ];
        let dll_source_path = possible_paths
            .iter()
            .find(|&p| p.exists())
            .expect("Could not find the DLL in any of the specified folders");
        let dll_dest_path = out_path.join("../../..").join(dll_name);
        fs::copy(dll_source_path, dll_dest_path).expect("Failed to copy DLL");
    }
}
