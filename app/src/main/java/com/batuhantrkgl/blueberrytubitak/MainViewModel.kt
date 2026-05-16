package com.batuhantrkgl.blueberrytubitak

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import kotlinx.coroutines.flow.StateFlow

class MainViewModel(application: Application) : AndroidViewModel(application) {

    init {
        EmergencyAlertManager.initialize(application)
    }

    val alerts: StateFlow<List<AlertEntity>> = EmergencyAlertManager.alerts

    fun clearAlerts() {
        EmergencyAlertManager.clearAlerts(getApplication())
    }

    fun deleteAlert(id: Int) {
        EmergencyAlertManager.deleteAlert(getApplication(), id)
    }
}
