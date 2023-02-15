#include "display.h"

#include <lvgl.h>
#include "lvgl_fs\lvgl_fs_sd_card.h"

// #include "ui/ui2.h"
// #include "config_manager.h"
// #include "wifi_manager.h"
// #include "hardware.h"

#include "logging/logging_tags.h"

#include <esp_log.h>

#define LV_TICK_PERIOD_MS 1

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

bool display::pre_begin()
{
    ESP_LOGI(DISPLAY_TAG, "Setting up display");

    lv_init();
    lv_fs_if_fatfs_init();

    if (!display_device.init())
    {
        ESP_LOGE(DISPLAY_TAG, "Failed to init display");
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
        ESP_LOGE(DISPLAY_TAG, "Failed to allocate lvgl display buffer");
        return false;
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

    const auto err = lvgl_task.spawn_pinned("lv gui", 1024 * 4, 5, 0);

    if (err != ESP_OK)
    {
        ESP_LOGE(DISPLAY_TAG, "Create task for LVGL failed");
        if (lv_periodic_timer)
        {
            esp_timer_delete(lv_periodic_timer);
        }
        return false;
    }

    ui_instance.init();

    display_device.setBrightness(128);
    ESP_LOGI(DISPLAY_TAG, "Display setup done");
    return true;
}

// void display::begin()
// {
//     for (auto i = 0; i < total_sensors; i++)
//     {
//         const auto id = static_cast<sensor_id_index>(i);
//         hardware::instance.get_sensor(id).add_callback([id, this]
//                                                        {
//             const auto& sensor =  hardware::instance.get_sensor(id);
//             const auto value = sensor.get_value();
//             std::lock_guard<esp32::semaphore> lock(lgvl_mutex);
//             ui_instance.set_sensor_value(id, value); });
//     }

//     wifi_manager::instance.add_callback([this]
//                                         {
//             std::lock_guard<esp32::semaphore> lock(lgvl_mutex);
//             ui_instance.wifi_changed(); });

//     ESP_LOGI(DISPLAY_TAG, "Display Ready");
// }

void display::update_boot_message(const std::string &message)
{
    std::lock_guard<esp32::semaphore> lock(lvgl_mutex);
    ui_instance.update_boot_message(message);
}

void display::set_main_screen()
{
    std::lock_guard<esp32::semaphore> lock(lvgl_mutex);
    ESP_LOGI(DISPLAY_TAG, "Switching to main screen");
    ui_instance.set_main_screen();
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
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        std::lock_guard<esp32::semaphore> lock(lvgl_mutex);
        lv_task_handler();
    }
}
