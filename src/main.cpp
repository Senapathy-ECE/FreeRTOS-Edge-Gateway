/**
 * =============================================================
 * FILE: main.cpp
 * PROJECT: FreeRTOS-Edge-Gateway
 * MILESTONE: 4 — Hardware ISR Failsafe with FreeRTOS Semaphore
 * AUTHOR: Senapathy
 * DATE: 19-06-2026
 *
 * PURPOSE:
 *   Implement a hardware interrupt-driven failsafe system.
 *   A physical button press triggers an ISR which signals
 *   a FreeRTOS Binary Semaphore, instantly waking a dedicated
 *   failsafe task that suspends all normal operations.
 *
 *
 * KEY CONCEPTS:
 *   - attachInterrupt()           : Register ISR with hardware
 *   - IRAM_ATTR                   : Force ISR into fast RAM
 *   - xSemaphoreCreateBinary()    : Create binary semaphore
 *   - xSemaphoreGiveFromISR()     : Signal from ISR (non-blocking)
 *   - xSemaphoreTake()            : Wait for signal in task
 *   - vTaskSuspend()              : Freeze a task instantly
 * =============================================================
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"       // ← NEW: Semaphore API


#define LED_PIN             2      // Onboard LED
#define BUTTON_PIN          4      // Emergency stop button
#define FAILSAFE_LED_PIN    2      // Same LED, will blink SOS


// TIMING

#define SENSOR_PERIOD_MS    1000
#define BLINK_PERIOD_MS     500
#define FAILSAFE_BLINK_MS   200    // Fast blink = emergency state


// QUEUE CONFIGURATION 

#define QUEUE_LENGTH        5
#define QUEUE_ITEM_SIZE     sizeof(int)


// GLOBAL HANDLES

QueueHandle_t  xSensorQueue    = NULL;
SemaphoreHandle_t xEmergencySem = NULL;  // Binary semaphore

TaskHandle_t   xSensorHandle   = NULL;   // For vTaskSuspend()
TaskHandle_t   xDisplayHandle  = NULL;   // For vTaskSuspend()
TaskHandle_t   xBlinkHandle    = NULL;   // For vTaskSuspend()


// FORWARD DECLARATIONS

void taskSensor(void* pvParameters);
void taskDisplay(void* pvParameters);
void taskBlink(void* pvParameters);
void taskFailsafe(void* pvParameters);
void IRAM_ATTR isrEmergencyButton();    // ISR declaration

// =============================================================
// setup()
// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("===========================================");
    Serial.println("  FreeRTOS-Edge-Gateway | Milestone 4");
    Serial.println("  Hardware ISR Failsafe System");
    Serial.println("===========================================");

    pinMode(LED_PIN, OUTPUT);

    // ==========================================================
    // BUTTON PIN CONFIGURATION
    // INPUT_PULLUP activates the ESP32's internal pull-up
    // ==========================================================
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // ==========================================================
    // ATTACH HARDWARE INTERRUPT
    //
    // attachInterrupt(pin, ISR_function, trigger_mode)
    //
    // FALLING means: trigger when signal goes HIGH → LOW
    // ==========================================================
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                    isrEmergencyButton,
                    FALLING);
    Serial.println("[INIT] Emergency ISR attached | GPIO4 | FALLING edge");

    // Create semaphore BEFORE tasks (same rule as queue)
    xEmergencySem = xSemaphoreCreateBinary();
    if (xEmergencySem == NULL) {
        Serial.println("[FATAL] Semaphore creation failed! Halting.");
        while(true);
    }
    Serial.println("[INIT] Emergency semaphore created");

    // Create queue
    xSensorQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    if (xSensorQueue == NULL) {
        Serial.println("[FATAL] Queue creation failed! Halting.");
        while(true);
    }
    Serial.println("[INIT] Sensor queue created");

    // ==========================================================
    // CREATE TASKS 
    // taskFailsafe gets HIGHEST priority (3)
    // It must preempt everything when emergency occurs
    // ==========================================================
    xTaskCreate(taskFailsafe, "TaskFailsafe", 2048, NULL, 3, NULL);
    Serial.println("[INIT] TaskFailsafe created | Priority: 3 (HIGHEST)");

    xTaskCreate(taskSensor,  "TaskSensor",  2048, NULL, 2, &xSensorHandle);
    Serial.println("[INIT] TaskSensor  created | Priority: 2");

    xTaskCreate(taskDisplay, "TaskDisplay", 2048, NULL, 1, &xDisplayHandle);
    Serial.println("[INIT] TaskDisplay created | Priority: 1");

    xTaskCreate(taskBlink,   "TaskBlink",   2048, NULL, 1, &xBlinkHandle);
    Serial.println("[INIT] TaskBlink   created | Priority: 1");

    Serial.println("[INIT] System NOMINAL. Press button for emergency.");
    Serial.println("===========================================\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}

// =============================================================
// ISR: isrEmergencyButton
//
// IRAM_ATTR — CRITICAL KEYWORD:
//   Normal code lives in Flash memory (slow).
//   IRAM_ATTR forces this function into IRAM (Internal RAM)
//   which is directly accessible by the CPU — much faster.
//   ISRs MUST be in IRAM on ESP32, or the system will crash
//   if the Flash cache is busy when the interrupt fires.
//
// BaseType_t xHigherPriorityTaskWoken:
//   FreeRTOS uses this to know if giving the semaphore
//   woke up a higher priority task. If yes, we must
//   explicitly yield so that task runs immediately.
//   portYIELD_FROM_ISR() handles this context switch.
// =============================================================
void IRAM_ATTR isrEmergencyButton() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Signal the semaphore — wakes taskFailsafe
    xSemaphoreGiveFromISR(xEmergencySem, &xHigherPriorityTaskWoken);

    // If taskFailsafe has higher priority than current task,
    // yield immediately so it runs without scheduler delay
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// =============================================================
// TASK: taskFailsafe
// Priority: 3 (HIGHEST)
// =============================================================
void taskFailsafe(void* pvParameters) {

    while(true) {
        
        if (xSemaphoreTake(xEmergencySem, portMAX_DELAY) == pdTRUE) {

            Serial.println("\n[!!!] EMERGENCY TRIGGERED [!!!]");
            Serial.println("[FAILSAFE] ISR detected! Suspending all tasks.");

            // Suspend all normal operation tasks instantly
            vTaskSuspend(xSensorHandle);
            vTaskSuspend(xDisplayHandle);
            vTaskSuspend(xBlinkHandle);

            Serial.println("[FAILSAFE] TaskSensor  → SUSPENDED");
            Serial.println("[FAILSAFE] TaskDisplay → SUSPENDED");
            Serial.println("[FAILSAFE] TaskBlink   → SUSPENDED");
            Serial.println("[FAILSAFE] System in SAFE STATE");
            Serial.println("[FAILSAFE] Fast blink = emergency indicator\n");

            // Enter emergency state — fast LED blink forever
            
            while(true) {
                digitalWrite(LED_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(FAILSAFE_BLINK_MS));
                digitalWrite(LED_PIN, LOW);
                vTaskDelay(pdMS_TO_TICKS(FAILSAFE_BLINK_MS));
            }
        }
    }
}

// =============================================================
// TASK: taskSensor (same as Milestone 3)
// =============================================================
void taskSensor(void* pvParameters) {
    int sensorValue = 0;

    while(true) {
        sensorValue = (sensorValue + 5) % 101;

        xQueueSend(xSensorQueue, &sensorValue, portMAX_DELAY);
        Serial.println("[SENSOR] Sent value: " + String(sensorValue));

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

// =============================================================
// TASK: taskDisplay (same as Milestone 3)
// =============================================================
void taskDisplay(void* pvParameters) {
    int receivedValue = 0;

    while(true) {
        if (xQueueReceive(xSensorQueue, &receivedValue, portMAX_DELAY) == pdPASS) {
            Serial.println("[DISPLAY] Sensor Value: " + String(receivedValue));
        }
    }
}

// =============================================================
// TASK: taskBlink (same as Milestone 3)
// =============================================================
void taskBlink(void* pvParameters) {
    while(true) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}