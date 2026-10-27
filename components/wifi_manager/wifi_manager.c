#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>

#include "event_manager.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"

#ifndef CONFIG_SMARTCAP_WIFI_SSID
#define CONFIG_SMARTCAP_WIFI_SSID "SMARTCAP_TEST"
#endif

#ifndef CONFIG_SMARTCAP_WIFI_PASSWORD
#define CONFIG_SMARTCAP_WIFI_PASSWORD "smartcap123"
#endif

#ifndef CONFIG_SMARTCAP_WIFI_IP
#define CONFIG_SMARTCAP_WIFI_IP "192.168.4.1"
#endif

#define SMARTCAP_WIFI_MAX_CLIENTS 3

static const char *TAG = "wifi_manager";
static esp_netif_t *s_ap_netif;
static bool s_started;

esp_err_t wifi_manager_start(void)
{
    const size_t ssid_length = strlen(CONFIG_SMARTCAP_WIFI_SSID);
    const size_t password_length = strlen(CONFIG_SMARTCAP_WIFI_PASSWORD);
    if (ssid_length == 0 || ssid_length > 32 ||
        password_length < 8 || password_length > 63) {
        ESP_LOGE(TAG, "Configure an SSID (1-32 bytes) and WPA2 password (8-63 bytes)");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        return ESP_FAIL;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init_config);
    if (err != ESP_OK) {
        return err;
    }

    esp_netif_ip_info_t ip_info = {0};
    err = esp_netif_str_to_ip4(CONFIG_SMARTCAP_WIFI_IP, &ip_info.ip);
    if (err != ESP_OK) {
        return err;
    }
    ip_info.gw = ip_info.ip;
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    err = esp_netif_dhcps_stop(s_ap_netif);
    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE &&
        err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        return err;
    }
    err = esp_netif_set_ip_info(s_ap_netif, &ip_info);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_netif_dhcps_start(s_ap_netif);
    if (err != ESP_OK) {
        return err;
    }

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.ap.ssid, CONFIG_SMARTCAP_WIFI_SSID, ssid_length);
    wifi_config.ap.ssid_len = (uint8_t)ssid_length;
    memcpy(wifi_config.ap.password, CONFIG_SMARTCAP_WIFI_PASSWORD, password_length);
    wifi_config.ap.max_connection = SMARTCAP_WIFI_MAX_CLIENTS;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    s_started = true;
    event_manager_record(EVENT_WIFI_AP_STARTED);
    ESP_LOGI(TAG, "Access point started; IP=%s", CONFIG_SMARTCAP_WIFI_IP);
    return ESP_OK;
}

void wifi_manager_get_status(wifi_manager_status_t *status)
{
    if (status == NULL) {
        return;
    }

    memset(status, 0, sizeof(*status));
    status->active = s_started;
    snprintf(status->ssid, sizeof(status->ssid), "%s", CONFIG_SMARTCAP_WIFI_SSID);
    snprintf(status->ip_address, sizeof(status->ip_address), "%s", CONFIG_SMARTCAP_WIFI_IP);
    if (!s_started) {
        return;
    }

    wifi_sta_list_t station_list = {0};
    if (esp_wifi_ap_get_sta_list(&station_list) == ESP_OK) {
        status->connected_clients = station_list.num;
    }
}
