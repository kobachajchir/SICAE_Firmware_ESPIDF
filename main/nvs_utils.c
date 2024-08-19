#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "globals.h"

#define TAG "nvs"

void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void read_firmware() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    size_t firmwareVersion_len = sizeof(firmwareVersion);
    err = nvs_get_str(my_handle, "firmware", firmwareVersion, &firmwareVersion_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading firmware version!", esp_err_to_name(err));
        // Optionally set a default value if read fails
        strcpy(firmwareVersion, "0.0.0");
    } else {
        ESP_LOGI(TAG, "Firmware read: %s", firmwareVersion);
    }

    nvs_close(my_handle);
}

void save_firmware() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(my_handle, "firmware", FIRMWARE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) saving firmware version!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Firmware saved: %s", FIRMWARE);
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
    }

    nvs_close(my_handle);
}

void read_server_credentials(char* disp_url, size_t disp_url_len, char* events_url, size_t events_url_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    err = nvs_get_str(my_handle, "disp_url", disp_url, &disp_url_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading device URL!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "disp_url: %s", disp_url);
    }

    err = nvs_get_str(my_handle, "events_url", events_url, &events_url_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading events URL!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "events_url: %s", events_url);
    }

    nvs_close(my_handle);
}

void save_server_credentials(const char* device_id) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos/%s/data/", device_id);
        err = nvs_set_str(my_handle, "disp_url", url);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving device URL!", esp_err_to_name(err));
        }
        snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/events/%s/", device_id);
        err = nvs_set_str(my_handle, "events_url", url);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving events URL!", esp_err_to_name(err));
        }
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }
}

void save_wifi_credentials(const char* ssid, const char* password, const char* url) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        err = nvs_set_str(my_handle, "wifi_ssid", ssid);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving SSID!", esp_err_to_name(err));
        }

        err = nvs_set_str(my_handle, "wifi_password", password);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving Password!", esp_err_to_name(err));
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

void read_wifi_credentials(char* ssid, size_t ssid_len, char* password, size_t password_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        err = nvs_get_str(my_handle, "wifi_ssid", ssid, &ssid_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) reading SSID!", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "SSID: %s", ssid);
        }

        err = nvs_get_str(my_handle, "wifi_password", password, &password_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) reading Password!", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Password: %s", password);
        }

        nvs_close(my_handle);
    }
}

void save_ap_credentials(const char* ssid, const char* password) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        err = nvs_set_str(my_handle, "ap_ssid", ssid);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving AP SSID!", esp_err_to_name(err));
        }

        err = nvs_set_str(my_handle, "ap_password", password);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) saving AP Password!", esp_err_to_name(err));
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

void read_ap_credentials(char* ssid, size_t ssid_len, char* password, size_t password_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        err = nvs_get_str(my_handle, "ap_ssid", ssid, &ssid_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) reading AP SSID!", esp_err_to_name(err));
        }

        err = nvs_get_str(my_handle, "ap_password", password, &password_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) reading AP Password!", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

esp_err_t write_device_pin_to_nvs(int device_number, uint8_t gpio_pin) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    // Construct the key as "device-X" where X is the device number
    char key[16];
    snprintf(key, sizeof(key), "device-%d", device_number);

    // Write the value (gpio_pin) to NVS
    err = nvs_set_u8(my_handle, key, gpio_pin);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to set value in NVS (%s)", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    // Commit the written value
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to commit NVS (%s)", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
    return err;
}

esp_err_t read_device_pin_from_nvs(int device_number, uint8_t *gpio_pin) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    // Construct the key as "device-X" where X is the device number
    char key[16];
    snprintf(key, sizeof(key), "device-%d", device_number);

    // Read the value from NVS
    err = nvs_get_u8(my_handle, key, gpio_pin);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to get value from NVS (%s)", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
    return err;
}

void initialize_nvs_with_default_data(char* device_id) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        // Verifica si la bandera "initialized" ya está configurada
        uint8_t initialized = 0;
        size_t size = sizeof(initialized);
        err = nvs_get_u8(my_handle, "initialized", &initialized);
		if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error initialized not found");
		}
        if (err == ESP_ERR_NVS_NOT_FOUND || initialized == 0) {
        	ESP_LOGI(TAG, "Initializing NVS Data");
            // Guarda los datos por defecto si no están inicializados
            err = nvs_set_str(my_handle, "firmware", FIRMWARE);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) saving default SSID!", esp_err_to_name(err));
            }

            err = nvs_set_str(my_handle, "wifi_ssid", "Koba");
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) saving default SSID!", esp_err_to_name(err));
            }

            err = nvs_set_str(my_handle, "wifi_password", "koba1254");
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) saving default Password!", esp_err_to_name(err));
            }

			// Crear y guardar la URL del dispositivo
			snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos/%s/data/", device_id);
			err = nvs_set_str(my_handle, "disp_url", url);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Error (%s) saving device URL!", esp_err_to_name(err));
			} else {
				ESP_LOGI(TAG, "Saved disp_url: %s", url);
			}

			// Crear y guardar la URL de eventos
			snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/events/%s/", device_id);
			err = nvs_set_str(my_handle, "events_url", url);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Error (%s) saving events URL!", esp_err_to_name(err));
			} else {
				ESP_LOGI(TAG, "Saved events_url: %s", url);
			}
			char ap_ssid[64];
			snprintf(ap_ssid, sizeof(ap_ssid), "AP-%s", device_id);
			err = nvs_set_str(my_handle, "ap_ssid", ap_ssid);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) saving default AP SSID!", esp_err_to_name(err));
            }
			char ap_password[64];
			snprintf(ap_password, sizeof(ap_password), "%spassword", device_id);
			err = nvs_set_str(my_handle, "ap_password", ap_password);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) saving default AP Password!", esp_err_to_name(err));
            }

            char key[16];
            snprintf(key, sizeof(key), "device-0");
            err = nvs_set_u8(my_handle, key, 23);
            if (err != ESP_OK) {
                ESP_LOGE("NVS", "Failed to set value in NVS (%s)", esp_err_to_name(err));
            }

            // Establece la bandera "initialized"
            initialized = 1;
            err = nvs_set_u8(my_handle, "initialized", initialized);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) setting initialized flag!", esp_err_to_name(err));
            }

            // Commit los cambios
            err = nvs_commit(my_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
            }
        }

        nvs_close(my_handle);
    }
}
