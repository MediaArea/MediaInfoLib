plugins {
    id("com.android.library")
}

val consolidateHeaders by tasks.registering(Copy::class) {
    from("../../../Source/MediaInfo") {
        include(
            "MediaInfo.h",
            "MediaInfoList.h",
            "MediaInfo_Const.h",
            "MediaInfo_Events.h"
        )
        into("MediaInfo")
    }
    from("../../../Source/MediaInfoDLL") {
        include(
            "MediaInfoDLL.h",
            "MediaInfoDLL_Static.h",
        )
        into("MediaInfoDLL")
    }
    destinationDir = layout.buildDirectory.dir("prefabHeadersStaging").get().asFile
}

android {
    namespace = "net.mediaarea.mediainfo.lib"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        minSdk = 24

        aarMetadata {
            minCompileSdk = minSdk
        }

        @Suppress("UnstableApiUsage")
        externalNativeBuild {
            cmake {
                arguments(
                    "-DANDROID_STL=c++_shared",
                    "-DENABLE_UNICODE=ON"
                )
            }
        }

        consumerProguardFiles("consumer-rules.pro")
    }

    externalNativeBuild {
        cmake {
            path = file("CMakeLists.txt")
        }
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    buildFeatures {
        prefabPublishing = true
        prefab = true
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
    packaging {
        jniLibs {
            keepDebugSymbols.add("**/*.so")
        }
    }
    prefab {
        create("mediainfo") {
            headers = consolidateHeaders.map { it.destinationDir.absolutePath }.get()
        }
    }
}

tasks.configureEach {
    if (name.contains("prefab") && name.contains("ConfigurePackage")) {
        dependsOn(consolidateHeaders)
    }
}

dependencies {
    // Curl static build from https://github.com/vvb2060/curl-android
    implementation("io.github.vvb2060.ndk:curl:8.18.0")
}
