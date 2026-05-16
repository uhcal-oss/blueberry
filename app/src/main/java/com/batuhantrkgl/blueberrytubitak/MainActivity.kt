package com.batuhantrkgl.blueberrytubitak

import android.Manifest
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.lifecycle.viewmodel.compose.viewModel
import com.batuhantrkgl.blueberrytubitak.ui.theme.BlueberryTubitakTheme
import com.google.firebase.messaging.FirebaseMessaging
import compose.icons.TablerIcons
import compose.icons.tablericons.Activity
import compose.icons.tablericons.AlertTriangle
import compose.icons.tablericons.Bell
import compose.icons.tablericons.Building
import compose.icons.tablericons.Clock
import compose.icons.tablericons.Code
import compose.icons.tablericons.Flame
import compose.icons.tablericons.InfoCircle
import compose.icons.tablericons.Moon
import compose.icons.tablericons.Pencil
import compose.icons.tablericons.Settings
import compose.icons.tablericons.ShieldCheck
import compose.icons.tablericons.Trash
import kotlinx.coroutines.delay
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        installSplashScreen()
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        val sharedPrefs = getSharedPreferences("BlueberryPrefs", Context.MODE_PRIVATE)

        FirebaseMessaging.getInstance().token.addOnCompleteListener { task ->
            if (task.isSuccessful) {
                Log.d("FCM_TOKEN", "Token: ${task.result}")
            }
        }

        setContent {
            var isDarkMode by remember { mutableStateOf(sharedPrefs.getBoolean("dark_mode", true)) }

            BlueberryTubitakTheme(darkTheme = isDarkMode) {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    MainScreen(
                        modifier = Modifier.padding(innerPadding),
                        isDarkMode = isDarkMode,
                        onDarkModeToggle = {
                            isDarkMode = it
                            sharedPrefs.edit().putBoolean("dark_mode", it).apply()
                        }
                    )
                }
            }
        }
    }
}

// --- Navigation ---
enum class MainTab { DASHBOARD, ALERTS, SETTINGS }

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    modifier: Modifier = Modifier,
    isDarkMode: Boolean,
    onDarkModeToggle: (Boolean) -> Unit
) {
    val permissionsToRequest = remember {
        mutableListOf(
            Manifest.permission.CALL_PHONE,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION
        ).apply {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                add(Manifest.permission.POST_NOTIFICATIONS)
            }
        }.toTypedArray()
    }

    var permissionsGranted by remember { mutableStateOf(false) }
    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestMultiplePermissions(),
        onResult = { permissions -> permissionsGranted = permissions.values.all { it } }
    )
    LaunchedEffect(Unit) { permissionLauncher.launch(permissionsToRequest) }

    val context = LocalContext.current
    val mainViewModel: MainViewModel = viewModel()
    val alerts by mainViewModel.alerts.collectAsState()
    var selectedTab by remember { mutableStateOf(MainTab.DASHBOARD) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        when (selectedTab) {
                            MainTab.DASHBOARD -> "Gösterge Paneli"
                            MainTab.ALERTS -> "Uyarı Geçmişi"
                            MainTab.SETTINGS -> "Ayarlar"
                        }
                    )
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        },
        bottomBar = {
            NavigationBar {
                NavigationBarItem(
                    selected = selectedTab == MainTab.DASHBOARD,
                    onClick = { selectedTab = MainTab.DASHBOARD },
                    icon = { Icon(TablerIcons.Activity, contentDescription = "Gösterge") },
                    label = { Text("Gösterge") }
                )
                NavigationBarItem(
                    selected = selectedTab == MainTab.ALERTS,
                    onClick = { selectedTab = MainTab.ALERTS },
                    icon = {
                        BadgedBox(badge = {
                            if (alerts.isNotEmpty()) Badge { Text("${alerts.size}") }
                        }) {
                            Icon(TablerIcons.Bell, contentDescription = "Uyarılar")
                        }
                    },
                    label = { Text("Uyarılar") }
                )
                NavigationBarItem(
                    selected = selectedTab == MainTab.SETTINGS,
                    onClick = { selectedTab = MainTab.SETTINGS },
                    icon = { Icon(TablerIcons.Settings, contentDescription = "Ayarlar") },
                    label = { Text("Ayarlar") }
                )
            }
        },
        floatingActionButton = {
            if (selectedTab == MainTab.DASHBOARD) {
                ExtendedFloatingActionButton(
                    onClick = {
                        val intent = Intent(context, EmergencyActionActivity::class.java).apply {
                            putExtra("coordinates", "40.9785° K, 29.0833° D (Test)")
                            putExtra("status", "Manuel Yangın Test Alarmı")
                            putExtra("flame", "250 (Alarm)")
                            putExtra("mq2", "850 (Kritik)")
                            putExtra("mq7", "600 (Yüksek)")
                        }
                        context.startActivity(intent)
                    },
                    icon = { Icon(TablerIcons.Flame, contentDescription = "Test Alarmı") },
                    text = { Text("Test Alarmı") },
                    containerColor = MaterialTheme.colorScheme.error,
                    contentColor = MaterialTheme.colorScheme.onError
                )
            }
        }
    ) { innerPadding ->
        AnimatedContent(
            targetState = selectedTab,
            transitionSpec = { fadeIn() togetherWith fadeOut() },
            label = "tab_transition"
        ) { tab ->
            when (tab) {
                MainTab.DASHBOARD -> DashboardTab(
                    modifier = modifier.padding(innerPadding),
                    permissionsGranted = permissionsGranted,
                    onRequestPermissions = { permissionLauncher.launch(permissionsToRequest) }
                )
                MainTab.ALERTS -> AlertsTab(
                    modifier = modifier.padding(innerPadding),
                    alerts = alerts,
                    onClearAll = { mainViewModel.clearAlerts() },
                    onDeleteAlert = { mainViewModel.deleteAlert(it) }
                )
                MainTab.SETTINGS -> SettingsTab(
                    modifier = modifier.padding(innerPadding),
                    isDarkMode = isDarkMode,
                    onDarkModeToggle = onDarkModeToggle
                )
            }
        }
    }
}

// --- Dashboard ---

@Composable
fun DashboardTab(
    modifier: Modifier = Modifier,
    permissionsGranted: Boolean,
    onRequestPermissions: () -> Unit
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        if (!permissionsGranted) {
            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(16.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.errorContainer)
            ) {
                Row(modifier = Modifier.padding(16.dp), verticalAlignment = Alignment.Top) {
                    Icon(
                        imageVector = TablerIcons.AlertTriangle,
                        contentDescription = null,
                        tint = MaterialTheme.colorScheme.onErrorContainer,
                        modifier = Modifier.size(24.dp)
                    )
                    Spacer(modifier = Modifier.width(12.dp))
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "Uygulama İzinleri Eksik",
                            fontWeight = FontWeight.Bold,
                            style = MaterialTheme.typography.titleSmall,
                            color = MaterialTheme.colorScheme.onErrorContainer
                        )
                        Spacer(modifier = Modifier.height(4.dp))
                        Text(
                            text = "Acil arama, SMS gönderimi ve konum tespiti için gerekli izinler verilmedi.",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onErrorContainer
                        )
                        Spacer(modifier = Modifier.height(12.dp))
                        Button(
                            onClick = onRequestPermissions,
                            colors = ButtonDefaults.buttonColors(
                                containerColor = MaterialTheme.colorScheme.error,
                                contentColor = MaterialTheme.colorScheme.onError
                            ),
                            modifier = Modifier.fillMaxWidth()
                        ) { Text("İzinleri Ver") }
                    }
                }
            }
        }

        ConnectionStatusCard()
        SystemHealthCard()
        Spacer(modifier = Modifier.height(80.dp))
    }
}

// --- Alerts ---

@Composable
fun relativeTimeString(timestamp: Long): String {
    var now by remember { mutableStateOf(System.currentTimeMillis()) }
    LaunchedEffect(Unit) {
        while (true) {
            delay(30_000L)
            now = System.currentTimeMillis()
        }
    }
    val diff = now - timestamp
    return when {
        diff < 60_000L -> "Az önce"
        diff < 3_600_000L -> "${diff / 60_000L} dk önce"
        diff < 86_400_000L -> "${diff / 3_600_000L} sa önce"
        else -> SimpleDateFormat("dd MMM", Locale("tr")).format(Date(timestamp))
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AlertsTab(
    modifier: Modifier = Modifier,
    alerts: List<AlertEntity>,
    onClearAll: () -> Unit,
    onDeleteAlert: (Int) -> Unit
) {
    val context = LocalContext.current

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        if (alerts.isNotEmpty()) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    "${alerts.size} uyarı",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                TextButton(onClick = onClearAll) {
                    Text("Tümünü Temizle", color = MaterialTheme.colorScheme.error)
                }
            }
        }

        Card(
            modifier = Modifier.fillMaxWidth(),
            shape = RoundedCornerShape(16.dp),
            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
        ) {
            Column(modifier = Modifier.padding(20.dp)) {
                if (alerts.isEmpty()) {
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 48.dp),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        Icon(
                            imageVector = TablerIcons.ShieldCheck,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.tertiary,
                            modifier = Modifier.size(80.dp)
                        )
                        Spacer(modifier = Modifier.height(16.dp))
                        Text(
                            "Son uyarı yok. Sistem sakin.",
                            style = MaterialTheme.typography.titleMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            textAlign = TextAlign.Center
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Text(
                            "Tüm sensörler normal aralıkta çalışıyor.",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            textAlign = TextAlign.Center
                        )
                    }
                } else {
                    alerts.forEachIndexed { index, alert ->
                        val severityColor = if (alert.severity == "KRITIK")
                            MaterialTheme.colorScheme.error
                        else
                            Color(0xFFFFC107)
                        val severityIcon = if (alert.severity == "KRITIK")
                            TablerIcons.Flame
                        else
                            TablerIcons.AlertTriangle

                        val dismissState = rememberSwipeToDismissBoxState(
                            confirmValueChange = { value ->
                                if (value != SwipeToDismissBoxValue.Settled) {
                                    onDeleteAlert(alert.id)
                                    true
                                } else false
                            }
                        )

                        SwipeToDismissBox(
                            state = dismissState,
                            backgroundContent = {
                                Box(
                                    modifier = Modifier
                                        .fillMaxSize()
                                        .background(MaterialTheme.colorScheme.errorContainer),
                                    contentAlignment = Alignment.CenterEnd
                                ) {
                                    Icon(
                                        imageVector = TablerIcons.Trash,
                                        contentDescription = "Sil",
                                        tint = MaterialTheme.colorScheme.error,
                                        modifier = Modifier.padding(horizontal = 20.dp)
                                    )
                                }
                            }
                        ) {
                            ListItem(
                                headlineContent = {
                                    Text(alert.title, fontWeight = FontWeight.Bold)
                                },
                                supportingContent = {
                                    Column {
                                        Text(alert.body)
                                        if (alert.coordinates.isNotEmpty()) {
                                            Spacer(modifier = Modifier.height(4.dp))
                                            TextButton(
                                                onClick = {
                                                    val uri = Uri.parse("geo:0,0?q=${Uri.encode(alert.coordinates)}")
                                                    try {
                                                        context.startActivity(Intent(Intent.ACTION_VIEW, uri))
                                                    } catch (_: Exception) {}
                                                },
                                                contentPadding = PaddingValues(0.dp)
                                            ) {
                                                Text(
                                                    "🗺 Haritada Gör",
                                                    style = MaterialTheme.typography.bodySmall,
                                                    color = MaterialTheme.colorScheme.primary
                                                )
                                            }
                                        }
                                    }
                                },
                                trailingContent = {
                                    Text(
                                        relativeTimeString(alert.timestamp),
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant
                                    )
                                },
                                leadingContent = {
                                    Box(
                                        contentAlignment = Alignment.Center,
                                        modifier = Modifier
                                            .size(40.dp)
                                            .background(severityColor.copy(alpha = 0.15f), CircleShape)
                                    ) {
                                        Icon(
                                            imageVector = severityIcon,
                                            contentDescription = null,
                                            tint = severityColor,
                                            modifier = Modifier.size(20.dp)
                                        )
                                    }
                                }
                            )
                        }
                        if (index < alerts.size - 1) HorizontalDivider()
                    }
                }
            }
        }
    }
}

// --- Settings ---

@Composable
fun SettingsTab(
    modifier: Modifier = Modifier,
    isDarkMode: Boolean,
    onDarkModeToggle: (Boolean) -> Unit
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        DarkModeCard(isDarkMode = isDarkMode, onToggle = onDarkModeToggle)
        CountdownSettingCard()
        StationProfileCard()
        DebugInfoCard()
        AboutCard()
        Spacer(modifier = Modifier.height(24.dp))
    }
}

@Composable
fun DarkModeCard(isDarkMode: Boolean, onToggle: (Boolean) -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Row(
            modifier = Modifier
                .padding(20.dp)
                .fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.weight(1f)
            ) {
                Icon(
                    imageVector = TablerIcons.Moon,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(12.dp))
                Column {
                    Text(
                        "Karanlık Mod",
                        style = MaterialTheme.typography.titleSmall,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        "Uygulamanın tema tercihini değiştir",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
            Switch(checked = isDarkMode, onCheckedChange = onToggle)
        }
    }
}

@Composable
fun CountdownSettingCard() {
    val context = LocalContext.current
    val sharedPrefs = context.getSharedPreferences("BlueberryPrefs", Context.MODE_PRIVATE)
    var countdownSeconds by remember { mutableStateOf(sharedPrefs.getInt("countdown_seconds", 10)) }

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.Clock,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "Geri Sayım Süresi",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                "Acil durum SMS'i göndermeden önce beklenecek süre",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(16.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                listOf(10, 20, 30, 60).forEach { seconds ->
                    FilterChip(
                        selected = countdownSeconds == seconds,
                        onClick = {
                            countdownSeconds = seconds
                            sharedPrefs.edit().putInt("countdown_seconds", seconds).apply()
                        },
                        label = { Text("${seconds}s") },
                        modifier = Modifier.weight(1f)
                    )
                }
            }
        }
    }
}

@Composable
fun AboutCard() {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.InfoCircle,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "Hakkında",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                "Blueberry-Tubitak",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold
            )
            Text(
                "Versiyon 1.0.0",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(12.dp))
            HorizontalDivider()
            Spacer(modifier = Modifier.height(12.dp))
            Text(
                "TÜBİTAK Otonom Orman Yangını Tespit Projesi",
                style = MaterialTheme.typography.bodyMedium,
                fontWeight = FontWeight.Medium
            )
            Spacer(modifier = Modifier.height(4.dp))
            Text(
                "Blueberry-1 İstasyonu — İzmir, Türkiye"
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                "Arduino + Firebase Bulut Mesajlaşma mimarisi üzerine inşa edilmiş otonom orman yangını erken uyarı sistemi.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

// --- Cards ---

@Composable
fun ConnectionStatusCard() {
    val context = LocalContext.current
    val sharedPrefs = context.getSharedPreferences("BlueberryPrefs", Context.MODE_PRIVATE)
    val lastConnectionMs = sharedPrefs.getLong("last_connection_timestamp", 0L)

    val infiniteTransition = rememberInfiniteTransition(label = "connection_pulse")
    val pulseScale by infiniteTransition.animateFloat(
        initialValue = 0.8f,
        targetValue = 1.2f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        ),
        label = "dot_scale"
    )
    val pulseAlpha by infiniteTransition.animateFloat(
        initialValue = 0.4f,
        targetValue = 1.0f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        ),
        label = "dot_alpha"
    )

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.Activity,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "Bağlantı Durumu",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier
                        .size(10.dp)
                        .scale(pulseScale)
                        .alpha(pulseAlpha)
                        .background(MaterialTheme.colorScheme.tertiary, CircleShape)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text("Durum: Arduino Dinleniyor", style = MaterialTheme.typography.bodyMedium)
            }
            Spacer(modifier = Modifier.height(8.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier
                        .size(10.dp)
                        .scale(pulseScale)
                        .alpha(pulseAlpha)
                        .background(MaterialTheme.colorScheme.tertiary, CircleShape)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text("FCM: Bağlı", style = MaterialTheme.typography.bodyMedium)
            }
            if (lastConnectionMs > 0L) {
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "Son bağlantı: ${relativeTimeString(lastConnectionMs)}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else {
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "Henüz bağlantı yok",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
fun SystemHealthCard() {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.ShieldCheck,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.tertiary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "İstasyon Sağlık & Diagnostik",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(20.dp))
            SensorGaugeItem(name = "Alev Sensörü (A0)", value = 145f, threshold = 400f)
            Spacer(modifier = Modifier.height(16.dp))
            SensorGaugeItem(name = "MQ-2 Duman Sensörü (A1)", value = 180f, threshold = 800f)
            Spacer(modifier = Modifier.height(16.dp))
            SensorGaugeItem(name = "MQ-7 Karbon Monoksit (A2)", value = 95f, threshold = 500f)
            Spacer(modifier = Modifier.height(16.dp))
            HorizontalDivider()
            Spacer(modifier = Modifier.height(12.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier
                        .size(8.dp)
                        .background(MaterialTheme.colorScheme.tertiary, CircleShape)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text("Wi-Fi Bağlantısı: Aktif", style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}

@Composable
fun SensorGaugeItem(
    name: String,
    value: Float,
    threshold: Float,
    modifier: Modifier = Modifier
) {
    val ratio = (value / threshold).coerceIn(0f, 1f)
    val indicatorColor = when {
        ratio < 0.5f -> MaterialTheme.colorScheme.tertiary
        ratio < 0.8f -> Color(0xFFFFC107)
        else -> MaterialTheme.colorScheme.error
    }
    val statusText = when {
        ratio < 0.5f -> "Normal"
        ratio < 0.8f -> "Uyarı"
        else -> "Kritik"
    }

    Column(modifier = modifier) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                name,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier
                        .size(6.dp)
                        .background(indicatorColor, CircleShape)
                )
                Spacer(modifier = Modifier.width(4.dp))
                Text(
                    "$statusText (${value.toInt()}/${threshold.toInt()})",
                    style = MaterialTheme.typography.bodySmall,
                    color = indicatorColor,
                    fontWeight = FontWeight.Bold
                )
            }
        }
        Spacer(modifier = Modifier.height(6.dp))
        LinearProgressIndicator(
            progress = { ratio },
            modifier = Modifier
                .fillMaxWidth()
                .height(6.dp)
                .clip(RoundedCornerShape(3.dp)),
            color = indicatorColor,
            trackColor = MaterialTheme.colorScheme.surfaceVariant
        )
    }
}

@Composable
fun StationProfileCard() {
    val context = LocalContext.current
    val sharedPrefs = context.getSharedPreferences("BlueberryPrefs", Context.MODE_PRIVATE)
    var emergencyNumber by remember { mutableStateOf(sharedPrefs.getString("emergency_number", "112") ?: "112") }
    var showDialog by remember { mutableStateOf(false) }

    if (showDialog) {
        var textValue by remember { mutableStateOf(emergencyNumber) }
        AlertDialog(
            onDismissRequest = { showDialog = false },
            title = { Text("Acil Numarayı Ayarla") },
            text = {
                OutlinedTextField(
                    value = textValue,
                    onValueChange = { textValue = it },
                    label = { Text("Telefon Numarası") },
                    singleLine = true,
                    keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(
                        keyboardType = androidx.compose.ui.text.input.KeyboardType.Phone
                    )
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    sharedPrefs.edit().putString("emergency_number", textValue).apply()
                    emergencyNumber = textValue
                    showDialog = false
                }) { Text("Kaydet") }
            },
            dismissButton = {
                TextButton(onClick = { showDialog = false }) { Text("İptal") }
            }
        )
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.Building,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "İstasyon Detayları",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                "İstasyon: İzmir / Blueberry-1"
                fontWeight = FontWeight.Bold,
                style = MaterialTheme.typography.bodyLarge
            )
            Text(
                "Bölge: Çam Ormanı Alfa",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(12.dp))
            HorizontalDivider()
            Spacer(modifier = Modifier.height(12.dp))
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.fillMaxWidth()
            ) {
                Icon(
                    imageVector = TablerIcons.Flame,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.secondary,
                    modifier = Modifier.size(20.dp)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        "Acil Müdahale: $emergencyNumber",
                        fontWeight = FontWeight.Bold,
                        style = MaterialTheme.typography.bodyMedium
                    )
                    Text(
                        "Otonom SMS aktif",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
                IconButton(
                    onClick = { showDialog = true },
                    modifier = Modifier.size(32.dp)
                ) {
                    Icon(
                        imageVector = TablerIcons.Pencil,
                        contentDescription = "Düzenle",
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }
        }
    }
}

@Composable
fun DebugInfoCard() {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
    ) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = TablerIcons.Code,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    "Hata Ayıklama Bilgisi",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text("Uygulama Versiyonu: 1.0.0", style = MaterialTheme.typography.bodySmall)
            Text("Cihaz SDK: ${Build.VERSION.SDK_INT}", style = MaterialTheme.typography.bodySmall)
            var fcmToken by remember { mutableStateOf("Alınıyor...") }
            LaunchedEffect(Unit) {
                FirebaseMessaging.getInstance().token.addOnCompleteListener { task ->
                    fcmToken = if (task.isSuccessful) task.result ?: "Bilinmiyor" else "Alınamadı"
                }
            }
            Text(
                "FCM Token (kısmi): ${fcmToken.take(15)}...",
                style = MaterialTheme.typography.bodySmall
            )
        }
    }
}
