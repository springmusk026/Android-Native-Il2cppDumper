package com.il2cpp.dumper.ui.screens

import android.content.Intent
import android.net.Uri
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.InsertDriveFile
import androidx.compose.material.icons.automirrored.filled.OpenInNew
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Download
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.FolderZip
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.ui.components.Card
import com.il2cpp.dumper.ui.components.CircleButton
import com.il2cpp.dumper.ui.components.Hairline
import com.il2cpp.dumper.ui.components.Heading
import com.il2cpp.dumper.ui.components.MonoText
import com.il2cpp.dumper.ui.components.PrimaryPill
import com.il2cpp.dumper.ui.components.ScreenHeader
import com.il2cpp.dumper.ui.components.SecondaryPill
import com.il2cpp.dumper.ui.components.ValueRow
import com.il2cpp.dumper.ui.components.entrance
import com.il2cpp.dumper.ui.components.pressable
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.ui.theme.Typo
import com.il2cpp.dumper.util.DebugInfoCollector
import com.il2cpp.dumper.viewmodel.DumpConfig
import com.il2cpp.dumper.viewmodel.JobDetailViewModel
import kotlinx.coroutines.launch
import org.json.JSONObject
import java.io.File

private const val REPORT_URL = "https://t.me/+GN7d4fxJAVgxMjg1"

@Composable
fun JobDetailScreen(
    viewModel: JobDetailViewModel,
    onBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    val palette = LocalPalette.current
    val job by viewModel.job.collectAsState()
    val exportResult by viewModel.exportResult.collectAsState()
    val shareIntent by viewModel.shareIntent.collectAsState()
    val snackbarHostState = remember { SnackbarHostState() }
    val context = LocalContext.current
    var confirmDelete by remember { mutableStateOf(false) }

    LaunchedEffect(exportResult) {
        exportResult?.let {
            snackbarHostState.showSnackbar(it)
            viewModel.clearExportResult()
        }
    }
    LaunchedEffect(shareIntent) {
        shareIntent?.let {
            context.startActivity(it)
            viewModel.clearShareIntent()
        }
    }

    val jobData = job
    val state = jobData?.let { jobState(it) }

    Box(modifier.fillMaxSize()) {
        Column(
            Modifier
                .fillMaxSize()
                .statusBarsPadding()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = Space.m),
            verticalArrangement = Arrangement.spacedBy(Space.s)
        ) {
            ScreenHeader(
                title = jobData?.let { formatTimestamp(it.timestamp) } ?: "Dump",
                status = when {
                    jobData == null || state == null -> "Loading"
                    state == JobState.Succeeded -> "Finished in ${formatSeconds(jobData.durationMs ?: 0)}"
                    state == JobState.Failed -> "Failed" + (jobData.durationMs?.let { " after ${formatSeconds(it)}" } ?: "")
                    state == JobState.Interrupted -> "Interrupted: the app was closed during the dump"
                    else -> "Running"
                },
                statusColor = state?.let { jobStateColor(it, palette) } ?: palette.tertiary,
                live = state == JobState.Running,
                leading = { CircleButton(Icons.AutoMirrored.Filled.ArrowBack, "Back", onBack) },
            ) {
                if (jobData != null) CircleButton(Icons.Default.Delete, "Delete job", { confirmDelete = true })
            }

            if (jobData == null || state == null) return@Column

            when (state) {
                JobState.Succeeded -> OutputCard(jobData, viewModel, Modifier.entrance(0))
                JobState.Failed, JobState.Interrupted -> FailureCard(jobData, state, Modifier.entrance(0))
                JobState.Running -> {}
            }
            RunCard(jobData, Modifier.entrance(1))
            OptionsSummaryCard(jobData.configJson, Modifier.entrance(2))
            if (jobData.logs.isNotEmpty()) CollapsibleLog(jobData.logs, Modifier.entrance(3))
            Spacer(Modifier.height(Space.xl).navigationBarsPadding())
        }

        SnackbarHost(snackbarHostState, Modifier.align(Alignment.BottomCenter).navigationBarsPadding())
    }

    if (confirmDelete) {
        AlertDialog(
            onDismissRequest = { confirmDelete = false },
            title = { Text("Delete this dump?") },
            text = { Text("Its output files and log are removed too.") },
            confirmButton = {
                TextButton(onClick = {
                    confirmDelete = false
                    viewModel.deleteJob(onBack)
                }) { Text("Delete", color = palette.critical) }
            },
            dismissButton = { TextButton(onClick = { confirmDelete = false }) { Text("Cancel", color = palette.text) } }
        )
    }
}

@Composable
private fun OutputCard(job: DumpJobEntity, viewModel: JobDetailViewModel, modifier: Modifier) {
    val palette = LocalPalette.current
    val files = remember(job.outputDirPath, job.outputFileNames) {
        job.outputFileNames.map { name -> name to (job.outputDirPath?.let { File(it, name).length() } ?: 0L) }
    }
    Card(modifier, glow = palette.accent, padding = Space.m) {
        Heading("Output") {
            MonoText("${files.size} file${if (files.size == 1) "" else "s"} · ${formatSize(files.sumOf { it.second })}", color = palette.tertiary)
        }
        Column(Modifier.padding(top = Space.xs)) {
            files.forEachIndexed { i, (name, size) ->
                if (i > 0) Hairline()
                Row(Modifier.fillMaxWidth().padding(vertical = Space.xs), verticalAlignment = Alignment.CenterVertically) {
                    Box(
                        Modifier.size(32.dp).clip(RoundedCornerShape(8.dp)).background(palette.accent.copy(alpha = 0.12f)),
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(Icons.AutoMirrored.Filled.InsertDriveFile, null, tint = palette.accent, modifier = Modifier.size(16.dp))
                    }
                    Column(Modifier.weight(1f).padding(start = Space.s)) {
                        Text(name, style = Typo.bodyStrong, color = palette.text, maxLines = 1, overflow = TextOverflow.Ellipsis)
                        MonoText(formatSize(size))
                    }
                    Row(horizontalArrangement = Arrangement.spacedBy(Space.xs)) {
                        CircleButton(Icons.Default.Download, "Save $name to Downloads", { viewModel.exportFile(name) })
                        CircleButton(Icons.Default.Share, "Share $name", { viewModel.shareFile(name) })
                    }
                }
            }
        }
        if (files.isEmpty()) {
            Text("This dump produced no files.", style = Typo.body, color = palette.secondary)
        } else {
            Row(Modifier.padding(top = Space.s), horizontalArrangement = Arrangement.spacedBy(Space.xs)) {
                PrimaryPill("Save all as ZIP", Icons.Default.FolderZip, { viewModel.exportAllAsZip() })
                SecondaryPill("Share ZIP", Icons.Default.Share, { viewModel.shareAllAsZip() })
            }
        }
    }
}

@Composable
private fun FailureCard(job: DumpJobEntity, state: JobState, modifier: Modifier) {
    val palette = LocalPalette.current
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val detail = job.logs.lastOrNull { it.startsWith("ERROR") }?.removePrefix("ERROR:")?.trim()
    var copied by remember { mutableStateOf(false) }
    val debugInfo = {
        DebugInfoCollector.collect(
            metadataFileName = job.metadataFileName,
            il2cppFileName = job.il2cppFileName,
            metadataSize = job.metadataSize,
            il2cppSize = job.il2cppSize,
            config = parseConfigToDumpConfig(job.configJson),
            logs = job.logs,
            metadataVersion = job.metadataVersion,
            il2cppVersion = job.il2cppVersion,
            errorMessage = job.errorMessage
        )
    }
    Card(modifier, glow = if (state == JobState.Failed) palette.critical else palette.caution, padding = Space.m) {
        Text(
            if (state == JobState.Failed) job.errorMessage ?: "Failed" else "Interrupted",
            style = Typo.metric,
            color = if (state == JobState.Failed) palette.critical else palette.caution
        )
        if (detail != null && detail != job.errorMessage) {
            MonoText(detail, color = palette.text, maxLines = 4, modifier = Modifier.padding(top = Space.xxs))
        }
        Text(
            "Reports with debug info (files, options, log) get fixed fastest.",
            style = Typo.caption,
            color = palette.secondary,
            modifier = Modifier.padding(top = Space.xs)
        )
        Row(Modifier.padding(top = Space.s), horizontalArrangement = Arrangement.spacedBy(Space.xs)) {
            PrimaryPill(if (copied) "Copied" else "Copy debug info", Icons.Default.ContentCopy, {
                DebugInfoCollector.copyToClipboard(context, debugInfo())
                copied = true
                scope.launch { kotlinx.coroutines.delay(2000); copied = false }
            }, color = palette.text)
            SecondaryPill("Report", Icons.AutoMirrored.Filled.OpenInNew, {
                DebugInfoCollector.copyToClipboard(context, debugInfo())
                context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(REPORT_URL)))
            })
        }
    }
}

@Composable
private fun RunCard(job: DumpJobEntity, modifier: Modifier) {
    Card(modifier) {
        Heading("Run")
        Column(Modifier.padding(top = Space.xxs)) {
            job.metadataVersion?.let { ValueRow("Metadata", "v$it", mono = true) }
            job.il2cppVersion?.takeIf { !it.startsWith("Metadata") && it != "Unknown" }?.let { ValueRow("Il2Cpp", "v$it", mono = true) }
            job.durationMs?.let { ValueRow("Duration", formatSeconds(it), mono = true) }
        }
        Hairline(Modifier.padding(vertical = Space.xs))
        Heading("Inputs")
        Column(Modifier.padding(top = Space.xxs)) {
            InputFile(job.il2cppFileName, job.il2cppSize, job.il2cppHash)
            InputFile(job.metadataFileName, job.metadataSize, job.metadataHash)
        }
    }
}

@Composable
private fun InputFile(name: String, size: Long, hash: String?) {
    val palette = LocalPalette.current
    Column(Modifier.fillMaxWidth().padding(vertical = Space.xxs)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(name, style = Typo.bodyStrong, color = palette.text, modifier = Modifier.weight(1f), maxLines = 1, overflow = TextOverflow.Ellipsis)
            if (size > 0) MonoText(formatSize(size))
        }
        if (hash != null) MonoText("sha256 $hash", color = palette.tertiary, maxLines = 2)
    }
}

@Composable
private fun OptionsSummaryCard(configJson: String, modifier: Modifier) {
    val config = parseConfigToDumpConfig(configJson) ?: return
    val palette = LocalPalette.current
    val dumpCs = buildList {
        if (config.dumpMethod) add("methods")
        if (config.dumpField) add("fields")
        if (config.dumpProperty) add("properties")
        if (config.dumpAttribute) add("attributes")
        if (config.dumpFieldOffset) add("field offsets")
        if (config.dumpMethodOffset) add("method offsets")
        if (config.dumpTypeDefIndex) add("typedef index")
    }
    val advanced = buildList {
        if (config.forceDump) add("force dump mode")
        if (config.noRedirectedPointer) add("keep dump pointers")
        if (config.forceIl2CppVersion) add("forced version ${config.forceVersion}")
    }
    Card(modifier) {
        Heading("Options")
        Column(Modifier.padding(top = Space.xxs)) {
            ValueRow("dump.cs", dumpCs.joinToString(", ").ifEmpty { "off" }.replaceFirstChar { it.uppercase() })
            ValueRow("il2cpp.h", if (config.generateStruct) "Generated with script.json" else "Off")
            if (advanced.isNotEmpty()) {
                ValueRow("Advanced", advanced.joinToString(", ").replaceFirstChar { it.uppercase() }, valueColor = palette.caution)
            }
        }
    }
}

@Composable
private fun CollapsibleLog(logs: List<String>, modifier: Modifier) {
    val palette = LocalPalette.current
    var expanded by rememberSaveable { mutableStateOf(false) }
    Column(modifier) {
        Card(onClick = { expanded = !expanded }) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("Log", style = Typo.heading, color = palette.secondary)
                MonoText("${logs.size} lines", color = palette.tertiary, modifier = Modifier.padding(start = Space.xs))
                Spacer(Modifier.weight(1f))
                Icon(
                    if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                    contentDescription = if (expanded) "Hide log" else "Show log",
                    tint = palette.tertiary,
                    modifier = Modifier.size(18.dp)
                )
            }
        }
        AnimatedVisibility(expanded) {
            LogCard(logs, Modifier.padding(top = Space.xs))
        }
    }
}

private fun parseConfigToDumpConfig(json: String): DumpConfig? = try {
    val obj = JSONObject(json)
    DumpConfig(
        dumpMethod = obj.optBoolean("DumpMethod", true),
        dumpField = obj.optBoolean("DumpField", true),
        dumpProperty = obj.optBoolean("DumpProperty", false),
        dumpAttribute = obj.optBoolean("DumpAttribute", false),
        dumpFieldOffset = obj.optBoolean("DumpFieldOffset", true),
        dumpMethodOffset = obj.optBoolean("DumpMethodOffset", true),
        dumpTypeDefIndex = obj.optBoolean("DumpTypeDefIndex", true),
        generateStruct = obj.optBoolean("GenerateStruct", true),
        forceIl2CppVersion = obj.optBoolean("ForceIl2CppVersion", false),
        forceVersion = obj.optDouble("ForceVersion", 24.3),
        forceDump = obj.optBoolean("ForceDump", false),
        noRedirectedPointer = obj.optBoolean("NoRedirectedPointer", false)
    )
} catch (_: Exception) {
    null
}
