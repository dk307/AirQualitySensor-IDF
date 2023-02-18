#include "display.h"

#include <lvgl.h>
#include "lvgl_fs\lvgl_fs_sd_card.h"
#include "exceptions.h"

#include "ui/ui2.h"
#include "config/config_manager.h"
#include "wifi/wifi_manager.h"
#include "hardware/hardware.h"

#include "logging/logging_tags.h"

#include <esp_log.h>

const int LV_TICK_PERIOD_MS = 1;

const int task_notify_wifi_changed_bit = BIT(total_sensors + 1);
const int set_main_screen_changed_bit = BIT(total_sensors + 2);

/* Display flushing */
void display::display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    auto display_device = reinterpret_cast<LGFX *>(disp->user_data);
    if (display_device->getStartCount() == 0)
    {
        display_device->endWrite();
    }

    display_device->pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
                                 (lgfx::swap565_t *)&color_p->full);

    lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/*Read the touchpad*/
void display::touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    auto display_device = reinterpret_cast<LGFX *>(indev_driver->user_data);
    uint16_t touchX, touchY;
    const bool touched = display_device->getTouch(&touchX, &touchY);

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

void display::pre_begin()
{
    ESP_LOGI(DISPLAY_TAG, "Setting up display");

    lv_init();
    lv_fs_if_fatfs_init();

    if (!display_device.init())
    {
        CHECK_THROW("Failed to init display", ESP_FAIL, init_failure_exception);
    }
    display_device.setRotation(1);

    display_device.initDMA();
    display_device.startWrite();

    const auto screenWidth = display_device.width();
    const auto screenHeight = display_device.height();

    ESP_LOGI(DISPLAY_TAG, "Display initialized width:%ld height:%ld", screenWidth, screenHeight);

    ESP_LOGI(DISPLAY_TAG, "LV initialized");
    const int buffer_size = 80;

    const auto display_buffer_size = screenWidth * buffer_size * sizeof(lv_color_t);
    ESP_LOGI(DISPLAY_TAG, "Display buffer size:%ld", display_buffer_size);
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);
    disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);

    if (!disp_draw_buf || !disp_draw_buf2)
    {
        CHECK_THROW("Failed to allocate lvgl display buffer", ESP_ERR_NO_MEM, init_failure_exception);
    }

    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, disp_draw_buf2, screenWidth * buffer_size);

    ESP_LOGD(DISPLAY_TAG, "LVGL display buffer initialized");

    /*** LVGL : Setup & Initialize the display device driver ***/
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = display_device.width();
    disp_drv.ver_res = display_device.height();
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = &display_device;
    lv_display = lv_disp_drv_register(&disp_drv);

    ESP_LOGD(DISPLAY_TAG, "LVGL display initialized");

    //*** LVGL : Setup & Initialize the input device driver ***F
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_drv.user_data = &display_device;
    lv_indev_drv_register(&indev_drv);

    ESP_LOGD(DISPLAY_TAG, "LVGL input device driver initialized");

    create_timer();

    const auto err = lvgl_task.spawn_pinned("lv gui", 1024 * 4, 1, 1);

    if (err != ESP_OK)
    {
        if (lv_periodic_timer)
        {
            esp_timer_delete(lv_periodic_timer);
        }
        CHECK_THROW("Create task for LVGL failed", err, init_failure_exception);
    }

    ui_instance.init();

    display_device.setBrightness(128);
    ESP_LOGI(DISPLAY_TAG, "Display setup done");
}

void display::begin()
{
    for (auto i = 0; i < total_sensors; i++)
    {
        const auto id = static_cast<sensor_id_index>(i);
        hardware::instance.get_sensor(id).add_callback([i, this]
                                                       { xTaskNotify(lvgl_task.handle(),
                                                                     BIT(i + 1),
                                                                     eSetBits); });
    }

    wifi_manager::instance.add_callback([this]
                                        { xTaskNotify(lvgl_task.handle(),
                                                      task_notify_wifi_changed_bit,
                                                      eSetBits); });

    ESP_LOGI(DISPLAY_TAG, "Display Ready");
}

void display::set_main_screen()
{
    xTaskNotify(lvgl_task.handle(),
                set_main_screen_changed_bit,
                eSetBits);
}

uint8_t display::get_brightness()
{
    return display_device.getBrightness();
}

void display::set_brightness(uint8_t value)
{
    display_device.setBrightness(value);
}

void display::create_timer()
{
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    esp_timer_create_args_t lv_periodic_timer_args{};
    lv_periodic_timer_args.callback = &lv_tick_task;
    lv_periodic_timer_args.name = "periodic_gui";
    ESP_ERROR_CHECK(esp_timer_create(&lv_periodic_timer_args, &lv_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lv_periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

void display::lv_tick_task(void *)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

void display::gui_task()
{
    ESP_LOGI(DISPLAY_TAG, "Start to run LVGL Task");
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
                ui_instance.wifi_changed();
            }

            if (notification_value & set_main_screen_changed_bit)
            {
                ui_instance.set_main_screen();
            }

            for (auto i = 1; i <= total_sensors; i++)
            {
                if (notification_value & BIT(i))
                {
                    const auto id = static_cast<sensor_id_index>(i - 1);
                    const auto &sensor = hardware::instance.get_sensor(id);
                    const auto value = sensor.get_value();
                    ui_instance.set_sensor_value(id, value);
                }
            }
        }
    } while(true);
}
