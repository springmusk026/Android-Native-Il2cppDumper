package com.il2cpp.dumper.ui.screens

import android.content.Intent
import android.net.Uri
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.automirrored.filled.OpenInNew
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.BuildConfig
import com.il2cpp.dumper.ui.components.Card
import com.il2cpp.dumper.ui.components.CircleButton
import com.il2cpp.dumper.ui.components.Hairline
import com.il2cpp.dumper.ui.components.Heading
import com.il2cpp.dumper.ui.components.MonoText
import com.il2cpp.dumper.ui.components.ScreenHeader
import com.il2cpp.dumper.ui.components.entrance
import com.il2cpp.dumper.ui.components.pressable
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.ui.theme.Typo
import com.il2cpp.dumper.util.DebugInfoCollector

private const val REPO_URL = "https://github.com/springmusk026/Android-Native-Il2cppDumper"
private const val UPSTREAM_URL = "https://github.com/Perfare/Il2CppDumper"
private const val REPORT_GROUP_URL = "https://t.me/+GN7d4fxJAVgxMjg1"

@Composable
fun AboutScreen(
    onBack: () -> Unit,
    onNavigateToLicenses: () -> Unit,
    modifier: Modifier = Modifier
) {
    val palette = LocalPalette.current
    val context = LocalContext.current
    val open = { url: String -> context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url))) }
    var copied by remember { mutableStateOf(false) }

    Column(
        modifier
            .fillMaxSize()
            .statusBarsPadding()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = Space.m),
        verticalArrangement = Arrangement.spacedBy(Space.s)
    ) {
        ScreenHeader(
            title = "About",
            status = "Version ${BuildConfig.VERSION_NAME} · build ${BuildConfig.VERSION_CODE}",
            statusColor = palette.tertiary,
            leading = { CircleButton(Icons.AutoMirrored.Filled.ArrowBack, "Back", onBack) },
        )

        Card(Modifier.entrance(0), padding = Space.m) {
            Text("Il2Cpp Dumper", style = Typo.metric, color = palette.text)
            Text(
                "Dumps Unity IL2CPP games on the phone: dump.cs, script.json and il2cpp.h from libil2cpp.so and global-metadata.dat.",
                style = Typo.body,
                color = palette.secondary,
                modifier = Modifier.padding(top = Space.xxs)
            )
            Text(
                "A native port of Perfare's Il2CppDumper.",
                style = Typo.caption,
                color = palette.tertiary,
                modifier = Modifier.padding(top = Space.xs)
            )
        }

        Card(Modifier.entrance(1), padding = Space.xxs) {
            LinkRow("Source code", "github.com/springmusk026", Icons.AutoMirrored.Filled.OpenInNew) { open(REPO_URL) }
            Hairline(Modifier.padding(horizontal = Space.s))
            LinkRow("Upstream Il2CppDumper", "github.com/Perfare", Icons.AutoMirrored.Filled.OpenInNew) { open(UPSTREAM_URL) }
            Hairline(Modifier.padding(horizontal = Space.s))
            LinkRow("Telegram channel", "@Layout_musk", Icons.AutoMirrored.Filled.OpenInNew) { open("https://t.me/Layout_musk") }
            Hairline(Modifier.padding(horizontal = Space.s))
            LinkRow("Developer", "@springmusk", Icons.AutoMirrored.Filled.OpenInNew) { open("https://t.me/springmusk") }
        }

        Card(Modifier.entrance(2), padding = Space.xxs) {
            LinkRow(
                "Report a bug",
                if (copied) "Device info copied, paste it in the group" else "Copies device info, opens the Telegram group",
                Icons.AutoMirrored.Filled.OpenInNew,
                mono = false
            ) {
                DebugInfoCollector.copyToClipboard(context, DebugInfoCollector.collect())
                copied = true
                open(REPORT_GROUP_URL)
            }
            Hairline(Modifier.padding(horizontal = Space.s))
            LinkRow("Open-source licenses", "Third-party libraries", Icons.AutoMirrored.Filled.KeyboardArrowRight, mono = false, onClick = onNavigateToLicenses)
        }

        Card(Modifier.entrance(3)) {
            Heading("License")
            Text(
                "Springmusk Non-Commercial License",
                style = Typo.bodyStrong,
                color = palette.text,
                modifier = Modifier.padding(top = Space.xxs)
            )
            Text(
                "Free to use, modify and share, free of charge, with attribution to springmusk. No commercial use, ads or subscriptions.",
                style = Typo.caption,
                color = palette.secondary,
                modifier = Modifier.padding(top = Space.xxs)
            )
            Text(
                "Read the full license",
                style = Typo.caption,
                color = palette.accent,
                modifier = Modifier.padding(top = Space.xs).pressable({ open("$REPO_URL/blob/main/LICENSE") })
            )
        }

        Text(
            "For education and research. You are responsible for complying with the laws and terms that apply to what you dump.",
            style = Typo.caption,
            color = palette.tertiary,
            modifier = Modifier.padding(horizontal = Space.xxs)
        )
        Spacer(Modifier.height(Space.xl).navigationBarsPadding())
    }
}

@Composable
private fun LinkRow(title: String, subtitle: String, icon: ImageVector, mono: Boolean = true, onClick: () -> Unit) {
    val palette = LocalPalette.current
    Row(
        Modifier
            .fillMaxWidth()
            .pressable(onClick)
            .padding(Space.s),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(Modifier.weight(1f)) {
            Text(title, style = Typo.bodyStrong, color = palette.text)
            if (mono) MonoText(subtitle) else Text(subtitle, style = Typo.caption, color = palette.secondary)
        }
        Icon(icon, contentDescription = null, tint = palette.tertiary, modifier = Modifier.size(16.dp))
    }
}
