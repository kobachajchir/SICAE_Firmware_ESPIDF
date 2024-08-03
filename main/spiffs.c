#include "esp_log.h"
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "dirent.h"

static const char *TAG = "spiffs";
#define FILE_PATH_MAX (256)

void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS partition size: total: %d, used: %d", total, used);
    }

    // Open the directory and read its contents
    DIR *dir = opendir("/spiffs");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ensure the filepath does not exceed the buffer size
        if (strlen(entry->d_name) + strlen("/spiffs/") + 1 > FILE_PATH_MAX) {
            ESP_LOGE(TAG, "File path is too long for entry: %s", entry->d_name);
            continue;
        }

        // Construct the full path
        char filepath[FILE_PATH_MAX];
        strlcpy(filepath, "/spiffs/", sizeof(filepath));
        strlcat(filepath, entry->d_name, sizeof(filepath));

        // Open the file to get its size
        FILE *file = fopen(filepath, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            size_t file_size = ftell(file);
            fclose(file);

            ESP_LOGI(TAG, "File: %s, Size: %d bytes", filepath, file_size);
        } else {
            ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        }
    }

    closedir(dir);
}