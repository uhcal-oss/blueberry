package com.batuhantrkgl.blueberrytubitak

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager

class EmergencyChannel : Application() {

    companion object {
        const val CHANNEL_ID = "emergency_channel"
    }

    override fun onCreate() {
        super.onCreate()

        val channel = NotificationChannel(
            CHANNEL_ID,
            "Acil Durum",
            NotificationManager.IMPORTANCE_HIGH
        ).apply {
            description = "Arduino acil durum bildirimleri"
            enableVibration(true)
        }

        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager)
            .createNotificationChannel(channel)
    }
}
