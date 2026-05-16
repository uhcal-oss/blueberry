package com.batuhantrkgl.blueberrytubitak

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

object EmergencyAlertManager {
    private const val PREFS_NAME = "BlueberryAlerts"
    private const val KEY_ALERTS = "alerts_json"

    private val gson = Gson()
    private val _alerts = MutableStateFlow<List<AlertEntity>>(emptyList())
    val alerts: StateFlow<List<AlertEntity>> = _alerts.asStateFlow()

    private var nextId = 1

    fun initialize(context: Context) {
        val prefs = getPrefs(context)
        val loaded = loadAlerts(prefs)
        _alerts.value = loaded
        nextId = (loaded.maxOfOrNull { it.id } ?: 0) + 1
    }

    fun addAlert(
        context: Context,
        title: String,
        body: String,
        coordinates: String = "",
        severity: String = "KRITIK"
    ) {
        val alert = AlertEntity(
            id = nextId++,
            title = title,
            body = body,
            coordinates = coordinates,
            severity = severity
        )
        _alerts.value = listOf(alert) + _alerts.value
        saveAlerts(getPrefs(context), _alerts.value)
    }

    fun clearAlerts(context: Context) {
        _alerts.value = emptyList()
        saveAlerts(getPrefs(context), emptyList())
    }

    fun deleteAlert(context: Context, id: Int) {
        _alerts.value = _alerts.value.filter { it.id != id }
        saveAlerts(getPrefs(context), _alerts.value)
    }

    private fun getPrefs(context: Context): SharedPreferences {
        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }

    private fun saveAlerts(prefs: SharedPreferences, alerts: List<AlertEntity>) {
        val json = gson.toJson(alerts)
        prefs.edit().putString(KEY_ALERTS, json).apply()
    }

    private fun loadAlerts(prefs: SharedPreferences): List<AlertEntity> {
        val json = prefs.getString(KEY_ALERTS, null) ?: return emptyList()
        return try {
            val type = object : TypeToken<List<AlertEntity>>() {}.type
            gson.fromJson(json, type) ?: emptyList()
        } catch (_: Exception) {
            emptyList()
        }
    }
}
