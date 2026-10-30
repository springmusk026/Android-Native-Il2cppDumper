package com.il2cpp.dumper.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.DeleteSweep
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.JobStatus
import com.il2cpp.dumper.ui.components.Card
import com.il2cpp.dumper.ui.components.CircleButton
import com.il2cpp.dumper.ui.components.MonoText
import com.il2cpp.dumper.ui.components.ScreenHeader
import com.il2cpp.dumper.ui.components.StatusDot
import com.il2cpp.dumper.ui.components.entrance
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Palette
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.ui.theme.Typo
import com.il2cpp.dumper.viewmodel.JobHistoryViewModel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun JobHistoryScreen(
    viewModel: JobHistoryViewModel,
    onJobClick: (Long) -> Unit,
    modifier: Modifier = Modifier
) {
    val palette = LocalPalette.current
    val jobs by viewModel.jobs.collectAsState()
    var showClearDialog by remember { mutableStateOf(false) }

    val failed = jobs.count { it.status == JobStatus.FAILED }
    val running = jobs.any { jobState(it) == JobState.Running }
    val status = when {
        jobs.isEmpty() -> "No dumps yet"
        running -> "A dump is running"
        else -> buildString {
            append("${jobs.size} dump${if (jobs.size == 1) "" else "s"}")
            if (failed > 0) append(" · $failed failed")
        }
    }

    Column(modifier.fillMaxSize().statusBarsPadding()) {
        Box(Modifier.padding(horizontal = Space.m)) {
            ScreenHeader(
                title = "History",
                status = status,
                statusColor = if (running) palette.live else palette.tertiary,
                live = running,
            ) {
                if (jobs.isNotEmpty()) {
                    CircleButton(Icons.Default.DeleteSweep, "Clear all history", { showClearDialog = true })
                }
            }
        }

        if (jobs.isEmpty()) {
            Box(Modifier.fillMaxSize().padding(Space.xxl), contentAlignment = Alignment.Center) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("Nothing here yet", style = Typo.metric, color = palette.text)
                    Text(
                        "Each dump you run is kept here with its output files and log.",
                        style = Typo.body,
                        color = palette.secondary,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(top = Space.xxs)
                    )
                }
            }
        } else {
            LazyColumn(
                modifier = Modifier.fillMaxSize(),
                verticalArrangement = Arrangement.spacedBy(Space.xs),
                contentPadding = PaddingValues(start = Space.m, end = Space.m, top = Space.s, bottom = 96.dp)
            ) {
                itemsIndexed(jobs, key = { _, job -> job.id }) { i, job ->
                    JobRow(job, onClick = { onJobClick(job.id) }, modifier = Modifier.entrance(i.coerceAtMost(8)))
                }
            }
        }
    }

    if (showClearDialog) {
        AlertDialog(
            onDismissRequest = { showClearDialog = false },
            title = { Text("Clear history?") },
            text = { Text("Deletes all ${jobs.size} jobs and their output files. This can't be undone.") },
            confirmButton = {
                TextButton(onClick = {
                    viewModel.clearAll()
                    showClearDialog = false
                }) { Text("Clear all", color = palette.critical) }
            },
            dismissButton = {
                TextButton(onClick = { showClearDialog = false }) { Text("Cancel", color = palette.text) }
            }
        )
    }
}

enum class JobState { Running, Interrupted, Succeeded, Failed }

/** A RUNNING row that is old and never finished belongs to a killed process. */
fun jobState(job: DumpJobEntity): JobState = when (job.status) {
    JobStatus.SUCCESS -> JobState.Succeeded
    JobStatus.FAILED -> JobState.Failed
    JobStatus.RUNNING ->
        if (System.currentTimeMillis() - job.timestamp > 30 * 60_000L) JobState.Interrupted else JobState.Running
}

fun jobStateColor(state: JobState, palette: Palette): Color = when (state) {
    JobState.Running -> palette.live
    JobState.Interrupted -> palette.caution
    JobState.Succeeded -> palette.accent
    JobState.Failed -> palette.critical
}

@Composable
private fun JobRow(job: DumpJobEntity, onClick: () -> Unit, modifier: Modifier = Modifier) {
    val palette = LocalPalette.current
    val state = jobState(job)
    Card(modifier, onClick = onClick) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.size(16.dp), contentAlignment = Alignment.Center) {
                StatusDot(jobStateColor(state, palette), live = state == JobState.Running)
            }
            Column(Modifier.weight(1f).padding(start = Space.s)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(formatTimestamp(job.timestamp), style = Typo.bodyStrong, color = palette.text)
                    Text(
                        stateLabel(state),
                        style = Typo.caption,
                        color = jobStateColor(state, palette),
                        modifier = Modifier.padding(start = Space.xs)
                    )
                }
                when (state) {
                    JobState.Failed -> Text(
                        job.errorMessage ?: "Failed",
                        style = Typo.caption,
                        color = palette.secondary,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis
                    )
                    else -> MonoText(jobFacts(job))
                }
            }
            Icon(
                Icons.AutoMirrored.Filled.KeyboardArrowRight,
                contentDescription = null,
                tint = palette.tertiary,
                modifier = Modifier.size(18.dp)
            )
        }
    }
}

fun stateLabel(state: JobState) = when (state) {
    JobState.Running -> "Running"
    JobState.Interrupted -> "Interrupted"
    JobState.Succeeded -> "Done"
    JobState.Failed -> "Failed"
}

/** "Il2Cpp 29.1 · 3 files · 4.2 s" */
private fun jobFacts(job: DumpJobEntity): String = buildList {
    job.il2cppVersion?.takeIf { it.isNotBlank() && it != "Unknown" }?.let {
        add(if (it.startsWith("Metadata")) it else "Il2Cpp $it")
    } ?: job.metadataVersion?.let { add("Metadata $it") }
    if (job.outputFileNames.isNotEmpty()) add("${job.outputFileNames.size} file${if (job.outputFileNames.size == 1) "" else "s"}")
    job.durationMs?.let { add(formatSeconds(it)) }
    if (isEmpty()) add(job.il2cppFileName)
}.joinToString(" · ")

fun formatTimestamp(timestamp: Long): String =
    SimpleDateFormat("MMM d, HH:mm", Locale.getDefault()).format(Date(timestamp))
