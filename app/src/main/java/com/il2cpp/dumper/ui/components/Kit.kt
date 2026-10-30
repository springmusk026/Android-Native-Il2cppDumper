package com.il2cpp.dumper.ui.components

import android.provider.Settings
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Check
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.layout.Layout
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Motion
import com.il2cpp.dumper.ui.theme.Radius
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.ui.theme.Typo

/** True when the user turned animations off (developer option or accessibility). */
@Composable
fun reduceMotion(): Boolean {
    val context = LocalContext.current
    return remember {
        Settings.Global.getFloat(context.contentResolver, Settings.Global.ANIMATOR_DURATION_SCALE, 1f) == 0f
    }
}

/** Staggered entrance: fade in, 6dp lift, 0.985 scale. Runs once on first composition. */
@Composable
fun Modifier.entrance(index: Int): Modifier {
    val reduce = reduceMotion()
    val progress = remember { Animatable(if (reduce) 1f else 0f) }
    LaunchedEffect(Unit) {
        if (!reduce) {
            kotlinx.coroutines.delay(40L * index)
            progress.animateTo(1f, Motion.spring())
        }
    }
    val lift = with(LocalDensity.current) { 6.dp.toPx() }
    return graphicsLayer {
        alpha = progress.value
        translationY = (1f - progress.value) * lift
        val s = 0.985f + 0.015f * progress.value
        scaleX = s
        scaleY = s
    }
}

/** Neutral surface, hairline border, optional soft glow in the top-end corner. */
@Composable
fun Card(
    modifier: Modifier = Modifier,
    glow: Color? = null,
    padding: Dp = Space.s,
    onClick: (() -> Unit)? = null,
    content: @Composable ColumnScope.() -> Unit
) {
    val palette = LocalPalette.current
    val shape = RoundedCornerShape(Radius.card)
    val glowAlpha by animateFloatAsState(if (glow != null) 1f else 0f, Motion.quick(), label = "glow")
    val glowColor = glow ?: palette.accent
    Column(
        modifier = modifier
            .fillMaxWidth()
            .clip(shape)
            .background(palette.card)
            .drawBehind {
                if (glowAlpha > 0f) {
                    val radius = size.maxDimension * 0.75f
                    drawRect(
                        Brush.radialGradient(
                            0f to glowColor.copy(alpha = (if (palette.isDark) 0.20f else 0.13f) * glowAlpha),
                            1f to Color.Transparent,
                            center = Offset(size.width, 0f),
                            radius = radius
                        )
                    )
                }
                if (palette.isDark) {
                    // faint top highlight
                    drawLine(Color.White.copy(alpha = 0.06f), Offset(0f, 0.5f), Offset(size.width, 0.5f), 1f)
                }
            }
            .border(BorderStroke(1.dp, palette.hairline), shape)
            .then(if (onClick != null) Modifier.pressable(onClick) else Modifier)
            .padding(padding),
        content = content
    )
}

/** Click handling with a slight press scale instead of a ripple. */
@Composable
fun Modifier.pressable(onClick: () -> Unit, enabled: Boolean = true, role: Role = Role.Button): Modifier {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    val scale by animateFloatAsState(if (pressed) 0.97f else 1f, Motion.quick(), label = "press")
    return this
        .scale(scale)
        .clickable(interactionSource = interaction, indication = null, enabled = enabled, role = role, onClick = onClick)
}

@Composable
fun Heading(text: String, modifier: Modifier = Modifier, trailing: @Composable RowScope.() -> Unit = {}) {
    Row(modifier = modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(text, style = Typo.heading, color = LocalPalette.current.secondary, modifier = Modifier.weight(1f))
        trailing()
    }
}

@Composable
fun Hairline(modifier: Modifier = Modifier) {
    Box(modifier.fillMaxWidth().height(1.dp).background(LocalPalette.current.hairline))
}

/** The one primary action: pill with a contrasting circular leading icon. */
@Composable
fun PrimaryPill(
    text: String,
    icon: ImageVector,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    color: Color = LocalPalette.current.accent,
) {
    val palette = LocalPalette.current
    val bg by animateColorAsState(if (enabled) color else palette.track, Motion.quick(), label = "pillBg")
    val fg = if (enabled) palette.onAccent else palette.tertiary
    Row(
        modifier = modifier
            .height(44.dp)
            .clip(CircleShape)
            .background(bg)
            .pressable(onClick, enabled)
            .padding(start = Space.xxs, end = Space.l),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            Modifier.size(36.dp).clip(CircleShape).background(fg.copy(alpha = 0.16f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(icon, contentDescription = null, tint = fg, modifier = Modifier.size(18.dp))
        }
        Text(
            text,
            style = Typo.bodyStrong,
            color = fg,
            modifier = Modifier.padding(start = Space.s),
            maxLines = 1
        )
    }
}

/** Secondary action: round neutral icon button. */
@Composable
fun CircleButton(
    icon: ImageVector,
    contentDescription: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    tint: Color = LocalPalette.current.secondary,
    enabled: Boolean = true,
) {
    val palette = LocalPalette.current
    Box(
        modifier = modifier
            .size(36.dp)
            .clip(CircleShape)
            .background(palette.surface)
            .border(1.dp, palette.hairline, CircleShape)
            .pressable(onClick, enabled),
        contentAlignment = Alignment.Center
    ) {
        Icon(icon, contentDescription, tint = if (enabled) tint else palette.tertiary, modifier = Modifier.size(18.dp))
    }
}

/** Secondary text action: neutral pill. */
@Composable
fun SecondaryPill(text: String, icon: ImageVector?, onClick: () -> Unit, modifier: Modifier = Modifier) {
    val palette = LocalPalette.current
    Row(
        modifier = modifier
            .height(36.dp)
            .clip(CircleShape)
            .background(palette.surface)
            .border(1.dp, palette.hairline, CircleShape)
            .pressable(onClick)
            .padding(horizontal = Space.s),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center
    ) {
        if (icon != null) {
            Icon(icon, null, tint = palette.secondary, modifier = Modifier.size(16.dp))
            Box(Modifier.width(Space.xs))
        }
        Text(text, style = Typo.bodyStrong, color = palette.text, maxLines = 1)
    }
}

/** State dot. Breathes only while `live` (something is actually running). */
@Composable
fun StatusDot(color: Color, live: Boolean = false, size: Dp = 8.dp) {
    val reduce = reduceMotion()
    val alpha = if (live && !reduce) {
        val transition = rememberInfiniteTransition(label = "breathe")
        transition.animateFloat(
            initialValue = 0.35f, targetValue = 1f,
            animationSpec = infiniteRepeatable(tween(900), RepeatMode.Reverse), label = "breatheAlpha"
        ).value
    } else 1f
    Box(contentAlignment = Alignment.Center) {
        if (live) {
            Box(Modifier.size(size * 2).clip(CircleShape).background(color.copy(alpha = 0.18f * alpha)))
        }
        Box(Modifier.size(size).clip(CircleShape).background(color.copy(alpha = if (live) 0.5f + 0.5f * alpha else 1f)))
    }
}

/**
 * Pipeline track: one dot per stage, done stages filled, the running stage lit,
 * a failed stage in critical. Labels are centered under their dots.
 */
@Composable
fun StageTrack(
    stages: List<String>,
    current: Int,          // running stage; stages.size when all done; -1 when idle
    color: Color,
    failed: Boolean = false,
    modifier: Modifier = Modifier,
) {
    val palette = LocalPalette.current
    val head by animateFloatAsState(current.coerceIn(0, stages.size - 1).toFloat(), Motion.spring(), label = "head")
    val inset = 8.dp
    fun stageColor(i: Int) = when {
        current < 0 -> palette.track
        failed && i == current -> palette.critical
        i <= current -> color
        else -> palette.track
    }
    Column(modifier.fillMaxWidth()) {
        Canvas(Modifier.fillMaxWidth().height(16.dp)) {
            val n = stages.size
            val y = size.height / 2
            val start = inset.toPx()
            val step = (size.width - 2 * start) / (n - 1).coerceAtLeast(1)
            val dotR = 3.5.dp.toPx()
            val lineW = 2.dp.toPx()
            drawLine(palette.track, Offset(start, y), Offset(size.width - start, y), lineW)
            if (current >= 0) {
                drawLine(if (failed) palette.critical else color, Offset(start, y), Offset(start + head * step, y), lineW)
            }
            for (i in 0 until n) {
                val center = Offset(start + i * step, y)
                if (i == current && current < n) drawCircle(stageColor(i).copy(alpha = 0.22f), dotR * 2.2f, center)
                drawCircle(stageColor(i), dotR, center)
            }
        }
        Layout(
            modifier = Modifier.fillMaxWidth().padding(top = Space.xxs),
            content = {
                stages.forEachIndexed { i, label ->
                    Text(
                        label,
                        style = Typo.caption,
                        color = when {
                            failed && i == current -> palette.critical
                            i == current -> palette.text
                            current >= 0 && i < current -> palette.secondary
                            else -> palette.tertiary
                        },
                        maxLines = 1
                    )
                }
            }
        ) { measurables, constraints ->
            val placeables = measurables.map { it.measure(constraints.copy(minWidth = 0)) }
            val width = constraints.maxWidth
            val start = inset.roundToPx()
            val step = (width - 2 * start) / (placeables.size - 1).coerceAtLeast(1)
            val height = placeables.maxOfOrNull { it.height } ?: 0
            layout(width, height) {
                placeables.forEachIndexed { i, p ->
                    val x = (start + i * step - p.width / 2).coerceIn(0, width - p.width)
                    p.place(x, 0)
                }
            }
        }
    }
}

/** Floating pill segmented control with a sliding selection. */
@Composable
fun Segmented(options: List<Pair<String, ImageVector>>, selected: Int, onSelect: (Int) -> Unit, modifier: Modifier = Modifier) {
    val palette = LocalPalette.current
    val segmentWidth = 112.dp
    val offset by animateFloatAsState(selected.toFloat(), Motion.spring(), label = "segment")
    Box(
        modifier
            .clip(CircleShape)
            .background(if (palette.isDark) Color(0xFF1B1C20) else Color.White)
            .border(1.dp, palette.hairline, CircleShape)
            .padding(Space.xxs)
    ) {
        Box(
            Modifier
                .offset(x = segmentWidth * offset)
                .width(segmentWidth)
                .height(40.dp)
                .clip(CircleShape)
                .background(palette.text.copy(alpha = if (palette.isDark) 0.12f else 0.07f))
        )
        Row {
            options.forEachIndexed { i, (label, icon) ->
                val isSelected = i == selected
                val fg by animateColorAsState(if (isSelected) palette.text else palette.secondary, Motion.quick(), label = "segFg")
                Row(
                    Modifier
                        .width(segmentWidth)
                        .height(40.dp)
                        .clip(CircleShape)
                        .pressable({ onSelect(i) }, role = Role.Tab),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.Center
                ) {
                    Icon(icon, null, tint = fg, modifier = Modifier.size(18.dp))
                    Text(label, style = Typo.bodyStrong, color = fg, modifier = Modifier.padding(start = Space.xs))
                }
            }
        }
    }
}

/** On/off option as a compact pill: outlined when off, accent outline + check when on. */
@Composable
fun TogglePill(label: String, checked: Boolean, onToggle: (Boolean) -> Unit) {
    val palette = LocalPalette.current
    val border by animateColorAsState(if (checked) palette.text.copy(alpha = 0.22f) else palette.hairline, Motion.quick(), label = "tgBorder")
    val bg by animateColorAsState(if (checked) palette.text.copy(alpha = 0.06f) else Color.Transparent, Motion.quick(), label = "tgBg")
    val fg = if (checked) palette.text else palette.secondary
    Row(
        Modifier
            .height(32.dp)
            .clip(CircleShape)
            .background(bg)
            .border(1.dp, border, CircleShape)
            .pressable({ onToggle(!checked) }, role = Role.Checkbox)
            .padding(horizontal = Space.s),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            Modifier
                .size(14.dp)
                .clip(CircleShape)
                .border(1.5.dp, if (checked) palette.text else palette.tertiary, CircleShape)
                .background(if (checked) palette.text else Color.Transparent),
            contentAlignment = Alignment.Center
        ) {
            if (checked) {
                Icon(
                    Icons.Default.Check, null,
                    tint = palette.background, modifier = Modifier.size(10.dp)
                )
            }
        }
        Text(label, style = Typo.caption, color = fg, modifier = Modifier.padding(start = Space.xs), maxLines = 1)
    }
}

/** Aligned label/value row; values that are IDs, sizes, hashes or times use mono. */
@Composable
fun ValueRow(label: String, value: String, mono: Boolean = false, valueColor: Color? = null) {
    val palette = LocalPalette.current
    Row(Modifier.fillMaxWidth().padding(vertical = Space.xxs), verticalAlignment = Alignment.Top) {
        Text(label, style = Typo.caption, color = palette.secondary, modifier = Modifier.width(104.dp))
        Text(
            value,
            style = if (mono) Typo.mono else Typo.caption,
            color = valueColor ?: palette.text,
            modifier = Modifier.weight(1f),
        )
    }
}

@Composable
fun MonoText(text: String, color: Color = LocalPalette.current.secondary, modifier: Modifier = Modifier, maxLines: Int = 1, style: TextStyle = Typo.mono) {
    Text(text, style = style, color = color, modifier = modifier, maxLines = maxLines, overflow = TextOverflow.Ellipsis)
}

/** A screen header: title, one-line live status, optional leading/trailing buttons. */
@Composable
fun ScreenHeader(
    title: String,
    status: String,
    statusColor: Color,
    live: Boolean = false,
    leading: (@Composable () -> Unit)? = null,
    trailing: @Composable RowScope.() -> Unit = {},
) {
    val palette = LocalPalette.current
    Row(Modifier.fillMaxWidth().padding(top = Space.xs, bottom = Space.xxs), verticalAlignment = Alignment.CenterVertically) {
        if (leading != null) {
            leading()
            Box(Modifier.width(Space.s))
        }
        Column(Modifier.weight(1f)) {
            Text(title, style = Typo.title, color = palette.text, maxLines = 1, overflow = TextOverflow.Ellipsis)
            Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(top = 2.dp)) {
                StatusDot(statusColor, live, size = 6.dp)
                Text(
                    status,
                    style = Typo.caption,
                    color = palette.secondary,
                    modifier = Modifier.padding(start = Space.xs),
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
            }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(Space.xs), content = trailing)
    }
}
