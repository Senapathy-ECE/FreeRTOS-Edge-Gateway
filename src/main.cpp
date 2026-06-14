/**
 * =============================================================
 * FILE: main.cpp
 * PROJECT: FreeRTOS-Edge-Gateway
 * MILESTONE: 2 — FreeRTOS Scheduler, Dual Tasks, Priorities
 * AUTHOR: senapathy
 * DATE: 14-06-2026
 *
 * PURPOSE:
 *   Demonstrate the FreeRTOS scheduler running two independent
 *   tasks with different priorities and timing
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"    
#include "freertos/task.h"         


#define LED_PIN           2        
#define BLINK_PERIOD_MS    500     
#define MONITOR_PERIOD_MS  2000   // Heartbeat print interval

void taskBlink(void* pvParameters);
void taskMonitor(void* pvParameters);


void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("===========================================");
    Serial.println("  FreeRTOS-Edge-Gateway | Milestone 2");
    Serial.println("  Initializing FreeRTOS Scheduler...");
    Serial.println("===========================================");

    pinMode(LED_PIN, OUTPUT);

    
    // Create Task 1 — LED Blink (Low Priority)
    xTaskCreate(
        taskBlink,           
        "TaskBlink",        
        2048,               
        NULL,                
        1,                   
        NULL                 
    );
    Serial.println("[INIT] TaskBlink created  | Priority: 1 | Stack: 2048");

    // Create Task 2 — System Monitor (High Priority)
    xTaskCreate(
        taskMonitor,         
        "TaskMonitor",       
        2048,                
        NULL,                
        2,                   
        NULL                 
    );
    Serial.println("[INIT] TaskMonitor created | Priority: 2 | Stack: 2048");

    Serial.println("[INIT] Scheduler now running. loop() is idle.");
    Serial.println("===========================================\n");
}


void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000)); // Idle 
}


void taskBlink(void* pvParameters) {


    while (true) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("[BLINK] LED → ON");
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));

        digitalWrite(LED_PIN, LOW);
        Serial.println("[BLINK] LED → OFF");
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
    
}


void taskMonitor(void* pvParameters) {

    uint32_t uptimeSeconds = 0;
    while (true) {
        uptimeSeconds += (MONITOR_PERIOD_MS / 1000);

        Serial.println("-------------------------------------------");
        Serial.println("[MONITOR] System Heartbeat");
        Serial.println("[MONITOR] Uptime     : " + String(uptimeSeconds) + "s");
        Serial.println("[MONITOR] Free Heap  : " + String(xPortGetFreeHeapSize()) + " bytes");
        Serial.println("[MONITOR] Tick Count : " + String(xTaskGetTickCount()));
        Serial.println("-------------------------------------------\n");

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}