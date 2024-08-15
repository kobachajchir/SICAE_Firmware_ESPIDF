
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <globals.h>
#include <IRremote.cpp>

static const char *TAG = "IR_utils";

void initIR() {
    ESP_LOGI(TAG, "Enabling IRin...");
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    ESP_LOGI(TAG, "Ready to receive IR signals of protocols: ");
    logsActiveIRProtocols();
    ESP_LOGI(TAG, "at pin %d", IR_RECEIVE_PIN);
}

void logsActiveIRProtocols() {
#if defined(DECODE_ONKYO)
    ESP_LOGI(TAG, "Onkyo, ");
#elif defined(DECODE_NEC)
    ESP_LOGI(TAG, "NEC/NEC2/Onkyo/Apple, ");
#endif
#if defined(DECODE_PANASONIC) || defined(DECODE_KASEIKYO)
    ESP_LOGI(TAG, "Panasonic/Kaseikyo, ");
#endif
#if defined(DECODE_DENON)
    ESP_LOGI(TAG, "Denon/Sharp, ");
#endif
#if defined(DECODE_SONY)
    ESP_LOGI(TAG, "Sony, ");
#endif
#if defined(DECODE_RC5)
    ESP_LOGI(TAG, "RC5, ");
#endif
#if defined(DECODE_RC6)
    ESP_LOGI(TAG, "RC6, ");
#endif
#if defined(DECODE_LG)
    ESP_LOGI(TAG, "LG, ");
#endif
#if defined(DECODE_JVC)
    ESP_LOGI(TAG, "JVC, ");
#endif
#if defined(DECODE_SAMSUNG)
    ESP_LOGI(TAG, "Samsung, ");
#endif
    /*
     * Start of the exotic protocols
     */
#if defined(DECODE_BEO)
    ESP_LOGI(TAG, "Bang & Olufsen, ");
#endif
#if defined(DECODE_FAST)
    ESP_LOGI(TAG, "FAST, ");
#endif
#if defined(DECODE_WHYNTER)
    ESP_LOGI(TAG, "Whynter, ");
#endif
#if defined(DECODE_LEGO_PF)
    ESP_LOGI(TAG, "Lego Power Functions, ");
#endif
#if defined(DECODE_BOSEWAVE)
    ESP_LOGI(TAG, "Bosewave, ");
#endif
#if defined(DECODE_MAGIQUEST)
    ESP_LOGI(TAG, "MagiQuest, ");
#endif
#if defined(DECODE_DISTANCE_WIDTH)
    ESP_LOGI(TAG, "Universal Pulse Distance Width, ");
#endif
#if defined(DECODE_HASH)
    ESP_LOGI(TAG, "Hash ");
#endif
#if defined(NO_DECODER) // for sending raw only
    (void)aSerial; // to avoid compiler warnings
#endif
}

// Variables globales para las tareas
static TaskHandle_t irReceiveTaskHandle = NULL;
static TaskHandle_t longPressTaskHandle = NULL;

// Prototipos de funciones
void initIRTask(void *pvParameters);
void irReceiveTask(void *pvParameters);
void handleOverflow();
bool detectLongPress(uint16_t aLongPressDurationMillis);

void irReceiveTask(void *pvParameters) {
    while (1) {
        if (IrReceiver.decode()) {
            ESP_LOGI(TAG, "IR signal decoded");

            if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
                handleOverflow();
            } else {
                if ((IrReceiver.decodedIRData.protocol != SONY) && (IrReceiver.decodedIRData.protocol != PULSE_WIDTH)
                    && (IrReceiver.decodedIRData.protocol != PULSE_DISTANCE) && (IrReceiver.decodedIRData.protocol != UNKNOWN)) {
                    // Generar tono si es necesario
                }

                if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
                    ESP_LOGI(TAG, "Received unknown IR protocol");
                } else {
                    ESP_LOGI(TAG, "IR command received: 0x%x", IrReceiver.decodedIRData.command);
                }
            }
            IrReceiver.resume();  // Preparar el receptor para el siguiente paquete de datos

            // Aquí podrías agregar la lógica para manejar comandos específicos
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Ajusta el delay según sea necesario
    }
}

void longPressTask(void *pvParameters) {
    uint16_t longPressDurationMillis = 1000;  // Duración para considerar una pulsación larga
    while (1) {
        if (detectLongPress(longPressDurationMillis)) {
            ESP_LOGI(TAG, "Long press detected: Command 0x%x", IrReceiver.decodedIRData.command);
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Ajusta el delay según sea necesario
    }
}

void handleOverflow() {
    ESP_LOGI(TAG, "Overflow detected. Try increasing RAW_BUFFER_LENGTH.");
    // Manejar desbordamiento
}

bool detectLongPress(uint16_t aLongPressDurationMillis) {
    static unsigned long sMillisOfFirstReceive;
    static bool sLongPressJustDetected = false;

    if (!sLongPressJustDetected && (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
        if (millis() - aLongPressDurationMillis > sMillisOfFirstReceive) {
            sLongPressJustDetected = true;
        }
    } else {
        sMillisOfFirstReceive = millis();
        sLongPressJustDetected = false;
    }
    return sLongPressJustDetected;
}
