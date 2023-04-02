#include "homekit/homekit_integration.h"
#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "hardware/sensors/sensor.h"
#include "homekit_definitions.h"
#include "logging/logging_tags.h"
#include "util/cores.h"
#include "util/default_event.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include <array>
#include <esp_log.h>
#include <esp_random.h>
#include <string_view>

// these functions are internal functions for hap
extern "C" bool is_accessory_paired();

constexpr auto air_quality_sensor_id = sensor_id_index::pm_2_5;

#define CHECK_HAP_RESULT(x)                                                                                                                          \
    do                                                                                                                                               \
    {                                                                                                                                                \
        const auto error = (x);                                                                                                                      \
        if (error != HAP_SUCCESS) [[unlikely]]                                                                                                       \
        {                                                                                                                                            \
            CHECK_THROW_ESP(ESP_FAIL);                                                                                                               \
        }                                                                                                                                            \
    } while (false);

#define CHECK_NULL_RESULT(x)                                                                                                                         \
    do                                                                                                                                               \
    {                                                                                                                                                \
        if ((x) == nullptr) [[unlikely]]                                                                                                             \
        {                                                                                                                                            \
            CHECK_THROW_ESP(ESP_FAIL);                                                                                                               \
        }                                                                                                                                            \
    } while (false);

void homekit_integration::begin()
{
    homekit_task_.spawn_pinned("homekit_task", 4 * 1024, esp32::task::default_priority, esp32::wifi_core);
}

/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int device_identify(hap_acc_t *ha)
{
    ESP_LOGI(HOMEKIT_TAG, "Accessory identified");
    return esp32::event_post(APP_COMMON_EVENT, DEVICE_IDENTIFY);
}

void homekit_integration::init_hap()
{
    hap_cfg_t cfg;
    hap_get_config(&cfg);
    cfg.task_priority = esp32::task::default_priority;
    hap_set_config(&cfg);

    /* Initialize the HAP core */
    CHECK_HAP_RESULT(hap_init(HAP_TRANSPORT_WIFI));
}

void homekit_integration::homekit_task_ftn()
{
    ESP_LOGI(HOMEKIT_TAG, "Started Homekit Task on Core:%d", xPortGetCoreID());

    try
    {
        instance_app_common_event_.subscribe();

        generate_password();

        init_hap();

        /* Initialise the mandatory parameters for Accessory which will be added as
         * the mandatory services internally
         */
        name_ = config_.get_host_name();
        mac_address_ = ui_interface::get_default_mac_address();
        hap_acc_cfg_t cfg{};
        cfg.name = const_cast<char *>(name_.c_str());
        cfg.manufacturer = const_cast<char *>("Espressif");
        cfg.model = const_cast<char *>("Espressif");
        cfg.serial_num = const_cast<char *>(mac_address_.c_str());
        cfg.fw_rev = const_cast<char *>("0.0.0");
        cfg.hw_rev = NULL;
        cfg.pv = const_cast<char *>("1.1.0");
        cfg.identify_routine = device_identify;
        cfg.cid = HAP_CID_SENSOR;

        /* Create accessory object */
        accessory_ = hap_acc_create(&cfg);
        CHECK_NULL_RESULT(accessory_);

        /* Add a dummy Product Data */
        constexpr uint8_t product_data[8] = {'E', 'S', 'P', '3', '2', 'H', 'A', 'P'};
        CHECK_HAP_RESULT(hap_acc_add_product_data(accessory_, const_cast<uint8_t *>(product_data), sizeof(product_data)));

        create_sensor_services_and_chars();

        hap_add_accessory(accessory_);

        hap_set_setup_code(setup_password_.c_str());
        CHECK_HAP_RESULT(hap_set_setup_id(setup_id_.c_str()));

        instance_hap_event_.subscribe();

        CHECK_HAP_RESULT(hap_start());

        do
        {
            uint32_t notification_value = 0;
            const auto result = xTaskNotifyWait(pdFALSE,             /* Don't clear bits on entry. */
                                                ULONG_MAX,           /* Clear all bits on exit. */
                                                &notification_value, /* Stores the notified value. */
                                                portMAX_DELAY);

            if (result == pdPASS)
            {
                if (notification_value & task_notify_restarting_bit)
                {
                    hap_nw_stop();
                }

                for (auto i = 1; i <= total_sensors; i++)
                {
                    if (notification_value & BIT(i))
                    {
                        const auto id = static_cast<sensor_id_index>(i - 1);
                        auto iter = chars1_.find(id);
                        if (iter != chars1_.end())
                        {
                            hap_val_t h_value{};
                            h_value.f = get_sensor_value(id);
                            hap_char_update_val(iter->second, &h_value);
                        }

                        if (id == air_quality_sensor_id)
                        {
                            hap_val_t h_value{};
                            h_value.u = get_air_quality();
                            hap_char_update_val(air_quality_char, &h_value);
                        }
                    }
                }
            }
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGE(HOMEKIT_TAG, "Homekit Task Failure:%s", ex.what());
        throw;
    }

    vTaskDelete(NULL);
}

int homekit_integration::sensor_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv)
{
    if (hap_req_get_ctrl_id(read_priv))
    {
        ESP_LOGI(HOMEKIT_TAG, "Received read from %s", hap_req_get_ctrl_id(read_priv));
    }

    auto &instance = homekit_integration::get_instance();

    auto iter = instance.chars2_.find(hc);
    if (iter != instance.chars2_.end())
    {
        hap_val_t new_val{};
        new_val.f = homekit_integration::get_instance().get_sensor_value(iter->second);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
        return HAP_SUCCESS;
    }

    return HAP_FAIL;
}

void homekit_integration::create_sensor_services_and_chars()
{
    for (auto &&definition : homekit_definitions)
    {
        hap_serv_t *service;
        const auto iter = services_.find(definition.get_service_type_uuid());
        if (iter == services_.end())
        {
            service = hap_serv_create(const_cast<char *>(definition.get_service_type_uuid().data()));
            CHECK_NULL_RESULT(service);
            services_.insert(std::make_pair(definition.get_service_type_uuid(), service));
            hap_serv_set_read_cb(service, sensor_read);
            if (definition.get_service_type_uuid() == primary_service)
            {
                hap_serv_mark_primary(service);
            }
        }
        else
        {
            service = iter->second;
        }

        auto sensor_char = hap_char_float_create(const_cast<char *>(definition.get_cha_type_uuid().data()), HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV,
                                                 get_sensor_value(definition.get_sensor()));
        CHECK_NULL_RESULT(service);

        const auto sensor_def = get_sensor_definition(definition.get_sensor());

        hap_char_float_set_constraints(sensor_char, sensor_def.get_min_value(), sensor_def.get_max_value(), sensor_def.get_value_step());
        if (definition.get_uint_str().length())
        {
            hap_char_add_unit(sensor_char, definition.get_uint_str().data());
        }
        chars1_.insert(std::make_pair(definition.get_sensor(), sensor_char));
        chars2_.insert(std::make_pair(sensor_char, definition.get_sensor()));

        CHECK_HAP_RESULT(hap_serv_add_char(service, sensor_char));
    }

    air_quality_char = hap_char_air_quality_create(get_air_quality());
    CHECK_NULL_RESULT(air_quality_char);
    static_assert(homekit_definitions[0].get_sensor() == air_quality_sensor_id);
    CHECK_HAP_RESULT(hap_serv_add_char(services_[homekit_definitions[0].get_service_type_uuid()], air_quality_char));

    for (auto &&[_, service] : services_)
    {
        CHECK_HAP_RESULT(hap_acc_add_serv(accessory_, service));
    }
}

uint8_t homekit_integration::get_air_quality()
{
    const auto value = homekit_integration::get_instance().hardware_.get_sensor(air_quality_sensor_id).get_value();

    if (!value.has_value())
    {
        return 0; // unknown
    }
    else
    {
        const auto sensor_def = get_sensor_definition(air_quality_sensor_id);
        auto level = sensor_def.calculate_level(value.value());

        if (level > 5)
        {
            level = 5;
        }

        return level;
    }
}

void homekit_integration::hap_event_handler(esp_event_base_t event_base, int32_t event, void *data)
{
    switch (event)
    {
    case HAP_EVENT_PAIRING_STARTED:
        ESP_LOGI(HOMEKIT_TAG, "Pairing Started");
        break;
    case HAP_EVENT_PAIRING_ABORTED:
        ESP_LOGI(HOMEKIT_TAG, "Pairing Aborted");
        break;
    case HAP_EVENT_CTRL_PAIRED:
        ESP_LOGI(HOMEKIT_TAG, "Controller %s Paired. Controller count: %d", (char *)data, hap_get_paired_controller_count());
        break;
    case HAP_EVENT_CTRL_UNPAIRED:
        ESP_LOGI(HOMEKIT_TAG, "Controller %s Removed. Controller count: %d", (char *)data, hap_get_paired_controller_count());
        break;
    case HAP_EVENT_CTRL_CONNECTED:
        ESP_LOGI(HOMEKIT_TAG, "Controller %s Connected", (char *)data);
        break;
    case HAP_EVENT_CTRL_DISCONNECTED:
        ESP_LOGI(HOMEKIT_TAG, "Controller %s Disconnected", (char *)data);
        break;
    case HAP_EVENT_ACC_REBOOTING: {
        char *reason = (char *)data;
        ESP_LOGI(HOMEKIT_TAG, "Accessory Rebooting (Reason: %s)", reason ? reason : "null");
    }
    break;
    case HAP_EVENT_PAIRING_MODE_TIMED_OUT:
        ESP_LOGI(HOMEKIT_TAG, "Pairing Mode timed out. Please reboot the device.");
    }
}

void homekit_integration::app_event_handler(esp_event_base_t base, int32_t event, void *data)
{
    switch (event)
    {
    case SENSOR_VALUE_CHANGE: {
        const auto id = (*reinterpret_cast<sensor_id_index *>(data));
        xTaskNotify(homekit_task_.handle(), BIT(static_cast<uint8_t>(id) + 1), eSetBits);
    }
    break;
    case APP_EVENT_REBOOT:
        xTaskNotify(homekit_task_.handle(), task_notify_restarting_bit, eSetBits);
        break;
    }
}

float homekit_integration::get_sensor_value(sensor_id_index id)
{
    const auto &sensor = hardware_.get_sensor(id);
    return sensor.get_value().value_or(0);
}

void homekit_integration::generate_password()
{
    std::array<uint8_t, 8 + 4 + 1> buffer;
    esp_fill_random(buffer.data(), buffer.size());

    for (auto &&c : buffer)
    {
        c = c % 10;
    }

    setup_password_ =
        esp32::string::sprintf("%d%d%d-%d%d-%d%d%d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

    setup_id_ = esp32::string::sprintf("%c%c%c%d", 65 + buffer[8], 65 + buffer[9] + buffer[12], 65 + buffer[10], buffer[11]);
}

bool homekit_integration::is_paired()
{
    return is_accessory_paired();
}

uint16_t homekit_integration::get_connection_count()
{
    return hap_get_paired_controller_count();
}

const std::string &homekit_integration::get_password() const
{
    return setup_password_;
}

const std::string &homekit_integration::get_setup_id() const
{
    return setup_id_;
}

void homekit_integration::forget_pairings()
{
    ESP_LOGW(HOMEKIT_TAG, "Resetting Homekit data");
    CHECK_HAP_RESULT(hap_reset_homekit_data());
}

void homekit_integration::reenable_pairing()
{
    ESP_LOGI(HOMEKIT_TAG, "Renabling Pairing");
    hap_pair_setup_re_enable();
}
