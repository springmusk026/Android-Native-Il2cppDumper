package com.il2cpp.dumper.ui.theme

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

@Composable
fun Il2CppDumperTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    val palette = if (darkTheme) DarkPalette else LightPalette
    // Material components that remain (dialogs, text field, snackbar) read
    // their colors from the same palette.
    val base = if (darkTheme) darkColorScheme() else lightColorScheme()
    val colorScheme = base.copy(
        primary = palette.accent,
        onPrimary = palette.onAccent,
        background = palette.background,
        onBackground = palette.text,
        surface = palette.background,
        onSurface = palette.text,
        onSurfaceVariant = palette.secondary,
        surfaceContainerHigh = if (darkTheme) Color(0xFF1B1C20) else Color.White,
        outline = palette.hairline,
        outlineVariant = palette.hairline,
        error = palette.critical,
        inverseSurface = palette.text,
        inverseOnSurface = palette.background,
    )

    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            WindowCompat.getInsetsController(window, view).apply {
                isAppearanceLightStatusBars = !darkTheme
                isAppearanceLightNavigationBars = !darkTheme
            }
        }
    }

    CompositionLocalProvider(LocalPalette provides palette) {
        MaterialTheme(colorScheme = colorScheme, content = content)
    }
}
