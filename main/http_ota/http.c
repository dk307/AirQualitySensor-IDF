#include <errno.h>
#include <esp_flash_partitions.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_tls_crypto.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct
{
    const char *username;
    const char *password;
} basic_auth_info_t;

#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

static basic_auth_info_t auth_info = {
    .username = "deepak",
    .password = "deepak",
};

static char *_http_auth_basic(const char *username, const char *password);

static esp_err_t _root_get_handler(httpd_req_t *req);
static esp_err_t _root_post_handler(httpd_req_t *req);
static esp_err_t _ota_get_handler(httpd_req_t *req);
static esp_err_t _ota_post_handler(httpd_req_t *req);
static esp_err_t _reset_get_handler(httpd_req_t *req);
static esp_err_t _reset_post_handler(httpd_req_t *req);

//-----------------------------------------------------------------------------
static char *_http_auth_basic(const char *username, const char *password)
{
    int out;
    char user_info[128];
    static char digest[512];
    size_t n = 0;
    sprintf(user_info, "%s:%s", username, password);

    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    // 6: The length of the "Basic " string
    // n: Number of bytes for a base64 encode format
    // 1: Number of bytes for a reserved which be used to fill zero
    if (sizeof(digest) > (6 + n + 1))
    {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }

    return digest;
}

char const *application_get_html(const char *p_custom_header)
{
    static char buffer[2048] = {0};
    char *p_buffer = buffer;

    if (p_custom_header)
    {
        p_buffer += sprintf(p_buffer, "%s", p_custom_header);
    }

    //   p_buffer += sprintf(p_buffer, "<h1>System Info</h1>");
    //   p_buffer += sprintf(p_buffer, "System Time: %s<br>", get_system_time_str());
    //   p_buffer += sprintf(p_buffer, "Firmware Build: %s %s, Boot Count: %i<br>", __DATE__, __TIME__, nvm_get_param_int32( NVM_PARAM_RESET_COUNTER
    //   )); p_buffer += sprintf(p_buffer, "Up-time: "); p_buffer += add_formatted_duration_str( p_buffer, system_uptime_s() ); p_buffer +=
    //   sprintf(p_buffer, "<br>");

    return buffer;
}

//-----------------------------------------------------------------------------
char const *application_post_html(const char *p_post_data)
{
    return application_get_html(NULL);
}

//-----------------------------------------------------------------------------
static esp_err_t _root_get_handler(httpd_req_t *req)
{
    const char *p_html_resp = application_get_html(NULL);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_send(req, p_html_resp, strlen(p_html_resp));
    return ESP_OK;
}

//-----------------------------------------------------------------------------
static esp_err_t _root_post_handler(httpd_req_t *req)
{
    char post_data[256] = {0};

    // Read the data for the request
    httpd_req_recv(req, post_data, MIN(sizeof(post_data) - 1, req->content_len));
    // printf( "Received: %s\n", post_data);

    const char *p_html_resp = application_post_html(post_data);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_send(req, p_html_resp, strlen(p_html_resp));
    return ESP_OK;
}

//-----------------------------------------------------------------------------
static char auth_buffer[512];
static const char ota_html_file[] = "\
<style>\n\
.progress {margin: 15px auto;  max-width: 500px;height: 30px;}\n\
.progress .progress__bar {\n\
  height: 100%; width: 1%; border-radius: 15px;\n\
  background: repeating-linear-gradient(135deg,#336ffc,#036ffc 15px,#1163cf 15px,#1163cf 30px); }\n\
 .status {font-weight: bold; font-size: 30px;};\n\
</style>\n\
<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/2.2.1/css/bootstrap.min.css\">\n\
<div class=\"well\" style=\"text-align: center;\">\n\
  <div class=\"btn\" onclick=\"file_sel.click();\"><i class=\"icon-upload\" style=\"padding-right: 5px;\"></i>Upload Firmware</div>\n\
  <div class=\"progress\"><div class=\"progress__bar\" id=\"progress\"></div></div>\n\
  <div class=\"status\" id=\"status_div\"></div>\n\
</div>\n\
<input type=\"file\" id=\"file_sel\" onchange=\"upload_file()\" style=\"display: none;\">\n\
<script>\n\
function upload_file() {\n\
  document.getElementById(\"status_div\").innerHTML = \"Upload in progress\";\n\
  let data = document.getElementById(\"file_sel\").files[0];\n\
  xhr = new XMLHttpRequest();\n\
  xhr.open(\"POST\", \"/ota\", true);\n\
  xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');\n\
  xhr.upload.addEventListener(\"progress\", function (event) {\n\
     if (event.lengthComputable) {\n\
    	 document.getElementById(\"progress\").style.width = (event.loaded / event.total) * 100 + \"%\";\n\
     }\n\
  });\n\
  xhr.onreadystatechange = function () {\n\
    if(xhr.readyState === XMLHttpRequest.DONE) {\n\
      var status = xhr.status;\n\
      if (status >= 200 && status < 400)\n\
      {\n\
        document.getElementById(\"status_div\").innerHTML = \"Upload accepted. Device will reboot.\";\n\
      } else {\n\
        document.getElementById(\"status_div\").innerHTML = \"Upload rejected!\";\n\
      }\n\
    }\n\
  };\n\
  xhr.send(data);\n\
  return false;\n\
}\n\
</script>";

//-----------------------------------------------------------------------------
static esp_err_t _ota_get_handler(httpd_req_t *req)
{
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    size_t buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if ((buf_len > 1) && (buf_len <= sizeof(auth_buffer)))
    {
        if (httpd_req_get_hdr_value_str(req, "Authorization", auth_buffer, buf_len) == ESP_OK)
        {
            char *auth_credentials = _http_auth_basic(basic_auth_info->username, basic_auth_info->password);
            if (!strncmp(auth_credentials, auth_buffer, buf_len))
            {
                printf("Authenticated!\n");
                httpd_resp_set_status(req, HTTPD_200);
                httpd_resp_set_hdr(req, "Connection", "keep-alive");
                httpd_resp_send(req, ota_html_file, strlen(ota_html_file));
                return ESP_OK;
            }
        }
    }

    printf("Not authenticated\n");
    httpd_resp_set_status(req, HTTPD_401);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

//-----------------------------------------------------------------------------
static esp_err_t _ota_post_handler(httpd_req_t *req)
{
    char buf[256];
    httpd_resp_set_status(req, HTTPD_500); // Assume failure

    int ret, remaining = req->content_len;
    printf("Receiving\n");

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (update_partition == NULL)
    {
        printf("Uh oh, bad things\n");
        goto return_failure;
    }

    printf("Writing partition: type %d, subtype %d, offset 0x%lx\n", update_partition->type, update_partition->subtype, update_partition->address);
    printf("Running partition: type %d, subtype %d, offset 0x%lx\n", running->type, running->subtype, running->address);
    esp_err_t err = ESP_OK;
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        printf("esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto return_failure;
    }
    while (remaining > 0)
    {
        // Read the data for the request
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                // Retry receiving if timeout occurred
                continue;
            }

            goto return_failure;
        }

        size_t bytes_read = ret;

        remaining -= bytes_read;
        err = esp_ota_write(update_handle, buf, bytes_read);
        if (err != ESP_OK)
        {
            goto return_failure;
        }
    }

    printf("Receiving done\n");

    // End response
    if ((esp_ota_end(update_handle) == ESP_OK) && (esp_ota_set_boot_partition(update_partition) == ESP_OK))
    {
        printf("OTA Success?!\n Rebooting\n");
        fflush(stdout);

        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_send(req, NULL, 0);

        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();

        return ESP_OK;
    }
    printf("OTA End failed (%s)!\n", esp_err_to_name(err));

return_failure:
    if (update_handle)
    {
        esp_ota_abort(update_handle);
    }

    httpd_resp_set_status(req, HTTPD_500); // Assume failure
    httpd_resp_send(req, NULL, 0);
    return ESP_FAIL;
}

//-----------------------------------------------------------------------------
static const char reset_html_file[] = "\
<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/2.2.1/css/bootstrap.min.css\">\n\
<div class=\"well\" style=\"text-align: center;\">\n\
  <div class=\"btn\" onclick=\"reset_btn.click();\"><i class=\"icon-wrench\" style=\"padding-right: 5px;\"></i>Reset Device</div>\n\
  <div class=\"status\" id=\"status_div\"></div>\n\
</div>\n\
<input type=\"button\" id=\"reset_btn\" onclick=\"reset_device()\" style=\"display: none;\">\n\
<script>\n\
function reset_device() {\n\
  document.getElementById(\"status_div\").innerHTML = \"Resetting Device...\";\n\
  xhr = new XMLHttpRequest();\n\
  xhr.open(\"POST\", \"/reset\", true);\n\
  xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');\n\
  xhr.onreadystatechange = function () {\n\
    if(xhr.readyState === XMLHttpRequest.DONE) {\n\
      var status = xhr.status;\n\
      if (status >= 200 && status < 400)\n\
      {\n\
        document.getElementById(\"status_div\").innerHTML = \"Device is rebooting, reload this page.\";\n\
      } else {\n\
        document.getElementById(\"status_div\").innerHTML = \"Device did NOT reboot?!\";\n\
      }\n\
    }\n\
  };\n\
  xhr.send("
                                      ");\n\
  return false;\n\
}\n\
</script>";

//-----------------------------------------------------------------------------
static esp_err_t _reset_post_handler(httpd_req_t *req)
{
    printf("Rebooting\n");
    fflush(stdout);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

//-----------------------------------------------------------------------------
static esp_err_t _reset_get_handler(httpd_req_t *req)
{
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    size_t buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if ((buf_len > 1) && (buf_len <= sizeof(auth_buffer)))
    {
        if (httpd_req_get_hdr_value_str(req, "Authorization", auth_buffer, buf_len) == ESP_OK)
        {
            char *auth_credentials = _http_auth_basic(basic_auth_info->username, basic_auth_info->password);
            if (!strncmp(auth_credentials, auth_buffer, buf_len))
            {
                printf("Authenticated!\n");
                httpd_resp_set_status(req, HTTPD_200);
                httpd_resp_set_hdr(req, "Connection", "keep-alive");
                httpd_resp_send(req, reset_html_file, strlen(reset_html_file));
                return ESP_OK;
            }
        }
    }

    printf("Not authenticated\n");
    httpd_resp_set_status(req, HTTPD_401);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

//-----------------------------------------------------------------------------
void http_start_webserver(httpd_handle_t *p_server)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;
    config.lru_purge_enable = true;

    // Start the httpd server
    printf("Starting server on port %d\n", config.server_port);

    if (httpd_start(p_server, &config) == ESP_OK)
    {
        static const httpd_uri_t root_post = {.uri = "/", .method = HTTP_POST, .handler = _root_post_handler, .user_ctx = NULL};
        httpd_register_uri_handler(*p_server, &root_post);

        static httpd_uri_t root_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = _root_get_handler,
            .user_ctx = &auth_info,
        };
        httpd_register_uri_handler(*p_server, &root_get);

        static const httpd_uri_t ota_post = {.uri = "/ota", .method = HTTP_POST, .handler = _ota_post_handler, .user_ctx = NULL};
        httpd_register_uri_handler(*p_server, &ota_post);

        static httpd_uri_t ota_get = {
            .uri = "/ota",
            .method = HTTP_GET,
            .handler = _ota_get_handler,
            .user_ctx = &auth_info,
        };
        httpd_register_uri_handler(*p_server, &ota_get);

        static httpd_uri_t reset_post = {
            .uri = "/reset",
            .method = HTTP_POST,
            .handler = _reset_post_handler,
            .user_ctx = NULL,
        };
        httpd_register_uri_handler(*p_server, &reset_post);

        static httpd_uri_t reset_get = {
            .uri = "/reset",
            .method = HTTP_GET,
            .handler = _reset_get_handler,
            .user_ctx = &auth_info,
        };
        httpd_register_uri_handler(*p_server, &reset_get);
    }
}

//-----------------------------------------------------------------------------
void http_stop_webserver(httpd_handle_t *p_server)
{
    httpd_stop(*p_server);
}

//-----------------------------------------------------------------------------
void http_init(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // Validate image some how, then call:
            esp_ota_mark_app_valid_cancel_rollback();
            // If needed: esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
}
