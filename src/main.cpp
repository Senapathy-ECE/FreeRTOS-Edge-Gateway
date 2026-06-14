/**
 * =============================================================
 * FILE: main.cpp
 * PROJECT: FreeRTOS-Edge-Gateway
 * MILESTONE: 3 — Queue-Based Inter-Task Communication
 * AUTHOR: senapathy
 * DATE: 14-06-2026
 *
 * PURPOSE:
 *   Demonstrate thread-safe data passing between two FreeRTOS
 *   tasks using a Queue. This eliminates race conditions that
 *   occur when tasks share global variables directly.
 *
 * ARCHITECTURE:
 *   taskSensor  (Priority 2) → generates data → xQueueSend()
 *   taskDisplay (Priority 1) → xQueueReceive() → prints data
 *
 * KEY CONCEPTS:
 *   - QueueHandle_t     : Handle to reference the queue
 *   - xQueueCreate()    : Create queue with size + item type
 *   - xQueueSend()      : Send data into queue (from producer)
 *   - xQueueReceive()   : Receive data from queue (consumer)
 *   - portMAX_DELAY     : Wait forever until data is available
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"    // ← NEW: Queue API


#define LED_PIN             2


#define SENSOR_PERIOD_MS    1000   // Sensor reads every 1 second
#define BLINK_PERIOD_MS     500    // LED blink every 500ms


#define QUEUE_LENGTH        5
#define QUEUE_ITEM_SIZE     sizeof(int)

// =============================================================
// GLOBAL QUEUE HANDLE
//   The QueueHandle_t  - is a REFERENCE to the
//   queue infrastructure managed by the FreeRTOS kernel.
//   Both tasks need this reference to find the same queue.
//   The actual data inside the queue is kernel-managed and
//   fully protected. This is safe. A raw shared int is not.
// =============================================================
QueueHandle_t xSensorQueue = NULL;

void taskSensor(void* pvParameters);
void taskDisplay(void* pvParameters);
void taskBlink(void* pvParameters);

// =============================================================
// setup() — Create Queue, Then Create Tasks - else error
// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("===========================================");
    Serial.println("  FreeRTOS-Edge-Gateway | Milestone 3");
    Serial.println("  Queue-Based Inter-Task Communication");
    Serial.println("===========================================");

    pinMode(LED_PIN, OUTPUT);

    // ==========================================================
    // CREATE THE QUEUE — Before any tasks
    //
    // xQueueCreate(length, itemSize)
    //   length   = max number of items queue can hold
    //   itemSize = size of each item in bytes
    //
    // Returns NULL if creation fails (not enough heap memory).
    // ALWAYS check for NULL — this is defensive programming,
    // mandatory in automotive-grade code (MISRA-C Rule 17.7).
    // ==========================================================
    
    xSensorQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    if (xSensorQueue == NULL) {
        Serial.println("[FATAL] Queue creation failed! Halting.");
        while(true); // Safe halt — never proceed without queue
    }
    Serial.println("[INIT] Sensor queue created | Length: 5 | Item: int");

    // Create taskSensor — HIGH priority (producer)
    xTaskCreate(taskSensor,  "TaskSensor",  2048, NULL, 2, NULL);
    Serial.println("[INIT] TaskSensor  created | Priority: 2");

    // Create taskDisplay — LOW priority (consumer)
    xTaskCreate(taskDisplay, "TaskDisplay", 2048, NULL, 1, NULL);
    Serial.println("[INIT] TaskDisplay created | Priority: 1");

    // Keep taskBlink running as before
    xTaskCreate(taskBlink,   "TaskBlink",   2048, NULL, 1, NULL);
    Serial.println("[INIT] TaskBlink   created | Priority: 1");

    Serial.println("[INIT] All tasks running. Scheduler active.");
    Serial.println("===========================================\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}

// =============================================================
// TASK 1: taskSensor — Data Producer
// Priority: 2 (High)
//
// SIMULATES: An I2C sensor (like BME280 temperature sensor)
//   In Milestone 4, this task will do real I2C reads.
//   For now, we generate fake data to prove the Queue works.
//
// xQueueSend(queue, &data, ticksToWait)
//   &data        = pointer to the value we want to send
//   portMAX_DELAY = wait forever if queue is full
//   Returns pdPASS if sent successfully, errQUEUE_FULL if not
// =============================================================
void taskSensor(void* pvParameters) {
    int sensorValue = 0;
    BaseType_t queueStatus;

    while(true) {
        // Simulate a sensor reading (0-100 range, cycling)
        sensorValue = (sensorValue + 5) % 101;

        // Send to queue — thread-safe, atomic operation
        queueStatus = xQueueSend(xSensorQueue, &sensorValue, portMAX_DELAY);

        if (queueStatus == pdPASS) {
            Serial.println("[SENSOR] Sent value: " + String(sensorValue)
                         + " | Queue spaces left: "
                         + String(uxQueueSpacesAvailable(xSensorQueue)));
        } else {
            Serial.println("[SENSOR] ERROR: Queue full! Data lost.");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

// =============================================================
// TASK 2: taskDisplay — Data Consumer
// Priority: 1 (Low)
//
// SIMULATES: A display output task (OLED in Milestone 4)
//
// xQueueReceive(queue, &buffer, ticksToWait)
//   &buffer      = where to store the received value
//   portMAX_DELAY = BLOCK here until data arrives
//
// KEY INSIGHT — portMAX_DELAY here is POWERFUL:
//   taskDisplay doesn't waste CPU polling "is data ready?"
//   It sleeps completely until taskSensor puts something
//   in the queue. The kernel wakes it up automatically.
//   This is called "blocking on a queue" — zero CPU waste.
// =============================================================
void taskDisplay(void* pvParameters) {
    int receivedValue = 0;

    while(true) {
        // Block here until sensor data arrives — uses zero CPU
        if (xQueueReceive(xSensorQueue, &receivedValue, portMAX_DELAY) == pdPASS) {

            Serial.println("-------------------------------------------");
            Serial.println("[DISPLAY] New data received!");
            Serial.println("[DISPLAY] Sensor Value : " + String(receivedValue));
            Serial.println("[DISPLAY] Free Heap    : "
                         + String(xPortGetFreeHeapSize()) + " bytes");
            Serial.println("-------------------------------------------\n");
        }
    }
}

// =============================================================
// TASK 3: taskBlink — Still running independently
// Priority: 1
// Proves that Queue communication doesn't affect other tasks
// =============================================================
void taskBlink(void* pvParameters) {
    while(true) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}