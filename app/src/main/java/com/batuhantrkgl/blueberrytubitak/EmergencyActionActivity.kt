package com.batuhantrkgl.blueberrytubitak

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.telephony.SmsManager
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.core.*
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.batuhantrkgl.blueberrytubitak.ui.theme.BlueberryTubitakTheme
import kotlinx.coroutines.delay

class EmergencyActionActivity : ComponentActivity() {

    private lateinit var ttsManager: EmergencyTTSManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        window.addFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or
            WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD or
            WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED or
            WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
        )

        ttsManager = EmergencyTTSManager(this)

        val coordinates = intent.getStringExtra("coordinates") ?: "Bilinmiyor"
        val status = intent.getStringExtra("status") ?: "Acil Durum"
        val flame = intent.getStringExtra("flame") ?: "Bilinmiyor"
        val mq2 = intent.getStringExtra("mq2") ?: "Bilinmiyor"
        val mq7 = intent.getStringExtra("mq7") ?: "Bilinmiyor"

        setContent {
            BlueberryTubitakTheme {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(
                            Brush.verticalGradient(
                                colors = listOf(
                                    Color(0xFF5C0000),
                                    Color(0xFF2A0000),
                                    Color(0xFF0F0000)
                                )
                            )
                        )
                ) {
                    EmergencyActionScreen(
                        coordinates = coordinates,
                        status = status,
                        flame = flame,
                        mq2 = mq2,
                        mq7 = mq7,
                        onDeny = { finish() },
                        onTakeOver = { makePhoneCall() },
                        onAccept = { sendEmergencySms(coordinates, status, flame, mq2, mq7) }
                    )
                }
            }
        }
    }

    private fun makePhoneCall() {
        val sharedPrefs = getSharedPreferences("BlueberryPrefs", android.content.Context.MODE_PRIVATE)
        val targetNumber = sharedPrefs.getString("emergency_number", "112") ?: "112"
        val callIntent = Intent(Intent.ACTION_CALL).apply {
            data = Uri.parse("tel:$targetNumber")
        }
        try {
            startActivity(callIntent)
        } catch (e: SecurityException) {
            e.printStackTrace()
        }
    }

    private fun sendEmergencySms(coordinates: String, emergencyType: String, flame: String, mq2: String, mq7: String) {
        val sharedPrefs = getSharedPreferences("BlueberryPrefs", android.content.Context.MODE_PRIVATE)
        val targetNumber = sharedPrefs.getString("emergency_number", "112") ?: "112"

        val message = "DİKKAT. Otonom orman yangını tespit sistemi uyarısındasınız.\n" +
                "Durum: $emergencyType\n" +
                "Koordinatlar: $coordinates\n" +
                "Sensör Verisi -> Alev: $flame, Duman: $mq2, CO: $mq7\n" +
                "Lütfen hemen bölgeye müdahale ekibi gönderin."

        try {
            val smsManager = SmsManager.getDefault()
            val parts = smsManager.divideMessage(message)
            smsManager.sendMultipartTextMessage(targetNumber, null, parts, null, null)
            Toast.makeText(this, "Acil durum SMS'i başarıyla gönderildi.", Toast.LENGTH_LONG).show()
        } catch (e: Exception) {
            e.printStackTrace()
            Toast.makeText(this, "SMS gönderimi başarısız: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    override fun onDestroy() {
        ttsManager.shutdown()
        super.onDestroy()
    }
}

enum class EmergencyViewState {
    COUNTDOWN, TAKEN_OVER, AUTOMATED
}

@Composable
fun EmergencyActionScreen(
    coordinates: String,
    status: String,
    flame: String,
    mq2: String,
    mq7: String,
    onDeny: () -> Unit,
    onTakeOver: () -> Unit,
    onAccept: () -> Unit
) {
    val context = androidx.compose.ui.platform.LocalContext.current
    val sharedPrefs = remember { context.getSharedPreferences("BlueberryPrefs", android.content.Context.MODE_PRIVATE) }
    val countdown = remember { sharedPrefs.getInt("countdown_seconds", 10) }
    var timeLeft by remember { mutableStateOf(countdown) }
    var viewState by remember { mutableStateOf(EmergencyViewState.COUNTDOWN) }

    val infiniteTransition = rememberInfiniteTransition(label = "pulse")
    val scale by infiniteTransition.animateFloat(
        initialValue = 1f,
        targetValue = 1.1f,
        animationSpec = infiniteRepeatable(
            animation = tween(800, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        ),
        label = "pulse_scale"
    )

    LaunchedEffect(viewState) {
        if (viewState == EmergencyViewState.COUNTDOWN) {
            while (timeLeft > 0) {
                delay(1000L)
                timeLeft -= 1
            }
            if (timeLeft == 0 && viewState == EmergencyViewState.COUNTDOWN) {
                viewState = EmergencyViewState.AUTOMATED
                onAccept()
            }
        }
    }

    val dispatchNumber = sharedPrefs.getString("emergency_number", "112") ?: "112"

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        when (viewState) {
            EmergencyViewState.COUNTDOWN -> {
                Text(
                    text = "🚨 ACİL DURUM TESPİT EDİLDİ 🚨",
                    style = MaterialTheme.typography.headlineMedium,
                    color = MaterialTheme.colorScheme.error,
                    fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center,
                    modifier = Modifier
                        .padding(bottom = 32.dp)
                        .graphicsLayer(scaleX = scale, scaleY = scale)
                )

                Box(contentAlignment = Alignment.Center, modifier = Modifier.size(160.dp)) {
                    CircularProgressIndicator(
                        progress = { if (countdown > 0) timeLeft.toFloat() / countdown.toFloat() else 0f },
                        modifier = Modifier.fillMaxSize(),
                        color = MaterialTheme.colorScheme.error,
                        strokeWidth = 8.dp,
                        trackColor = Color.White.copy(alpha = 0.12f)
                    )
                    Text(
                        text = "$timeLeft",
                        style = MaterialTheme.typography.displayLarge,
                        fontWeight = FontWeight.Bold,
                        color = Color.White
                    )
                }

                Spacer(modifier = Modifier.height(32.dp))

                Text(
                    text = "saniye içinde $dispatchNumber numarasına otonom SMS gönderilecek.",
                    style = MaterialTheme.typography.bodyLarge,
                    color = Color.White.copy(alpha = 0.87f),
                    textAlign = TextAlign.Center
                )

                Spacer(modifier = Modifier.height(48.dp))

                Button(
                    onClick = {
                        viewState = EmergencyViewState.AUTOMATED
                        onAccept()
                    },
                    modifier = Modifier.fillMaxWidth().height(64.dp),
                    shape = RoundedCornerShape(16.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
                ) {
                    Text(
                        "Kabul Et (Otonom SMS Gönder)",
                        fontWeight = FontWeight.Bold,
                        style = MaterialTheme.typography.titleMedium
                    )
                }

                Spacer(modifier = Modifier.height(16.dp))

                Button(
                    onClick = {
                        viewState = EmergencyViewState.TAKEN_OVER
                        onTakeOver()
                    },
                    modifier = Modifier.fillMaxWidth().height(56.dp),
                    shape = RoundedCornerShape(16.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary)
                ) {
                    Text("Devral (Ben Konuşacağım)")
                }

                Spacer(modifier = Modifier.height(16.dp))

                OutlinedButton(
                    onClick = { onDeny() },
                    shape = RoundedCornerShape(16.dp),
                    modifier = Modifier.fillMaxWidth().height(56.dp),
                    border = BorderStroke(1.dp, Color.White.copy(alpha = 0.4f))
                ) {
                    Text("Reddet (İptal Et)", color = Color.White.copy(alpha = 0.8f))
                }
            }

            EmergencyViewState.TAKEN_OVER -> {
                Text(
                    text = "📞 Çağrıyı Devraldınız",
                    style = MaterialTheme.typography.headlineMedium,
                    color = MaterialTheme.colorScheme.primary,
                    fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center
                )
                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    text = "Lütfen yangın ihbar operatörüne aşağıdaki bilgileri okuyun:",
                    style = MaterialTheme.typography.bodyLarge,
                    textAlign = TextAlign.Center,
                    color = Color.White.copy(alpha = 0.87f)
                )
                Spacer(modifier = Modifier.height(32.dp))
                EmergencyDetailsCard(status, coordinates, flame, mq2, mq7)
            }

            EmergencyViewState.AUTOMATED -> {
                Text(
                    text = "📩 Otonom SMS Gönderildi",
                    style = MaterialTheme.typography.headlineMedium,
                    color = MaterialTheme.colorScheme.error,
                    fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center
                )
                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    text = "Aşağıdaki bilgiler acil durum servislerine kısa mesaj (SMS) olarak iletildi.",
                    style = MaterialTheme.typography.bodyLarge,
                    textAlign = TextAlign.Center,
                    color = Color.White.copy(alpha = 0.87f)
                )
                Spacer(modifier = Modifier.height(32.dp))
                EmergencyDetailsCard(status, coordinates, flame, mq2, mq7)
                Spacer(modifier = Modifier.height(32.dp))
                OutlinedButton(
                    onClick = { onDeny() },
                    modifier = Modifier.fillMaxWidth().height(56.dp),
                    shape = RoundedCornerShape(16.dp),
                    border = BorderStroke(1.dp, Color.White.copy(alpha = 0.4f))
                ) {
                    Text("Otonom Sistemi Durdur ve Kapat", color = Color.White.copy(alpha = 0.8f))
                }
            }
        }
    }
}

@Composable
fun SensorDetailRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = Color.White.copy(alpha = 0.6f)
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.error
        )
    }
}

@Composable
fun EmergencyDetailsCard(status: String, coordinates: String, flame: String, mq2: String, mq7: String) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White.copy(alpha = 0.08f))
    ) {
        Column(modifier = Modifier.padding(24.dp)) {
            Text(
                text = "DURUM",
                fontWeight = FontWeight.Bold,
                style = MaterialTheme.typography.labelMedium,
                color = MaterialTheme.colorScheme.primary,
                letterSpacing = androidx.compose.ui.unit.TextUnit.Unspecified
            )
            Spacer(modifier = Modifier.height(4.dp))
            Text(text = status, style = MaterialTheme.typography.bodyLarge, color = Color.White)

            Spacer(modifier = Modifier.height(16.dp))

            Text(
                text = "KOORDİNATLAR",
                fontWeight = FontWeight.Bold,
                style = MaterialTheme.typography.labelMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(4.dp))
            Text(text = coordinates, style = MaterialTheme.typography.bodyLarge, color = Color.White)
            if (coordinates != "Bilinmiyor") {
                val mapContext = LocalContext.current
                TextButton(
                    onClick = {
                        val uri = android.net.Uri.parse("geo:0,0?q=${android.net.Uri.encode(coordinates)}")
                        try { mapContext.startActivity(android.content.Intent(android.content.Intent.ACTION_VIEW, uri)) } catch (_: Exception) {}
                    },
                    contentPadding = PaddingValues(0.dp)
                ) {
                    Text(
                        "\uD83D\uDDFA Haritada Göster",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.primary
                    )
                }
            }

            Spacer(modifier = Modifier.height(16.dp))
            HorizontalDivider(color = Color.White.copy(alpha = 0.12f), thickness = 1.dp)
            Spacer(modifier = Modifier.height(16.dp))

            Text(
                text = "SENSÖR TELEMETRİSİ",
                fontWeight = FontWeight.Bold,
                style = MaterialTheme.typography.labelMedium,
                color = MaterialTheme.colorScheme.error
            )
            Spacer(modifier = Modifier.height(10.dp))
            SensorDetailRow(label = "Alev (A0)", value = flame)
            Spacer(modifier = Modifier.height(8.dp))
            SensorDetailRow(label = "Duman MQ-2", value = mq2)
            Spacer(modifier = Modifier.height(8.dp))
            SensorDetailRow(label = "Karbon Monoksit MQ-7", value = mq7)
        }
    }
}
