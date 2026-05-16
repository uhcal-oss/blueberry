package com.batuhantrkgl.blueberrytubitak

import android.content.Context
import android.media.AudioManager
import android.speech.tts.TextToSpeech
import android.util.Log
import java.util.Locale

class EmergencyTTSManager(private val context: Context) : TextToSpeech.OnInitListener {
    private var tts: TextToSpeech? = null
    private var isInitialized = false
    private var ttsThread: Thread? = null
    @Volatile private var isCancelled = false

    init {
        tts = TextToSpeech(context, this)
    }

    override fun onInit(status: Int) {
        if (status == TextToSpeech.SUCCESS) {
            val result = tts?.setLanguage(Locale("tr", "TR"))
            if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
                tts?.setLanguage(Locale.ENGLISH)
                isInitialized = true
            } else {
                isInitialized = true
            }
        }
    }

    fun speakEmergencyMessage(coordinates: String, emergencyType: String) {
        if (!isInitialized) return

        isCancelled = false

        val message = "Dikkat. Bu otonom bir acil durum çağrısıdır. " +
                "Bir kullanıcının tehlikede olduğu tespit edildi. " +
                "Kullanıcı durumu: $emergencyType. " +
                "Koordinatlar: $coordinates. " +
                "Lütfen hemen yardıma gidin."

        val audioManager = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager

        ttsThread = Thread {
            try {
                // Wait for operator to answer the call
                Thread.sleep(8000)
                if (isCancelled) return@Thread

                // Enable speakerphone so the mic picks up TTS audio
                try {
                    audioManager.mode = AudioManager.MODE_IN_CALL
                    audioManager.isSpeakerphoneOn = true
                    audioManager.setStreamVolume(AudioManager.STREAM_ALARM, audioManager.getStreamMaxVolume(AudioManager.STREAM_ALARM), 0)
                    audioManager.setStreamVolume(AudioManager.STREAM_VOICE_CALL, audioManager.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL), 0)
                } catch (_: Exception) {}

                val params = android.os.Bundle()
                params.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM, AudioManager.STREAM_ALARM)
                params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, 1.0f)

                tts?.speak(message, TextToSpeech.QUEUE_FLUSH, params, "EMERGENCY_CALL_1")

                // Repeat after 15s for redundancy
                Thread.sleep(15000)
                if (isCancelled) return@Thread
                try { audioManager.isSpeakerphoneOn = true } catch (_: Exception) {}
                tts?.speak(message, TextToSpeech.QUEUE_ADD, params, "EMERGENCY_CALL_2")

            } catch (_: InterruptedException) {}
        }
        ttsThread?.start()
    }

    fun shutdown() {
        isCancelled = true
        ttsThread?.interrupt()
        tts?.stop()
        tts?.shutdown()
    }
}
