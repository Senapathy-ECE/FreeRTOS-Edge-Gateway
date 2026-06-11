/**
 * =============================================================
 * FILE: main.cpp
 * PROJECT: FreeRTOS-Edge-Gateway
 * MILESTONE: 1 — Hardware Handshake (Single-Task Blink)
 * AUTHOR: senapathy
 * 
 *
 * PURPOSE:
 *   Verify toolchain, board connection, and serial communication.
 *
 *
 * HARDWARE:
 *   - ESP32 DevKit v1
 *   - Onboard LED (GPIO 2 on most ESP32 DevKit boards)
 * =============================================================
 */


#include <Arduino.h>
#define LED_PIN         2       
#define BLINK_PERIOD_MS 1000    

void taskBlink(void* pvParameters);

void setup() {
    
    Serial.begin(115200);

    
    delay(500);

    Serial.println("===========================================");
    Serial.println("  FreeRTOS-Edge-Gateway | Milestone 1");
    Serial.println("  System Initializing...");
    Serial.println("===========================================");

    
    pinMode(LED_PIN, OUTPUT);

    Serial.println("[INIT] GPIO configured. Entering task loop.");
    Serial.println("[INFO] LED_PIN = GPIO " + String(LED_PIN));
    Serial.println("[INFO] Blink period = " + String(BLINK_PERIOD_MS) + "ms");
    Serial.println("===========================================\n");
}


void loop() {
    taskBlink(NULL); 
}


void taskBlink(void* pvParameters) {
    // Turn LED ON
    digitalWrite(LED_PIN, HIGH);
    Serial.println("[TASK_BLINK] LED → ON");

    delay(BLINK_PERIOD_MS / 2);  // 500ms

    // Turn LED OFF
    digitalWrite(LED_PIN, LOW);
    Serial.println("[TASK_BLINK] LED → OFF");

    delay(BLINK_PERIOD_MS / 2);  // 500ms
}