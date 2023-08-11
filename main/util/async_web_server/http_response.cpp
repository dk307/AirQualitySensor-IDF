#include "http_response.h"
#include "http_request.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/finally.h"
#include "util/helper.h"
#include <esp_check.h>
#include <esp_log.h>
#include <filesystem>
#include <memory>

namespace esp32
{
void http_response::add_common_headers()
{
    add_header("Connection", "close");
    // add_header("Connection", "keep-alive");
    // add_header("Access-Control-Allow-Origin", "*");
}

void http_response::add_header(const char *field, const char *value)
{
    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, field, value));
}

void http_response::redirect(const std::string &url)
{
    CHECK_THROW_ESP(httpd_resp_set_status(request_.req_, "302 Found"));
    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Location", url.c_str()));
    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Cache-Control", "no-cache"));
    CHECK_THROW_ESP(httpd_resp_send(request_.req_, nullptr, 0));
}

void http_response::send_empty_200()
{
    add_common_headers();
    CHECK_THROW_ESP(httpd_resp_send(request_.req_, "", HTTPD_RESP_USE_STRLEN));
}

void http_response::send_error(httpd_err_code_t code, const char *message)
{
    add_common_headers();
    CHECK_THROW_ESP(httpd_resp_send_err(request_.req_, code, message));
}

void array_response::send_response()
{
    ESP_LOGD(WEBSERVER_TAG, "Handling %s", request_.url().c_str());
    add_common_headers();

    // content type
    CHECK_THROW_ESP(httpd_resp_set_type(request_.req_, content_type_.data()));
    if (is_gz_)
    {
        CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Content-Encoding", "gzip"));
    }

    if (sha256_.has_value())
    {
        const auto match_sha256 = request_.get_header("If-None-Match");
        if (match_sha256.has_value() && (match_sha256.value() == sha256_.value()))
        {
            CHECK_THROW_ESP(httpd_resp_set_status(request_.req_, "304 Not Modified"));
            CHECK_THROW_ESP(httpd_resp_send(request_.req_, "", 0));
            return;
        }
    }

    CHECK_THROW_ESP(httpd_resp_set_status(request_.req_, HTTPD_200));
    if (sha256_.has_value())
    {
        CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "ETag", sha256_.value().data()));
    }
    CHECK_THROW_ESP(httpd_resp_send(request_.req_, reinterpret_cast<const char *>(buf_.data()), buf_.size()));
}

void array_response::send_response(esp32::http_request &request, const std::string_view &data_str, const std::string_view &content_type)
{
    esp32::array_response response(request, {reinterpret_cast<const uint8_t *>(data_str.data()), data_str.size()}, std::nullopt, false, content_type);
    response.send_response();
}

void fs_card_file_response::send_response()
{
    ESP_LOGD(WEBSERVER_TAG, "Handling %s", request_.url().c_str());
    add_common_headers();

    const auto gz_path = std::string(file_path_) + ".gz";

    // Content encoding
    std::filesystem::path path;
    if (!download_ && !esp32::filesystem::exists(path) && esp32::filesystem::exists(gz_path))
    {
        path = gz_path;
        CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Content-Encoding", "gzip"));
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
        httpd_resp_send_err(request_.req_, HTTPD_404_NOT_FOUND, "File does not exist");
        return;
    }

    if (!file_info.is_regular_file())
    {
        ESP_LOGE(WEBSERVER_TAG, "Path is not a file : %s", path.c_str());
        httpd_resp_send_err(request_.req_, HTTPD_404_NOT_FOUND, "Parh is not a file");
        return;
    }

    const auto etag = esp32::string::sprintf("%lld-%u", file_info.last_modified(), file_info.size());

    const auto existing_etag = request_.get_header("If-None-Match");
    if (existing_etag.has_value() && (existing_etag.value() == etag))
    {
        CHECK_THROW_ESP(httpd_resp_set_status(request_.req_, "304 Not Modified"));
        CHECK_THROW_ESP(httpd_resp_send(request_.req_, "", 0));
        return;
    }

    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "ETag", etag.c_str()));

    // Content dispostion
    const auto filename = path.filename();
    const auto content_disposition_header = download_ ? esp32::string::sprintf("attachment; filename=\"%s\"", filename.c_str())
                                                      : esp32::string::sprintf("inline; filename=\"%s\"", filename.c_str());
    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Content-Disposition", content_disposition_header.c_str()));

    // content type
    CHECK_THROW_ESP(httpd_resp_set_type(request_.req_, content_type_.data()));

    // Content-Length
    const auto size = esp32::string::to_string(file_info.size());
    CHECK_THROW_ESP(httpd_resp_set_hdr(request_.req_, "Content-Length", size.c_str()));

    ESP_LOGD(WEBSERVER_TAG, "Sending file %s", path.c_str());

    auto file_handle = fopen(path.c_str(), "r");
    if (!file_handle)
    {
        ESP_LOGE(WEBSERVER_TAG, "Failed to read existing file : %s", path.c_str());
        httpd_resp_send_err(request_.req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return;
    }

    auto auto_close_file = esp32::finally([&file_handle] { fclose(file_handle); });

    /* Retrieve the pointer to scratch buffer for temporary storage */
    size_t chuck_size = 16 * 1024;
    const auto chunk = std::make_unique<char[]>(chuck_size);

    if (!chunk)
    {
        CHECK_THROW_ESP(ESP_ERR_NO_MEM);
    }

    do
    {
        /* Read file in chunks into the scratch buffer */
        chuck_size = fread(chunk.get(), 1, chuck_size, file_handle);

        if (chuck_size > 0)
        {
            /* Send the buffer contents as HTTP response chunk */
            const auto send_error = httpd_resp_send_chunk(request_.req_, chunk.get(), chuck_size);
            if (send_error != ESP_OK)
            {
                ESP_LOGE(WEBSERVER_TAG, "File sending failed for %s with %s", path.c_str(), esp_err_to_name(send_error));

                /* Abort sending file */
                CHECK_THROW_ESP(httpd_resp_sendstr_chunk(request_.req_, NULL));
                CHECK_THROW_ESP(httpd_resp_send_err(request_.req_, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file"));
            }
        }
    } while (chuck_size != 0);

    /* Close file after sending complete */
    ESP_LOGD(WEBSERVER_TAG, "File sending complete for %s", path.c_str());

    httpd_resp_send_chunk(request_.req_, NULL, 0);
}
} // namespace esp32
