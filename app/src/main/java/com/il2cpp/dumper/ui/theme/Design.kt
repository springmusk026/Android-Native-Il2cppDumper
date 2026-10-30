package com.il2cpp.dumper.ui.theme

import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

// Design system. Color carries meaning:
//   accent   - the dump itself: ready to run, results, the one primary action
//   live     - something is running right now
//   caution  - needs attention (dump file without an address, protected binary)
//   critical - failed
// Everything else is neutral text on translucent surfaces with hairline borders.

object Space {
    val xxs = 4.dp
    val xs = 8.dp
    val s = 12.dp
    val m = 16.dp
    val l = 20.dp
    val xl = 24.dp
    val xxl = 32.dp
}

object Radius {
    val control = 6.dp
    val card = 12.dp
}

@Immutable
data class Palette(
    val background: Color,
    val text: Color,
    val secondary: Color,
    val tertiary: Color,
    val surface: Color,
    val surfaceStrong: Color,
    val hairline: Color,
    val track: Color,
    val accent: Color,
    val onAccent: Color,
    val live: Color,
    val caution: Color,
    val critical: Color,
    val isDark: Boolean,
)

val LightPalette = Palette(
    background = Color(0xFFF6F6F7),
    text = Color(0xFF16171A),
    secondary = Color(0xFF666A73),
    tertiary = Color(0xFF9A9DA5),
    surface = Color(0x09000000),
    surfaceStrong = Color(0xFFFFFFFF),
    hairline = Color(0x14000000),
    track = Color(0x1C000000),
    accent = Color(0xFF4B55D1),
    onAccent = Color(0xFFFFFFFF),
    live = Color(0xFF1F9D55),
    caution = Color(0xFFB7720E),
    critical = Color(0xFFD13B3B),
    isDark = false,
)

val DarkPalette = Palette(
    background = Color(0xFF0E0F11),
    text = Color(0xFFECEDEF),
    secondary = Color(0xFF9A9DA5),
    tertiary = Color(0xFF6B6E76),
    surface = Color(0x0DFFFFFF),
    surfaceStrong = Color(0x21FFFFFF),
    hairline = Color(0x17FFFFFF),
    track = Color(0x21FFFFFF),
    accent = Color(0xFF8D96FF),
    onAccent = Color(0xFF0E0F11),
    live = Color(0xFF45D48B),
    caution = Color(0xFFF2B34A),
    critical = Color(0xFFFF6B66),
    isDark = true,
)

val LocalPalette = staticCompositionLocalOf { LightPalette }

private const val TNUM = "tnum"

object Typo {
    val hero = TextStyle(fontSize = 30.sp, lineHeight = 36.sp, fontWeight = FontWeight.SemiBold, fontFeatureSettings = TNUM)
    val title = TextStyle(fontSize = 20.sp, lineHeight = 26.sp, fontWeight = FontWeight.SemiBold)
    val metric = TextStyle(fontSize = 15.sp, lineHeight = 20.sp, fontWeight = FontWeight.SemiBold, fontFeatureSettings = TNUM)
    val body = TextStyle(fontSize = 14.sp, lineHeight = 20.sp)
    val bodyStrong = TextStyle(fontSize = 14.sp, lineHeight = 20.sp, fontWeight = FontWeight.Medium)
    val heading = TextStyle(fontSize = 12.sp, lineHeight = 16.sp, fontWeight = FontWeight.Medium)
    val caption = TextStyle(fontSize = 12.sp, lineHeight = 16.sp)
    val mono = TextStyle(fontSize = 11.5.sp, lineHeight = 16.sp, fontFamily = FontFamily.Monospace)
}

object Motion {
    fun <T> spring() = spring<T>(dampingRatio = 0.86f, stiffness = Spring.StiffnessMediumLow)
    fun <T> quick() = tween<T>(220)
    fun <T> count() = tween<T>(800)
}
