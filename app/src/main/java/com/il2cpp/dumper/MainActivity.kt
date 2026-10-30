package com.il2cpp.dumper

import android.app.Application
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.AnimatedContentTransitionScope
import androidx.compose.animation.EnterTransition
import androidx.compose.animation.ExitTransition
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Memory
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavBackStackEntry
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.il2cpp.dumper.navigation.Screen
import com.il2cpp.dumper.ui.components.Segmented
import com.il2cpp.dumper.ui.screens.AboutScreen
import com.il2cpp.dumper.ui.screens.HomeScreen
import com.il2cpp.dumper.ui.screens.JobDetailScreen
import com.il2cpp.dumper.ui.screens.JobHistoryScreen
import com.il2cpp.dumper.ui.screens.LicensesScreen
import com.il2cpp.dumper.ui.theme.Il2CppDumperTheme
import com.il2cpp.dumper.ui.theme.LocalPalette
import com.il2cpp.dumper.ui.theme.Space
import com.il2cpp.dumper.viewmodel.DumperViewModel
import com.il2cpp.dumper.viewmodel.DumperViewModelFactory
import com.il2cpp.dumper.viewmodel.JobDetailViewModel
import com.il2cpp.dumper.viewmodel.JobDetailViewModelFactory
import com.il2cpp.dumper.viewmodel.JobHistoryViewModel
import com.il2cpp.dumper.viewmodel.JobHistoryViewModelFactory

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            Il2CppDumperTheme {
                AppNavigation()
            }
        }
    }
}

private val TabRoutes = listOf(Screen.Home.route, Screen.JobHistory.route)

private fun AnimatedContentTransitionScope<NavBackStackEntry>.isTabSwitch() =
    initialState.destination.route in TabRoutes && targetState.destination.route in TabRoutes

@Composable
fun AppNavigation() {
    val navController = rememberNavController()
    val app = LocalContext.current.applicationContext as Application
    val dumperViewModel: DumperViewModel = viewModel(factory = DumperViewModelFactory(app))
    val jobHistoryViewModel: JobHistoryViewModel = viewModel(factory = JobHistoryViewModelFactory(app))

    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = navBackStackEntry?.destination?.route
    val tabIndex = TabRoutes.indexOf(currentRoute)

    fun selectTab(index: Int) {
        val route = TabRoutes[index]
        if (currentRoute != route) {
            navController.navigate(route) {
                popUpTo(Screen.Home.route) { saveState = true }
                launchSingleTop = true
                restoreState = true
            }
        }
    }

    Box(Modifier.fillMaxSize().background(LocalPalette.current.background)) {
        NavHost(
            navController = navController,
            startDestination = Screen.Home.route,
            modifier = Modifier.fillMaxSize(),
            // Tabs crossfade with a small offset; pushed screens slide.
            enterTransition = {
                if (isTabSwitch()) fadeIn(tween(220)) + slideInVertically(tween(220)) { it / 40 }
                else slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.Left, tween(280)) + fadeIn(tween(280))
            },
            exitTransition = {
                if (isTabSwitch()) fadeOut(tween(160))
                else slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.Left, tween(280)) { it / 4 } + fadeOut(tween(280))
            },
            popEnterTransition = {
                if (isTabSwitch()) fadeIn(tween(220))
                else slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.Right, tween(280)) { it / 4 } + fadeIn(tween(280))
            },
            popExitTransition = {
                if (isTabSwitch()) fadeOut(tween(160))
                else slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.Right, tween(280)) + fadeOut(tween(280))
            }
        ) {
            composable(Screen.Home.route) {
                HomeScreen(
                    viewModel = dumperViewModel,
                    onNavigateToJob = { jobId -> navController.navigate(Screen.JobDetail.createRoute(jobId)) },
                    onOpenAbout = { navController.navigate(Screen.About.route) }
                )
            }

            composable(Screen.JobHistory.route) {
                JobHistoryScreen(
                    viewModel = jobHistoryViewModel,
                    onJobClick = { jobId -> navController.navigate(Screen.JobDetail.createRoute(jobId)) },
                    onStartDump = { selectTab(0) }
                )
            }

            composable(
                route = Screen.JobDetail.route,
                arguments = listOf(navArgument("jobId") { type = NavType.LongType })
            ) { backStackEntry ->
                val jobId = backStackEntry.arguments?.getLong("jobId") ?: return@composable
                val jobDetailViewModel: JobDetailViewModel = viewModel(
                    factory = JobDetailViewModelFactory(app, jobId),
                    key = "jobDetail_$jobId"
                )
                JobDetailScreen(
                    viewModel = jobDetailViewModel,
                    onBack = { navController.popBackStack() }
                )
            }

            composable(Screen.About.route) {
                AboutScreen(
                    onBack = { navController.popBackStack() },
                    onNavigateToLicenses = { navController.navigate(Screen.Licenses.route) }
                )
            }

            composable(Screen.Licenses.route) {
                LicensesScreen(onBack = { navController.popBackStack() })
            }
        }

        AnimatedVisibility(
            visible = tabIndex >= 0,
            enter = fadeIn(tween(220)) + slideInVertically(tween(220)) { it / 2 },
            exit = fadeOut(tween(160)) + slideOutVertically(tween(160)) { it / 2 },
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .navigationBarsPadding()
                .padding(bottom = Space.s)
        ) {
            Segmented(
                options = listOf("Dump" to Icons.Default.Memory, "History" to Icons.Default.History),
                selected = tabIndex.coerceAtLeast(0),
                onSelect = ::selectTab
            )
        }
    }
}
