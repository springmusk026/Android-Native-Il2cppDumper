package com.il2cpp.dumper.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.il2cpp.dumper.ui.components.CircleButton
import com.il2cpp.dumper.ui.components.ScreenHeader
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Space
import com.mikepenz.aboutlibraries.ui.compose.LibrariesContainer

@Composable
fun LicensesScreen(
    onBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier.fillMaxSize().statusBarsPadding()) {
        Box(Modifier.padding(horizontal = Space.m)) {
            ScreenHeader(
                title = "Open-source licenses",
                status = "Libraries bundled in this app",
                statusColor = LocalPalette.current.tertiary,
                leading = { CircleButton(Icons.AutoMirrored.Filled.ArrowBack, "Back", onBack) },
            )
        }
        LibrariesContainer(modifier = Modifier.fillMaxSize())
    }
}
