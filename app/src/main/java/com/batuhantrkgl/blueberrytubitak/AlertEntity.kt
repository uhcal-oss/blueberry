package com.batuhantrkgl.blueberrytubitak

data class AlertEntity(
    val id: Int = 0,
    val title: String,
    val body: String,
    val coordinates: String = "",
    val severity: String = "KRITIK",
    val timestamp: Long = System.currentTimeMillis()
)
