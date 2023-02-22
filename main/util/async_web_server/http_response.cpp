#include "http_response.h"
#include "http_request.h"
#include <filesystem>

#include "util/helper.h"
#include "util/exceptions.h"
#include "util/filesystem/filesystem.h"
#include "util/filesystem/file_info.h"

#include <esp_log.h>
#include <esp_check.h>
#include "logging/logging_tags.h"

namespace esp32
{
  void http_response::add_common_headers()
  {
    add_header("Connection", "keep-alive");
  }

  void http_response::add_header(const char *field, const char *value)
  {
    CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, field, value));
  }

  void http_response::redirect(const std::string &url)
  {
    CHECK_HTTP_REQUEST(httpd_resp_set_status(req_->req_, "302 Found"));
    CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Location", url.c_str()));
    CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Cache-Control", "no-cache"));
    CHECK_HTTP_REQUEST(httpd_resp_send(req_->req_, nullptr, 0));
  }

  void http_response::send_empty_200()
  {
    add_common_headers();
    CHECK_HTTP_REQUEST(httpd_resp_send(req_->req_, "", HTTPD_RESP_USE_STRLEN));
  }

  void http_response::send_error(httpd_err_code_t code, const char *message)
  {
    add_common_headers();
    CHECK_HTTP_REQUEST(httpd_resp_send_err(req_->req_, code, message));
  }

  void array_response::send_response()
  {
    ESP_LOGD(WEBSERVER_TAG, "Handling %s", req_->url().c_str());
    add_common_headers();

    // content type
    CHECK_HTTP_REQUEST(httpd_resp_set_type(req_->req_, content_type_));
    if (is_gz_)
    {
      CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Content-Encoding", "gzip"));
    }

    httpd_resp_set_status(req_->req_, HTTPD_200);
    httpd_resp_set_hdr(req_->req_, "Connection", "keep-alive");
    httpd_resp_send(req_->req_, reinterpret_cast<const char *>(buf_), buf_len_);
  }

  void array_response::send_response(esp32::http_request *request, const std::string &data_str, const char *content_type)
  {
    esp32::array_response response(request, reinterpret_cast<const uint8_t *>(data_str.c_str()), data_str.size(), false, content_type);
    response.send_response();
  }

  void fs_card_file_response::send_response()
  {
    ESP_LOGD(WEBSERVER_TAG, "Handling %s", req_->url().c_str());
    add_common_headers();

    const auto gz_path = std::string(file_path_) + ".gz";

    // Content encoding
    std::filesystem::path path;
    if (!download_ && !esp32::filesystem::exists(path) && esp32::filesystem::exists(gz_path))
    {
      path = gz_path;
      CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Content-Encoding", "gzip"));
    }
    else
    {
      path = file_path_;
    }

    const esp32::filesystem::file_info file_info{path};

    // Check file exists
    if (!file_info.exists())
    {
      ESP_LOGE(WEBSERVER_TAG, "Failed to find file : %s", path.c_str());
      httpd_resp_send_err(req_->req_, HTTPD_404_NOT_FOUND, "File does not exist");
      return;
    }

    if (!file_info.is_regular_file())
    {
      ESP_LOGE(WEBSERVER_TAG, "Path is not a file : %s", path.c_str());
      httpd_resp_send_err(req_->req_, HTTPD_404_NOT_FOUND, "Parh is not a file");
      return;
    }

    // Content dispostion
    const auto filename = path.filename();
    const auto content_disposition_header =
        download_ ? esp32::str_sprintf("attachment; filename=\"%s\"", filename.c_str()) : esp32::str_sprintf("inline; filename=\"%s\"", filename.c_str());
    CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Content-Disposition", content_disposition_header.c_str()));

    // content type
    CHECK_HTTP_REQUEST(httpd_resp_set_type(req_->req_, content_type_));

    // Content-Length
    const auto size = std::to_string(file_info.size());
    CHECK_HTTP_REQUEST(httpd_resp_set_hdr(req_->req_, "Content-Length", size.c_str()));

    ESP_LOGD(WEBSERVER_TAG, "Sending file %s", path.c_str());

    auto file_handle = fopen(path.c_str(), "r");
    if (!file_handle)
    {
      ESP_LOGE(WEBSERVER_TAG, "Failed to read existing file : %s", path.c_str());
      httpd_resp_send_err(req_->req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return;
    }

    /* Retrieve the pointer to scratch buffer for temporary storage */
    size_t chuck_size = 8192;
    const auto chunk = std::make_unique<char[]>(chuck_size);

    if (!chunk)
    {
      CHECK_HTTP_REQUEST(ESP_ERR_NO_MEM);
    }

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
          CHECK_HTTP_REQUEST(httpd_resp_sendstr_chunk(req_->req_, NULL));
          CHECK_HTTP_REQUEST(httpd_resp_send_err(req_->req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file"));
        }
      }
    } while (chuck_size != 0);

    fclose(file_handle);

    /* Close file after sending complete */
    ESP_LOGD(WEBSERVER_TAG, "File sending complete for %s", path.c_str());

    httpd_resp_send_chunk(req_->req_, NULL, 0);
  }
}
