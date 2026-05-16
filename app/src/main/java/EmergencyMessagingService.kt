package com.batuhantrkgl.blueberrytubitak

import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import androidx.core.app.NotificationCompat
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage

class EmergencyMessagingService : FirebaseMessagingService() {

    override fun onMessageReceived(message: RemoteMessage) {
        getSharedPreferences("BlueberryPrefs", Context.MODE_PRIVATE)
            .edit()
            .putLong("last_connection_timestamp", System.currentTimeMillis())
            .apply()

        if (message.data["emergency"] == "true") {
            val coords = message.data["coordinates"] ?: "İzmir / Blueberry-1 İstasyonu"
            val status = message.data["status"] ?: "Otonom Yangın İhbarı"
            val flame = message.data["sensorFlame"] ?: "Bilinmiyor"
            val mq2 = message.data["sensorMQ2"] ?: "Bilinmiyor"
            val mq7 = message.data["sensorMQ7"] ?: "Bilinmiyor"
            val severity = when (message.data["severity"]?.uppercase()) {
                "UYARI" -> "UYARI"
                else -> "KRITIK"
            }

            EmergencyAlertManager.addAlert(
                context = applicationContext,
                title = "MÜDAHALE GEREKLİ",
                body = "Orman yangını tespit edildi — $status",
                coordinates = coords,
                severity = severity
            )

            showEmergencyNotification(coords, status, flame, mq2, mq7)
        }
    }

    override fun onNewToken(token: String) {
        android.util.Log.d("FCM_TOKEN", "New token: $token")
    }

    private fun showEmergencyNotification(coordinates: String, status: String, flame: String, mq2: String, mq7: String) {
        val actionIntent = Intent(this, EmergencyActionActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            putExtra("coordinates", coordinates)
            putExtra("status", status)
            putExtra("flame", flame)
            putExtra("mq2", mq2)
            putExtra("mq7", mq7)
        }

        val pendingIntent = PendingIntent.getActivity(
            this, 0, actionIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val notification = NotificationCompat.Builder(this, EmergencyChannel.CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_notification)
            .setContentTitle("🚨 ACİL DURUM TESPİTİ")
            .setContentText("Kullanıcı müdahalesi bekleniyor…")
            .setPriority(NotificationCompat.PRIORITY_MAX)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .setFullScreenIntent(pendingIntent, true)
            .setAutoCancel(true)
            .build()

        val manager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        manager.notify(911, notification)

        try { startActivity(actionIntent) } catch (_: Exception) {}
    }
}
