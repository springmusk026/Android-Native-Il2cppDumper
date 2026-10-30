package com.il2cpp.dumper.ui.screens

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowForward
import androidx.compose.material.icons.automirrored.filled.InsertDriveFile
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.ui.components.Card
import com.il2cpp.dumper.ui.components.CircleButton
import com.il2cpp.dumper.ui.components.Hairline
import com.il2cpp.dumper.ui.components.Heading
import com.il2cpp.dumper.ui.components.MonoText
import com.il2cpp.dumper.ui.components.PrimaryPill
import com.il2cpp.dumper.ui.components.ScreenHeader
import com.il2cpp.dumper.ui.components.SecondaryPill
import com.il2cpp.dumper.ui.components.StageTrack
import com.il2cpp.dumper.ui.components.TogglePill
import com.il2cpp.dumper.ui.components.entrance
import com.il2cpp.dumper.ui.components.pressable
import com.il2cpp.dumper.ui.components.reduceMotion
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Motion
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.ui.theme.Typo
import com.il2cpp.dumper.viewmodel.DumpConfig
import com.il2cpp.dumper.viewmodel.DumpState
import com.il2cpp.dumper.viewmodel.DumperViewModel
import kotlinx.coroutines.delay

private val Stages = listOf("Load", "Init", "Search", "Dump")

@Composable
fun HomeScreen(
    viewModel: DumperViewModel,
    onNavigateToJob: (Long) -> Unit,
    onOpenAbout: () -> Unit,
    modifier: Modifier = Modifier
) {
    val palette = LocalPalette.current
    val state by viewModel.state.collectAsState()
    val logs by viewModel.logs.collectAsState()
    val config by viewModel.config.collectAsState()
    val metadataFile by viewModel.metadataFile.collectAsState()
    val il2cppFile by viewModel.il2cppFile.collectAsState()
    val isDumpFile by viewModel.isDumpFile.collectAsState()
    val startedAt by viewModel.startedAt.collectAsState()

    val metadataLauncher = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri?.let { viewModel.setMetadataUri(it) }
    }
    val il2cppLauncher = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri?.let { viewModel.setIl2cppUri(it) }
    }

    val running = state is DumpState.Initializing || state is DumpState.Searching || state is DumpState.Dumping
    val filesReady = (if (il2cppFile != null) 1 else 0) + (if (metadataFile != null) 1 else 0)

    // Stage index while running; remembered so a failure can show where it stopped.
    var lastStage by remember { mutableIntStateOf(-1) }
    val stage = when (state) {
        is DumpState.Initializing -> if (logs.any { it.startsWith("Initializing native") }) 1 else 0
        is DumpState.Searching -> 2
        is DumpState.Dumping -> 3
        is DumpState.Success -> Stages.size
        is DumpState.Error -> lastStage.coerceAtLeast(0)
        DumpState.Idle -> -1
    }
    LaunchedEffect(stage, running) { if (running) lastStage = stage }

    val elapsed by produceState(0L, running, startedAt) {
        while (running) {
            value = System.currentTimeMillis() - startedAt
            delay(100)
        }
    }

    val (status, statusColor) = when (val s = state) {
        DumpState.Idle -> if (filesReady == 2) "Ready to dump" to palette.accent
                          else "Waiting for ${2 - filesReady} file${if (filesReady == 1) "" else "s"}" to palette.tertiary
        is DumpState.Initializing, is DumpState.Searching, is DumpState.Dumping -> stageVerb(stage) to palette.live
        is DumpState.Success -> "Finished in ${formatSeconds(s.durationMs)}" to palette.accent
        is DumpState.Error -> "Failed" to palette.critical
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .statusBarsPadding()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = Space.m),
        verticalArrangement = Arrangement.spacedBy(Space.s)
    ) {
        ScreenHeader(
            title = "Il2Cpp Dumper",
            status = status,
            statusColor = statusColor,
            live = running,
        ) {
            CircleButton(Icons.Default.Info, "About", onOpenAbout)
        }

        HeroCard(
            state = state,
            stage = stage,
            elapsedMs = elapsed,
            filesReady = filesReady,
            logs = logs,
            onStart = { viewModel.startDump() },
            onOpenResults = { (state as? DumpState.Success)?.let { onNavigateToJob(it.jobId) } },
            onReset = { viewModel.reset() },
            modifier = Modifier.entrance(0)
        )

        Card(Modifier.entrance(1), padding = Space.xxs) {
            FileRow(
                title = "Il2Cpp binary",
                hint = "libil2cpp.so",
                file = il2cppFile,
                enabled = !running,
                onClick = { il2cppLauncher.launch(arrayOf("*/*")) }
            )
            Hairline(Modifier.padding(horizontal = Space.s))
            FileRow(
                title = "Metadata",
                hint = "global-metadata.dat",
                file = metadataFile,
                enabled = !running,
                onClick = { metadataLauncher.launch(arrayOf("*/*")) }
            )
            Hairline(Modifier.padding(horizontal = Space.s))
            AddressRow(
                isDumpFile = isDumpFile || config.forceDump,
                enabled = !running,
                onAddress = viewModel::setDumpAddress
            )
        }

        OptionsCard(
            config = config,
            enabled = !running,
            onConfigChange = viewModel::updateConfig,
            modifier = Modifier.entrance(2)
        )

        AnimatedVisibility(logs.isNotEmpty(), enter = fadeIn(Motion.quick()), exit = fadeOut(Motion.quick())) {
            LogCard(logs)
        }

        // Room for the floating tab switcher.
        Spacer(Modifier.height(88.dp))
    }
}

@Composable
private fun HeroCard(
    state: DumpState,
    stage: Int,
    elapsedMs: Long,
    filesReady: Int,
    logs: List<String>,
    onStart: () -> Unit,
    onOpenResults: () -> Unit,
    onReset: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val palette = LocalPalette.current
    val running = state is DumpState.Initializing || state is DumpState.Searching || state is DumpState.Dumping
    val glow = when (state) {
        is DumpState.Error -> palette.critical
        DumpState.Idle -> if (filesReady == 2) palette.accent else null
        else -> if (running) palette.live else palette.accent
    }

    Card(modifier, glow = glow, padding = Space.m) {
        AnimatedContent(
            targetState = heroKey(state, filesReady),
            transitionSpec = { fadeIn(Motion.quick()) togetherWith fadeOut(Motion.quick()) },
            label = "hero"
        ) { key ->
            Column {
                when (key) {
                    "files" -> {
                        HeroNumber("$filesReady", unit = "/ 2")
                        Caption("files picked: the game's libil2cpp.so and global-metadata.dat")
                    }
                    "ready" -> {
                        HeroNumber("Ready")
                        Caption("Both files picked. The options below apply to this run.")
                    }
                    "running" -> {
                        HeroNumber(formatElapsed(elapsedMs))
                        Caption(stageVerb(stage))
                    }
                    "success" -> {
                        val s = state as? DumpState.Success
                        HeroNumber(s?.let { formatSeconds(it.durationMs) } ?: "", countUp = true)
                        Caption(summarize(logs) ?: "Dump finished")
                    }
                    else -> {
                        HeroNumber("Failed", color = palette.critical)
                        Caption(failureReason(state, logs), maxLines = 3)
                    }
                }
            }
        }

        Spacer(Modifier.height(Space.m))
        StageTrack(
            stages = Stages,
            current = stage,
            color = if (running) palette.live else palette.accent,
            failed = state is DumpState.Error,
        )
        Spacer(Modifier.height(Space.m))

        Row(horizontalArrangement = Arrangement.spacedBy(Space.xs), verticalAlignment = Alignment.CenterVertically) {
            when (state) {
                is DumpState.Success -> {
                    PrimaryPill("Open results", Icons.AutoMirrored.Filled.ArrowForward, onOpenResults)
                    SecondaryPill("New dump", null, onReset)
                }
                is DumpState.Error -> PrimaryPill("Try again", Icons.Default.Refresh, onReset, color = palette.text)
                else -> PrimaryPill(
                    text = if (running) "Running…" else "Start dump",
                    icon = Icons.Default.PlayArrow,
                    onClick = onStart,
                    enabled = !running && filesReady == 2,
                )
            }
        }
    }
}

private fun heroKey(state: DumpState, filesReady: Int) = when (state) {
    DumpState.Idle -> if (filesReady == 2) "ready" else "files"
    is DumpState.Success -> "success"
    is DumpState.Error -> "error"
    else -> "running"
}

@Composable
private fun HeroNumber(text: String, unit: String? = null, color: Color = LocalPalette.current.text, countUp: Boolean = false) {
    val palette = LocalPalette.current
    val shown = if (countUp) countUp(text) else text
    Row(verticalAlignment = Alignment.Bottom) {
        Text(shown, style = Typo.hero, color = color, maxLines = 1)
        if (unit != null) {
            Text(unit, style = Typo.metric, color = palette.secondary, modifier = Modifier.padding(start = Space.xxs, bottom = 5.dp))
        }
    }
}

/** Animates the number in "4.2 s" up from 0. */
@Composable
private fun countUp(text: String): String {
    val target = text.substringBefore(' ').toFloatOrNull() ?: return text
    val suffix = text.substringAfter(' ', "")
    val reduce = reduceMotion()
    val value = remember { Animatable(if (reduce) target else 0f) }
    LaunchedEffect(target) { value.animateTo(target, Motion.count()) }
    return "%.1f %s".format(value.value, suffix).trim()
}

@Composable
private fun Caption(text: String, maxLines: Int = 2) {
    Text(
        text, style = Typo.body, color = LocalPalette.current.secondary, maxLines = maxLines,
        overflow = TextOverflow.Ellipsis, modifier = Modifier.padding(top = Space.xxs)
    )
}

@Composable
private fun FileRow(
    title: String,
    hint: String,
    file: DumperViewModel.PickedFile?,
    enabled: Boolean,
    onClick: () -> Unit,
) {
    val palette = LocalPalette.current
    Row(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(8.dp))
            .pressable(onClick, enabled)
            .padding(Space.s),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            Modifier
                .size(32.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(if (file != null) palette.accent.copy(alpha = 0.12f) else palette.surface),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                if (file != null) Icons.Default.Check else Icons.AutoMirrored.Filled.InsertDriveFile,
                contentDescription = null,
                tint = if (file != null) palette.accent else palette.tertiary,
                modifier = Modifier.size(16.dp)
            )
        }
        Column(Modifier.weight(1f).padding(start = Space.s)) {
            Text(title, style = Typo.bodyStrong, color = palette.text)
            if (file != null) {
                MonoText(if (file.size >= 0) "${file.name} · ${formatSize(file.size)}" else file.name)
            } else {
                MonoText(hint, color = palette.tertiary)
            }
        }
        Text(
            if (file != null) "Change" else "Choose",
            style = Typo.caption,
            color = if (enabled) palette.accent else palette.tertiary,
            modifier = Modifier.padding(start = Space.xs)
        )
    }
}

@Composable
private fun AddressRow(isDumpFile: Boolean, enabled: Boolean, onAddress: (Long) -> Unit) {
    val palette = LocalPalette.current
    var text by rememberSaveable { mutableStateOf("") }
    val parsed = text.trim().removePrefix("0x").removePrefix("0X").toULongOrNull(16)
    val valid = text.isBlank() || parsed != null
    val needsAddress = isDumpFile && text.isBlank()
    Row(
        Modifier.fillMaxWidth().padding(Space.s),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(Modifier.weight(1f)) {
            Text("Image base", style = Typo.bodyStrong, color = palette.text)
            Text(
                when {
                    !valid -> "Not a hex address"
                    needsAddress -> "Memory dump: enter its load address"
                    else -> "Only for memory-dumped binaries"
                },
                style = Typo.caption,
                color = when {
                    !valid -> palette.critical
                    needsAddress -> palette.caution
                    else -> palette.tertiary
                }
            )
        }
        BasicTextField(
            value = text,
            onValueChange = {
                text = it
                onAddress(it.trim().removePrefix("0x").removePrefix("0X").toULongOrNull(16)?.toLong() ?: 0L)
            },
            enabled = enabled,
            singleLine = true,
            textStyle = Typo.mono.copy(color = palette.text),
            cursorBrush = SolidColor(palette.accent),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Ascii),
            modifier = Modifier.width(132.dp),
            decorationBox = { inner ->
                Box(
                    Modifier
                        .clip(RoundedCornerShape(6.dp))
                        .background(palette.surface)
                        .padding(Space.xs)
                ) {
                    if (text.isEmpty()) MonoText("0x7a00000000", color = palette.tertiary)
                    inner()
                }
            }
        )
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun OptionsCard(
    config: DumpConfig,
    enabled: Boolean,
    onConfigChange: (DumpConfig) -> Unit,
    modifier: Modifier = Modifier,
) {
    val palette = LocalPalette.current
    var advanced by rememberSaveable { mutableStateOf(false) }
    val set: (DumpConfig) -> Unit = { if (enabled) onConfigChange(it) }
    val advancedOn = listOf(config.forceDump, config.noRedirectedPointer, config.forceIl2CppVersion).count { it }

    Card(modifier) {
        Heading("dump.cs")
        FlowRow(
            Modifier.padding(top = Space.xs),
            horizontalArrangement = Arrangement.spacedBy(Space.xs),
            verticalArrangement = Arrangement.spacedBy(Space.xs)
        ) {
            TogglePill("Methods", config.dumpMethod) { set(config.copy(dumpMethod = it)) }
            TogglePill("Fields", config.dumpField) { set(config.copy(dumpField = it)) }
            TogglePill("Properties", config.dumpProperty) { set(config.copy(dumpProperty = it)) }
            TogglePill("Attributes", config.dumpAttribute) { set(config.copy(dumpAttribute = it)) }
            TogglePill("Field offsets", config.dumpFieldOffset) { set(config.copy(dumpFieldOffset = it)) }
            TogglePill("Method offsets", config.dumpMethodOffset) { set(config.copy(dumpMethodOffset = it)) }
            TogglePill("TypeDef index", config.dumpTypeDefIndex) { set(config.copy(dumpTypeDefIndex = it)) }
        }
        Heading("Also generate", Modifier.padding(top = Space.s))
        Row(Modifier.padding(top = Space.xs)) {
            TogglePill("script.json + il2cpp.h", config.generateStruct) { set(config.copy(generateStruct = it)) }
        }

        Hairline(Modifier.padding(top = Space.s))
        Row(
            Modifier
                .fillMaxWidth()
                .pressable({ advanced = !advanced })
                .padding(top = Space.s),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("Advanced", style = Typo.heading, color = palette.secondary)
            if (advancedOn > 0) {
                Text("$advancedOn on", style = Typo.caption, color = palette.caution, modifier = Modifier.padding(start = Space.xs))
            }
            Spacer(Modifier.weight(1f))
            Icon(
                if (advanced) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                contentDescription = if (advanced) "Hide advanced options" else "Show advanced options",
                tint = palette.tertiary,
                modifier = Modifier.size(18.dp)
            )
        }
        AnimatedVisibility(advanced) {
            Column(Modifier.padding(top = Space.xs)) {
                FlowRow(horizontalArrangement = Arrangement.spacedBy(Space.xs), verticalArrangement = Arrangement.spacedBy(Space.xs)) {
                    TogglePill("Force dump mode", config.forceDump) { set(config.copy(forceDump = it)) }
                    TogglePill("Keep dump pointers", config.noRedirectedPointer) { set(config.copy(noRedirectedPointer = it)) }
                    TogglePill("Force version", config.forceIl2CppVersion) { set(config.copy(forceIl2CppVersion = it)) }
                }
                AnimatedVisibility(config.forceIl2CppVersion) {
                    VersionField(config.forceVersion) { set(config.copy(forceVersion = it)) }
                }
            }
        }
    }
}

@Composable
private fun VersionField(version: Double, onChange: (Double) -> Unit) {
    val palette = LocalPalette.current
    var text by remember { mutableStateOf(version.toString()) }
    Row(Modifier.padding(top = Space.s), verticalAlignment = Alignment.CenterVertically) {
        Text("Il2Cpp version", style = Typo.caption, color = palette.secondary, modifier = Modifier.weight(1f))
        BasicTextField(
            value = text,
            onValueChange = { v ->
                text = v
                v.toDoubleOrNull()?.let(onChange)
            },
            singleLine = true,
            textStyle = Typo.mono.copy(color = palette.text),
            cursorBrush = SolidColor(palette.accent),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal),
            modifier = Modifier.width(72.dp),
            decorationBox = { inner ->
                Box(Modifier.clip(RoundedCornerShape(6.dp)).background(palette.surface).padding(Space.xs)) { inner() }
            }
        )
    }
}

@Composable
fun LogCard(logs: List<String>, modifier: Modifier = Modifier) {
    val palette = LocalPalette.current
    val listState = rememberLazyListState()
    LaunchedEffect(logs.size) {
        if (logs.isNotEmpty()) listState.animateScrollToItem(logs.size - 1)
    }
    Card(modifier) {
        Heading("Log") {
            MonoText("${logs.size} lines", color = palette.tertiary)
        }
        LazyColumn(
            state = listState,
            modifier = Modifier.fillMaxWidth().height(180.dp).padding(top = Space.xs)
        ) {
            itemsIndexed(logs) { i, line ->
                val color = when {
                    line.startsWith("ERROR") || line.startsWith("Error") -> palette.critical
                    line.startsWith("WARNING") -> palette.caution
                    i == logs.lastIndex -> palette.text
                    else -> palette.secondary
                }
                Text(line, style = Typo.mono, color = color)
            }
        }
    }
}

private fun stageVerb(stage: Int) = when (stage) {
    0 -> "Copying input files"
    1 -> "Parsing metadata and ELF"
    2 -> "Searching for registrations"
    3 -> "Writing output files"
    else -> "Working"
}

/** "12,345 types · 98,765 methods · Il2Cpp 29.1", parsed from the native log. */
private fun summarize(logs: List<String>): String? {
    val counts = logs.firstNotNullOfOrNull { Regex("""Types: (\d+) \| Methods: (\d+)""").find(it) }
    val version = logs.lastOrNull { it.contains("Types loaded") }
        ?.let { Regex("""Il2Cpp version: ([\d.]+)""").find(it)?.groupValues?.get(1) }
    val parts = buildList {
        counts?.let {
            add("%,d types".format(it.groupValues[1].toLong()))
            add("%,d methods".format(it.groupValues[2].toLong()))
        }
        version?.let { add("Il2Cpp $it") }
    }
    return parts.takeIf { it.isNotEmpty() }?.joinToString(" · ")
}

private fun failureReason(state: DumpState, logs: List<String>): String {
    val message = (state as? DumpState.Error)?.message ?: "Unknown error"
    val detail = logs.lastOrNull { it.startsWith("ERROR") }?.removePrefix("ERROR:")?.trim()
    return if (detail != null && !message.contains(detail)) "$message. $detail" else message
}

private fun formatElapsed(ms: Long): String {
    val tenths = ms / 100
    return "%d:%02d.%d".format(tenths / 600, (tenths / 10) % 60, tenths % 10)
}

fun formatSeconds(ms: Long): String = "%.1f s".format(ms / 1000.0)

fun formatSize(bytes: Long): String = when {
    bytes >= 1_073_741_824 -> "%.2f GB".format(bytes / 1_073_741_824.0)
    bytes >= 1_048_576 -> "%.1f MB".format(bytes / 1_048_576.0)
    bytes >= 1024 -> "%.1f KB".format(bytes / 1024.0)
    else -> "$bytes B"
}
