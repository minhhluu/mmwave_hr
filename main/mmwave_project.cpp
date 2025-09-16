//file: main.cpp

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "Arduino.h"
#include "Seeed_Arduino_mmWave.h"
#include "led_strip.h"

static const char *TAG = "mmWave_feature";
static const char *TAG_1 = "LED";

// GPIO assignment
#define LED_STRIP_GPIO_PIN  1
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 24

// ---------------------------- LED -------------------------------

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        // set the color order of the strip: GRB
        .color_component_format = {
            .format = {
                .r_pos = 1, // red is the second byte in the color data
                .g_pos = 0, // green is the first byte in the color data
                .b_pos = 2, // blue is the third byte in the color data
                .num_components = 3, // total 3 color components
            },
        },
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: SPI
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .spi_bus = SPI2_HOST,           // SPI bus ID
        .flags = {
            .with_dma = true, // Using DMA can improve performance and help drive more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG_1, "Created LED strip object with SPI backend");
    return led_strip;
}

// ---------------------------- LED -------------------------------


// If the board is an ESP32, include the HardwareSerial library and create a
// HardwareSerial object for the mmWave serial communication
#ifdef ESP32
  #include <HardwareSerial.h>
  HardwareSerial mmWaveSerial(0);
#else
  // Otherwise, define mmWaveSerial as Serial1
  #define mmWaveSerial Serial1
#endif

// Declare queue handle globally (OK — just a variable)
QueueHandle_t uart_queue;

// ---------------------------- 
// Moving Average Filter declaration
#define HR_BUFFER_SIZE 10
float hrBuffer[HR_BUFFER_SIZE] = {0};
int hrIndex = 0;

float getFilteredHeartRate(float rawHR) {
  if (rawHR < 40 || rawHR > 120) return -1;

  hrBuffer[hrIndex] = rawHR;
  hrIndex = (hrIndex + 1) % HR_BUFFER_SIZE;

  float sum = 0;
  int count = 0;
  for (int i = 0; i < HR_BUFFER_SIZE; i++) {
    if (hrBuffer[i] > 0) {
      sum += hrBuffer[i];
      count++;
    }
  }
  return count > 0 ? sum / count : -1;
}

float lastPhase = 0;
float phaseThreshold = 0.8; // threshold changing phase

bool isSignalValid(float currentPhase) {
  if (lastPhase == 0) {
    lastPhase = currentPhase;
    return true;
  }

  float delta = fabs(currentPhase - lastPhase);
  lastPhase = currentPhase;

  return delta < phaseThreshold; // If it's getting over threshold → noise. 
}

// ---------------------------- 

SEEED_MR60BHA2 mmWave;

extern "C" void app_main(void)
{
    // Initialize Arduino core FIRST (if using Serial, delay, etc.)
    initArduino();

    // Initialize ESP-IDF UARTs INSIDE app_main()
    const int uart_buffer_size = (1024 * 2);

    // Install UART1 for mmWave (or whatever you're using it for)
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Starting app...");

    printf("Arduino setup done!\n");

    // Initialize the mmWave sensor
    mmWave.begin(&mmWaveSerial);
    ESP_LOGI(TAG, "mmWave sensor initialized");
    
    // Initialize Serial for logging
    Serial.begin(115200);

    led_strip_handle_t led_strip = configure_led();
    bool led_on_off = false;

    ESP_LOGI(TAG_1, "Start blinking LED strip");

    // Main loop
    while (1) {

        // LED function
        if (led_on_off) {
            /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 5, 5, 5));
            }
            /* Refresh the strip to send data */
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            // ESP_LOGI(TAG_1, "LED ON!");
        } else {
            /* Set all LED off to clear all pixels */
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            //ESP_LOGI(TAG_1, "LED OFF!");
        }
            led_on_off = !led_on_off;
            vTaskDelay(pdMS_TO_TICKS(500));
        

        // mmWave function
        if (mmWave.update(100)) {
            float total_phase, breath_phase, heart_phase;
            if (mmWave.isHumanDetected()) {
                // ESP_LOGI(TAG, "----- Human Detected-----");
            }

            PeopleCounting target_info;
            if (mmWave.getPeopleCountingTargetInfo(target_info)) {
                // ESP_LOGI(TAG, "-----Got Target Info-----");
                // ESP_LOGI(TAG, "Number of targets: %zu", target_info.targets.size());

                // heart_rate sensor
                if (mmWave.getHeartBreathPhases(total_phase, breath_phase, heart_phase)) {
                    if (isSignalValid(heart_phase)) {
                        float heart_rate;
                        if (mmWave.getHeartRate(heart_rate)) {
                            float filteredHR = getFilteredHeartRate(heart_rate);

                            // Print LED status instead of controlling NeoPixel
                            // ESP_LOGI(TAG, "Heart rate detected - GREEN LED would light up");
                            
                            if (filteredHR > 0) {
                                // Serial.printf("HR_Filtered: %.2f\n", filteredHR);
                                ESP_LOGI(TAG, "HR_Filtered: %.2f", filteredHR);
                            }
                        }

                        // Serial.printf("heart_phase_____: %.2f\n", heart_phase);
                        // printf("heart_phase_____: %.2f\n", heart_phase);
                    }
                }

                for (size_t i = 0; i < target_info.targets.size(); i++) {
                    const auto& target = target_info.targets[i];
                    // Serial.printf("Total Target: %zu\n", i + 1);
                    // Serial.printf("  move_speed: %.2f cm/s\n", target.dop_index * RANGE_STEP);
                    // ("Total Target: %zu, move_speed: %.2f cm/s\n", i + 1, target.dop_index * 0.5);
                }
            } else {
                // Print LED status instead of fading NeoPixel
                // Fade the NeoPixel from red to black
                for (int i = 255; i >= 0; i--) {
                    for (int j = 0; j < LED_STRIP_LED_COUNT; j++) {
                        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, i, 0, 0));
                    }
                    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                printf("----- no one detected ----\n");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}