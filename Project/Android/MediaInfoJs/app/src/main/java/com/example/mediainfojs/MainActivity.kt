package com.example.mediainfojs

import android.app.Activity
import android.net.Uri
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.webkit.*
import android.content.Intent
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    var callback: ValueCallback<Array<Uri>>? = null


    /*
    * Override WebViewClient::shouldInterceptRequest to respond with MediaInfoJS files from assets when they are requested
    */
    inner class WebViewClientImpl(private val activity: Activity) : WebViewClient() {
        override fun shouldInterceptRequest(view: WebView?, request: WebResourceRequest?): WebResourceResponse? {
            var response: WebResourceResponse? = null
            when {
                request?.url.toString() == "https://mi/" -> // load sample html from assets
                    response = WebResourceResponse("text/html", "utf-8", activity.assets.open("MediaInfo_DLL_JavaScript/HowToUse_Dll.html"))
                request?.url.toString() == "https://mi/MediaInfoWasm.js" -> // load mediainfo wasm javascript module from assets
                    response = WebResourceResponse("application/javascript", "utf-8", activity.assets.open("MediaInfo_DLL_JavaScript/MediaInfoWasm.js"))
                request?.url.toString() == "https://mi/MediaInfoWasm.wasm" -> // load mediainfo wasm binary from assets
                    response = WebResourceResponse("application/octet-stream", "utf-8", activity.assets.open("MediaInfo_DLL_JavaScript/MediaInfoWasm.wasm"))
                request?.url.toString() == "https://mi/MediaInfo.js" -> // load mediainfo asmjs javascript module from assets
                    response = WebResourceResponse("application/javascript", "utf-8", activity.assets.open("MediaInfo_DLL_JavaScript/MediaInfo.js"))
                request?.url.toString() == "https://mi/MediaInfo.js.mem" -> // load mediainfo asmjs memory file from assets
                    response = WebResourceResponse("application/octet-stream", "utf-8", activity.assets.open("MediaInfo_DLL_JavaScript/MediaInfo.js.mem"))

            }
            return response
        }
    }


    /*
     * Handle file selection request
    */
    inner class WebChromeClientImpl(private val activity: Activity) : WebChromeClient() {
        override fun onShowFileChooser(webView: WebView?, filePathCallback: ValueCallback<Array<Uri>>?, fileChooserParams: FileChooserParams?): Boolean {
            callback?.onReceiveValue(null) // Terminate previous request if exist
            callback = filePathCallback

            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)

            intent.addCategory(Intent.CATEGORY_OPENABLE)
            intent.type = "*/*"
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            activity.startActivityForResult(intent, MainActivity.OPEN_FILE_REQUEST)

            return true
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        WebView.setWebContentsDebuggingEnabled(true)

        web_view.webViewClient = WebViewClientImpl(this)
        web_view.webChromeClient = WebChromeClientImpl(this)
        web_view.settings.javaScriptEnabled = true
        web_view.settings.allowFileAccess = true

        // WebView automatically append a slash to this url, all url referenced in this page will be relative to https://mi/
        web_view.loadUrl("https://mi") // Chrome allow JavaScript loading only from http/https protocols
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == Activity.RESULT_OK) {
            when (requestCode) {
                OPEN_FILE_REQUEST -> {
                    if (data == null)
                        return

                    var uris: Array<Uri>? = null
                    val clipData = data.clipData
                    if (clipData != null) {
                        uris = Array(clipData.itemCount) {
                            clipData.getItemAt(it).uri
                        }
                    } else if (data.data != null) {
                        uris = Array(1) {
                            data.data
                        }
                    }
                    if (uris != null) {
                        callback?.onReceiveValue(uris)
                    }

                    callback = null
                }
            }
        }
    }
    companion object {
        const val OPEN_FILE_REQUEST = 1
    }
}