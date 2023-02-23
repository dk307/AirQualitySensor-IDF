#include "operations.h"

#include "logging/logging_tags.h"
#include "config/config_manager.h"

#include <nvs_flash.h>
#include <esp_log.h>

operations operations::instance;

void operations::factory_reset()
{
	ESP_LOGW(OPERATIONS_TAG, "Doing Factory Reset");
	nvs_flash_erase();
	config::erase();
	reset();
}

void operations::begin()
{
}

void operations::reboot()
{
	reset();
}

// bool operations::start_update(size_t length, const std::string &md5, std::string &error)
// {
// 	ESP_LOGI(OPERATIONS_TAG, "Update call start with length:%d bytes", length);
// 	ESP_LOGI(OPERATIONS_TAG, "Current Sketch size:%d bytes", ESP.getSketchSize());
// 	ESP_LOGI(OPERATIONS_TAG, "Free sketch space:%d bytes", ESP.getFreeSketchSpace());

// 	const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
// 	if (!Update.setMD5(md5.c_str()))
// 	{
// 		ESP_LOGE(OPERATIONS_TAG, "Md5 Invalid:%s", error.c_str());
// 		return false;
// 	}

// 	if (Update.begin(maxSketchSpace))
// 	{
// 		ESP_LOGI(OPERATIONS_TAG, "Update begin successful");
// 		return true;
// 	}
// 	else
// 	{
// 		get_update_error(error);
// 		ESP_LOGE(OPERATIONS_TAG, "Update begin failed with %s", error.c_str());
// 		return false;
// 	}
// }

// bool operations::write_update(const uint8_t *data, size_t length, std::string &error)
// {
// 	ESP_LOGD(OPERATIONS_TAG, "Update write with length:%d", length);
// 	ESP_LOGD(OPERATIONS_TAG, "Update stats Size: %d progress:%d remaining:%d ", Update.size(), Update.progress(), Update.remaining());
// 	const auto written = Update.write(const_cast<uint8_t *>(data), length);
// 	if (written == length)
// 	{
// 		ESP_LOGD(OPERATIONS_TAG, "Update write successful");
// 		return true;
// 	}
// 	else
// 	{
// 		get_update_error(error);
// 		ESP_LOGE(OPERATIONS_TAG, "Update write failed with %s", error.c_str());
// 		return false;
// 	}
// }

// bool operations::end_update(std::string &error)
// {
// 	ESP_LOGE(OPERATIONS_TAG, "Update end called");

// 	if (Update.end(true))
// 	{
// 		ESP_LOGI(OPERATIONS_TAG, "Update end successful");
// 		return true;
// 	}
// 	else
// 	{
// 		get_update_error(error);
// 		ESP_LOGE(OPERATIONS_TAG, "Update end failed with %s", error.c_str());
// 		return false;
// 	}
// }

// void operations::abort_update()
// {
// 	ESP_LOGD(OPERATIONS_TAG, "Update end called");
// 	if (Update.isRunning())
// 	{
// 		if (Update.end(true))
// 		{
// 			ESP_LOGI(OPERATIONS_TAG, "Aborted update");
// 		}
// 		else
// 		{
// 			ESP_LOGE(OPERATIONS_TAG, "Aborted update failed");
// 		}
// 	}
// }

// bool operations::is_update_in_progress()
// {
// 	return Update.isRunning();
// }

// void operations::get_update_error(std::string &error)
// {
// 	StreamString streamString;
// 	Update.printError(streamString);
// 	error = std::move(streamString);
// }

[[noreturn]] void operations::reset()
{
	ESP_LOGI(OPERATIONS_TAG, "Restarting...");
	vTaskDelay(pdMS_TO_TICKS(2000));
	; // for http response, etc to finish
	esp_restart();
}