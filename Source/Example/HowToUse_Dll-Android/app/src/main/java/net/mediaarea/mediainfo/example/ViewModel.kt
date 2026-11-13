package net.mediaarea.mediainfo.example

import android.app.Application
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.os.ParcelFileDescriptor
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.core.graphics.createBitmap
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.mediaarea.mediainfo.MediaInfo
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

data class MediaInfoState(
    val version: String = "Loading...",
    val inform: String = "Loading...",
    val parameters: String = "Loading...",
    val codecs: String = "Loading...",
    val nativeOutput: String = "Loading..."
)

class MediaViewModel(application: Application) : AndroidViewModel(application) {
    var uiState by mutableStateOf(MediaInfoState())
        private set

    init {
        loadData()
    }

    private fun loadData() {
        viewModelScope.launch(Dispatchers.IO) {
            val context = getApplication<Application>()

            // Create sample file
            val path = createSamplePng(context)

            // Kotlin interface
            // Using net.mediaarea.mediainfo Kotlin JNI wrapper
            val mi = MediaInfo()
            val version = mi.Option("Info_Version")
            val parameters = mi.Option("Info_Parameters")
            val codecs = mi.Option("Info_Codecs")
            mi.Option("Language", "  Config_Text_ColumnSize;20")
            if (path != null) {
                val file = File(path)
                val pfd = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY)
                val fd = pfd.detachFd()
                mi.Open(fd, "sample.png")
            }
            val info = mi.Inform()

            // C++ interface
            // Bridge is just an example bridge that receives output from C++ code
            val bridge = Bridge()
            var output = bridge.Version()
            output += bridge.Inform(path ?: "")
            output += bridge.Info()

            withContext(Dispatchers.Main) {
                uiState = MediaInfoState(
                    version = version,
                    inform = info,
                    parameters = parameters,
                    codecs = codecs,
                    nativeOutput = output
                )
            }
        }
    }

    private fun createSamplePng(context: Context): String? {
        val bitmap = createBitmap(100, 100)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.GRAY)

        val file = File(context.filesDir, "sample.png")

        try {
            FileOutputStream(file).use { out ->
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
                return file.absolutePath
            }
        } catch (e: IOException) {
            e.printStackTrace()
            return null
        }
    }
}
