package com.il2cpp.dumper.ui.screens

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.ui.components.ConfigPanel
import com.il2cpp.dumper.ui.components.FileSelector
import com.il2cpp.dumper.ui.components.LogViewer
import com.il2cpp.dumper.ui.theme.DumpGreen
import com.il2cpp.dumper.viewmodel.DumpState
import com.il2cpp.dumper.viewmodel.DumperViewModel

@Composable
fun HomeScreen(
    viewModel: DumperViewModel,
    onNavigateToJob: (Long) -> Unit,
    modifier: Modifier = Modifier
) {
    val state by viewModel.state.collectAsState()
    val logs by viewModel.logs.collectAsState()
    val config by viewModel.config.collectAsState()
    val metadataUri by viewModel.metadataUri.collectAsState()
    val il2cppUri by viewModel.il2cppUri.collectAsState()

    val metadataLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let { viewModel.setMetadataUri(it) }
    }

    val il2cppLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let { viewModel.setIl2cppUri(it) }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .statusBarsPadding()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(
            text = "Il2Cpp Dumper",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "Select Il2Cpp binary and metadata files to dump",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        Spacer(modifier = Modifier.height(4.dp))

        FileSelector(
            title = "Il2Cpp Binary",
            subtitle = "libil2cpp.so (ELF)",
            selectedUri = il2cppUri,
            onSelect = { il2cppLauncher.launch(arrayOf("*/*")) }
        )

        FileSelector(
            title = "Global Metadata",
            subtitle = "global-metadata.dat",
            selectedUri = metadataUri,
            onSelect = { metadataLauncher.launch(arrayOf("*/*")) }
        )

        var dumpAddrText by remember { mutableStateOf("") }
        OutlinedTextField(
            value = dumpAddrText,
            onValueChange = {
                dumpAddrText = it
                val cleaned = it.trim().removePrefix("0x").removePrefix("0X")
                val addr = cleaned.toULongOrNull(16)?.toLong() ?: 0L
                viewModel.setDumpAddress(addr)
            },
            label = { Text("Dump Address (optional)") },
            placeholder = { Text("0x0 (leave empty for auto-detect)") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Ascii),
            supportingText = { Text("For dump files: enter the image base address in hex") }
        )

        ConfigPanel(
            config = config,
            onConfigChange = { viewModel.updateConfig(it) }
        )

        Spacer(modifier = Modifier.height(4.dp))

        val isRunning = state is DumpState.Initializing || state is DumpState.Searching || state is DumpState.Dumping

        Button(
            onClick = { viewModel.startDump() },
            modifier = Modifier.fillMaxWidth(),
            enabled = !isRunning
        ) {
            if (isRunning) {
                CircularProgressIndicator(
                    modifier = Modifier.size(18.dp),
                    strokeWidth = 2.dp,
                    color = MaterialTheme.colorScheme.onPrimary
                )
                Spacer(modifier = Modifier.width(8.dp))
            } else {
                Icon(
                    imageVector = Icons.Default.PlayArrow,
                    contentDescription = null
                )
                Spacer(modifier = Modifier.width(4.dp))
            }
            Text(
                text = when (state) {
                    is DumpState.Initializing -> "Initializing..."
                    is DumpState.Searching -> "Searching..."
                    is DumpState.Dumping -> "Dumping..."
                    else -> "Start Dump"
                }
            )
        }

        if (logs.isNotEmpty()) {
            Text(
                text = "Log Output",
                style = MaterialTheme.typography.titleSmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            LogViewer(logs = logs)
        }

        when (val s = state) {
            is DumpState.Success -> {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Icon(
                        Icons.Default.CheckCircle,
                        contentDescription = null,
                        tint = DumpGreen,
                        modifier = Modifier.size(24.dp)
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = "Dump completed successfully!",
                        style = MaterialTheme.typography.bodyLarge,
                        color = DumpGreen
                    )
                }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Button(
                        onClick = { onNavigateToJob(s.jobId) },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("View Job Details")
                    }
                    OutlinedButton(
                        onClick = { viewModel.reset() },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("New Dump")
                    }
                }
            }
            is DumpState.Error -> {
                Text(
                    text = "Error: ${s.message}",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.error
                )
                Button(
                    onClick = { viewModel.reset() },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Try Again")
                }
            }
            else -> {}
        }

        Spacer(modifier = Modifier.height(16.dp))
    }
}
