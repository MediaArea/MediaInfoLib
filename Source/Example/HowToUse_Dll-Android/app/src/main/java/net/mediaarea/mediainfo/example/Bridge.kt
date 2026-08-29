package net.mediaarea.mediainfo.example

class Bridge {
    companion object {
        init {
            System.loadLibrary("mediainfolib_example_bridge")
        }
    }

    external fun Version(): String
    external fun Info(): String
    external fun Inform(path: String): String
}
