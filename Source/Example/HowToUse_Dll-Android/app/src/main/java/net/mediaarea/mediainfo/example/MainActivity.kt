package net.mediaarea.mediainfo.example

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.adaptive.navigationsuite.NavigationSuiteScaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.PreviewScreenSizes
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.mediaarea.mediainfo.MediaInfo
import net.mediaarea.mediainfo.example.ui.theme.MediaInfoLibExampleTheme


class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val viewModel: MediaViewModel = viewModel()
            MediaInfoLibExampleTheme {
                MediaInfoLibExampleApp(viewModel.uiState)
            }
        }
    }
}

@Composable
private fun MediaInfoLibExampleApp(state: MediaInfoState) {
    var currentDestination by rememberSaveable { mutableStateOf(AppDestinations.HOME) }

    NavigationSuiteScaffold(
        navigationSuiteItems = {
            AppDestinations.entries.forEach {
                item(
                    icon = {
                        Icon(
                            painterResource(it.icon),
                            contentDescription = it.label
                        )
                    },
                    label = { Text(it.label) },
                    selected = it == currentDestination,
                    onClick = { currentDestination = it }
                )
            }
        }
    ) {
        Scaffold(
            modifier = Modifier
                .fillMaxSize()
                .padding(10.dp)
        ) { innerPadding ->
            // Use a 'when' expression to swap the content dynamically
            when (currentDestination) {
                AppDestinations.HOME -> HomeScreen(
                    modifier = Modifier
                        .padding(innerPadding)
                        .verticalScroll(rememberScrollState())
                )

                AppDestinations.KOTLIN -> KotlinScreen(
                    modifier = Modifier
                        .padding(innerPadding)
                        .verticalScroll(rememberScrollState()),
                    state = state
                )

                AppDestinations.NATIVE -> NativeScreen(
                    modifier = Modifier
                        .padding(innerPadding)
                        .verticalScroll(rememberScrollState()),
                    state = state
                )

                AppDestinations.URL -> NetworkScreen(
                    modifier = Modifier
                        .padding(innerPadding)
                        .verticalScroll(rememberScrollState()),
                )
            }
        }
    }
}

private enum class AppDestinations(
    val label: String,
    val icon: Int,
) {
    HOME("Home", R.drawable.ic_home),
    KOTLIN("Kotlin", R.drawable.android_48px),
    NATIVE("Native (C++)", R.drawable.code_48px),
    URL("URL", R.drawable.http_48px),
}

@Composable
private fun HomeScreen(modifier: Modifier = Modifier) {
    Column(
        modifier = modifier.padding(20.dp)
    ) {
        Text(
            text = "MediaInfoLib Android Example App",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        Text(
            text = "This app features dedicated navigation pages showcasing MediaInfoLib Android library's dual-language integration paths as well as Curl integration. " +
                    "Explore the Kotlin page to see the included JNI wrapper in action, or switch to the Native page to see how MediaInfoLib integrates directly into C/C++ workflows using Prefab and CMake. " +
                    "Check out the URL page to try out MediaInfoLib's native Curl (libcurl) integration by analyzing a file from a URL.",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onBackground,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

@Composable
private fun KotlinScreen(modifier: Modifier = Modifier, state: MediaInfoState) {
    // Using net.mediaarea.mediainfo Kotlin JNI wrapper
    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(12.dp) // Adds space between the cards
    ) {
        InfoCard(title = "Info Version", value = state.version)
        InfoCard(
            title = "Inform",
            value = state.inform,
            fontFamily = FontFamily.Monospace,
        )
        InfoCard(title = "Info Parameters", value = state.parameters)
        InfoCard(title = "Info Codecs", value = state.codecs)
    }
}

@Composable
private fun NativeScreen(modifier: Modifier = Modifier, state: MediaInfoState) {
    Text(
        text = state.nativeOutput,
        style = MaterialTheme.typography.bodySmall,
        fontFamily = FontFamily.Monospace,
        modifier = modifier
    )
}

@Composable
private fun NetworkScreen(modifier: Modifier = Modifier) {
    var url by remember { mutableStateOf("https://raw.githubusercontent.com/MediaArea/MediaInfoLib/refs/heads/master/Source/Resource/Image/MediaInfo.ico") }
    var result by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }

    // Create a coroutine scope tied to this composable's lifecycle
    val scope = rememberCoroutineScope()

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        OutlinedTextField(
            value = url,
            onValueChange = { url = it },
            label = { Text("Enter URL") },
            modifier = Modifier.fillMaxWidth(),
            enabled = !isLoading
        )
        Button(
            onClick = {
                if (url.isNotBlank()) {
                    isLoading = true
                    result = ""

                    // Launch background worker thread
                    scope.launch {
                        val returned = withContext(Dispatchers.IO) {
                            val mi = MediaInfo()
                            mi.Option("Language", "  Config_Text_ColumnSize;20")
                            mi.Open(url)
                            mi.Inform()
                        }
                        result = returned
                        isLoading = false
                    }
                }
            },
            modifier = Modifier.fillMaxWidth(),
            enabled = !isLoading && url.isNotBlank()
        ) {
            Text(if (isLoading) "Processing..." else "Process URL")
        }
        if (isLoading) {
            CircularProgressIndicator(
                modifier = Modifier.align(Alignment.CenterHorizontally)
            )
        }
        if (result.isNotEmpty()) {
            InfoCard(
                title = "MediaInfoLib Inform Text Output",
                value = result,
                fontFamily = FontFamily.Monospace
            )
        }
    }
}

@Composable
private fun InfoCard(
    title: String,
    value: String,
    modifier: Modifier = Modifier,
    fontFamily: FontFamily = FontFamily.Default,
) {
    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        ),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(
            modifier = Modifier
                .padding(16.dp)
                .fillMaxWidth()
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                text = value.ifEmpty { "None detected" },
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                fontFamily = fontFamily
            )
        }
    }
}

@PreviewScreenSizes
@Composable
private fun MediaInfoMultiSizePreview() {
    val mockState = MediaInfoState(
        version = "",
        inform = "",
        parameters = "",
        codecs = ""
    )
    MediaInfoLibExampleTheme {
        MediaInfoLibExampleApp(mockState)
    }
}
