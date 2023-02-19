#include "http_response.h"
#include "http_request.h"
#include <filesystem>

#include <util/helper.h>
#include <esp_log.h>
#include <esp_check.h>
#include "logging/logging_tags.h"

namespace esp32
{

#define LOG_AND_RETURN(x)                                                                  \
  do                                                                                       \
  {                                                                                        \
    const auto err_rc_ = (x);                                                              \
    if (err_rc_ != ESP_OK)                                                                 \
    {                                                                                      \
      ESP_LOGE(WEBSERVER_TAG, "Failed with %s(%d)", esp_err_to_name(err_rc_), err_rc_); \
      return err_rc_;                                                                      \
    }                                                                                      \
  } while (0)

  esp_err_t fs_card_file_response::send_response()
  {
    ESP_LOGD(WEBSERVER_TAG, "Handling %s", req_->url().c_str());
    const auto gz_path = file_path_.generic_string() + ".gz";

    // Content encoding
    std::filesystem::path path;
    if (!download_ && !std::filesystem::exists(path) && std::filesystem::exists(gz_path))
    {
      path = gz_path;
      LOG_AND_RETURN(httpd_resp_set_hdr(req_->req_, "Content-Encoding", "gzip"));
    }
    else
    {
      path = file_path_;
    }

    // Check file exists
    if (!std::filesystem::exists(path))
    {
      ESP_LOGE(WEBSERVER_TAG, "Failed to find file : %s", path.c_str());
      httpd_resp_send_err(req_->req_, HTTPD_404_NOT_FOUND, "File does not exist");
      return ESP_FAIL;
    }

    // Content dispostion
    const auto filename = path.filename();
    const auto content_disposition_header =
        download_ ? esp32::str_sprintf("attachment; filename=\"%s\"", filename.c_str()) : esp32::str_sprintf("inline; filename=\"%s\"", filename.c_str());
    LOG_AND_RETURN(httpd_resp_set_hdr(req_->req_, "Content-Disposition", content_disposition_header.c_str()));

    // content type
    LOG_AND_RETURN(httpd_resp_set_type(req_->req_, content_type_.c_str()));

    // Content-Length
    const auto size = std::to_string(std::filesystem::file_size(path));
    LOG_AND_RETURN(httpd_resp_set_hdr(req_->req_, "Content-Length", size.c_str()));

    ESP_LOGD(WEBSERVER_TAG, "Sending file %s", path.c_str());

    auto file_handle = fopen(path.c_str(), "r");
    if (!file_handle)
    {
      ESP_LOGE(WEBSERVER_TAG, "Failed to read existing file : %s", path.c_str());
      httpd_resp_send_err(req_->req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return ESP_FAIL;
    }

    /* Retrieve the pointer to scratch buffer for temporary storage */
    size_t chuck_size = 8192;
    auto chunk = std::make_unique<char[]>(chuck_size);

    do
    {
      /* Read file in chunks into the scratch buffer */
      chuck_size = fread(chunk.get(), 1, chuck_size, file_handle);

      if (chuck_size > 0)
      {
        /* Send the buffer contents as HTTP response chunk */
        const auto send_error = httpd_resp_send_chunk(req_->req_, chunk.get(), chuck_size);
        if (send_error != ESP_OK)
        {
          fclose(file_handle);
          ESP_LOGE(WEBSERVER_TAG, "File sending failed for %s with %s", path.c_str(), esp_err_to_name(send_error));

          /* Abort sending file */
          httpd_resp_sendstr_chunk(req_->req_, NULL);
          httpd_resp_send_err(req_->req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
          return ESP_FAIL;
        }
      }
    } while (chuck_size != 0);

    fclose(file_handle);

    /* Close file after sending complete */
    ESP_LOGD(WEBSERVER_TAG, "File sending complete for %s", path.c_str());

    httpd_resp_send_chunk(req_->req_, NULL, 0);
    return ESP_OK;
  }
}
