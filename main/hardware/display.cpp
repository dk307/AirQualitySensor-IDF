#include "display.h"
#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "logging/logging_tags.h"
#include "lvgl_fs/lvgl_fs_sd_card.h"
#include "ui/ui2.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "wifi/wifi_manager.h"

#include <esp_log.h>
#include <lvgl.h>

const int LV_TICK_PERIOD_MS = 1;

const int task_notify_wifi_changed_bit = BIT(total_sensors + 1);
const int set_main_screen_changed_bit = BIT(total_sensors + 2);

/* Display flushing */
void display::display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    auto display_device_ = reinterpret_cast<LGFX *>(disp->user_data);
    if (display_device_->getStartCount() == 0)
    {
        display_device_->endWrite();
    }

    display_device_->pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);

    lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/*Read the touchpad*/
void display::touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    auto display_device_ = reinterpret_cast<LGFX *>(indev_driver->user_data);
    uint16_t touchX, touchY;
    const bool touched = display_device_->getTouch(&touchX, &touchY);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
        ESP_LOGV(DISPLAY_TAG, "Touch at point %d:%d", touchX, touchY);
    }
}

void display::start()
{
    ESP_LOGI(DISPLAY_TAG, "Setting up display");

    lv_init();
    lv_fs_if_fatfs_init();

    if (!display_device_.init())
    {
        CHECK_THROW_INIT2(ESP_FAIL, "Failed to init display");
    }

    display_device_.setRotation(1);
    display_device_.initDMA();
    display_device_.startWrite();

    const auto screenWidth = display_device_.width();
    const auto screenHeight = display_device_.height();

    ESP_LOGI(DISPLAY_TAG, "Display initialized width:%ld height:%ld", screenWidth, screenHeight);

    ESP_LOGI(DISPLAY_TAG, "LV initialized");
    const int buffer_size = 80;

    const auto display_buffer_size = screenWidth * buffer_size * sizeof(lv_color_t);
    ESP_LOGI(DISPLAY_TAG, "Display buffer size:%ld", display_buffer_size);
    disp_draw_buf_ = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);
    disp_draw_buf2_ = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);

    if (!disp_draw_buf_ || !disp_draw_buf2_)
    {
        CHECK_THROW_INIT2(ESP_ERR_NO_MEM, "Failed to allocate lvgl display buffer");
    }

    lv_disp_draw_buf_init(&draw_buf_, disp_draw_buf_, disp_draw_buf2_, screenWidth * buffer_size);

    ESP_LOGD(DISPLAY_TAG, "LVGL display buffer initialized");

    /*** LVGL : Setup & Initialize the display device driver ***/
    lv_disp_drv_init(&disp_drv_);
    disp_drv_.hor_res = display_device_.width();
    disp_drv_.ver_res = display_device_.height();
    disp_drv_.flush_cb = display_flush;
    disp_drv_.draw_buf = &draw_buf_;
    disp_drv_.user_data = &display_device_;
    lv_display_ = lv_disp_drv_register(&disp_drv_);

    ESP_LOGD(DISPLAY_TAG, "LVGL display initialized");

    //*** LVGL : Setup & Initialize the input device driver ***F
    lv_indev_drv_init(&indev_drv_);
    indev_drv_.type = LV_INDEV_TYPE_POINTER;
    indev_drv_.read_cb = touchpad_read;
    indev_drv_.user_data = &display_device_;
    lv_indev_drv_register(&indev_drv_);

    ESP_LOGD(DISPLAY_TAG, "LVGL input device driver initialized");

    create_timer();

    const auto err = lvgl_task_.spawn_pinned("lv gui", 1024 * 8, esp32::task::default_priority, esp32::display_core);

    if (err != ESP_OK)
    {
        if (lv_periodic_timer_)
        {
            esp_timer_delete(lv_periodic_timer_);
        }
        CHECK_THROW_INIT2(err, "Create task for LVGL failed");
    }

    display_device_.setBrightness(128);
    ESP_LOGI(DISPLAY_TAG, "Display setup done");
}

void display::set_callbacks()
{
    for (auto i = 0; i < total_sensors; i++)
    {
        const auto id = static_cast<sensor_id_index>(i);
        hardware::instance.get_sensor(id).add_callback([i, this] { xTaskNotify(lvgl_task_.handle(), BIT(i + 1), eSetBits); });
    }

    wifi_manager::instance.add_callback([this] { xTaskNotify(lvgl_task_.handle(), task_notify_wifi_changed_bit, eSetBits); });

    ESP_LOGI(DISPLAY_TAG, "Display Ready");
}

void display::set_main_screen()
{
    xTaskNotify(lvgl_task_.handle(), set_main_screen_changed_bit, eSetBits);
}

uint8_t display::get_brightness()
{
    return display_device_.getBrightness();
}

void display::set_brightness(uint8_t value)
{
    display_device_.setBrightness(value);
}

void display::create_timer()
{
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    esp_timer_create_args_t lv_periodic_timer_args{};
    lv_periodic_timer_args.callback = &lv_tick_task;
    lv_periodic_timer_args.name = "periodic_gui";
    ESP_ERROR_CHECK(esp_timer_create(&lv_periodic_timer_args, &lv_periodic_timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lv_periodic_timer_, LV_TICK_PERIOD_MS * 1000));
}

void display::lv_tick_task(void *)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

void display::gui_task()
{
    ESP_LOGI(DISPLAY_TAG, "Start to run LVGL Task on core:%d", xPortGetCoreID());

    try
    {
        ui_instance_.load_boot_screen();
        lv_task_handler();
        ui_instance_.init();

        set_callbacks();

        do
        {
            lv_task_handler();

            uint32_t notification_value = 0;
            const auto result = xTaskNotifyWait(pdFALSE,             /* Don't clear bits on entry. */
                                                ULONG_MAX,           /* Clear all bits on exit. */
                                                &notification_value, /* Stores the notified value. */
                                                pdMS_TO_TICKS(10));

            if (result == pdPASS)
            {
                if (notification_value & task_notify_wifi_changed_bit)
                {
                    ui_instance_.wifi_changed();
                }

                if (notification_value & set_main_screen_changed_bit)
                {
                    ui_instance_.set_main_screen();
                }

                for (auto i = 1; i <= total_sensors; i++)
                {
                    if (notification_value & BIT(i))
                    {
                        const auto id = static_cast<sensor_id_index>(i - 1);
                        const auto &sensor = hardware::instance.get_sensor(id);
                        const auto value = sensor.get_value();
                        ui_instance_.set_sensor_value(id, value);
                    }
                }
            }
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "UI Task Failure:%s", ex.what());
        vTaskDelay(pdMS_TO_TICKS(5000));
        operations::instance.reboot();
    }

    vTaskDelete(NULL);
}
