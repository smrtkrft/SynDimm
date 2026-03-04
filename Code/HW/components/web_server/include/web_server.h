#pragma once

#include "esp_err.h"

/**
 * Web server baslatma
 * Dahili flash uzerinden ayar sayfalari sunar
 * Harici flash uzerinden statik dosyalari (HTML/CSS/JS) sunar
 */
esp_err_t web_server_start(void);

/**
 * Web server durdurma
 */
esp_err_t web_server_stop(void);

/**
 * SSE event gonder - tum bagli istemcilere bildir
 */
void web_server_send_sse_event(void);
