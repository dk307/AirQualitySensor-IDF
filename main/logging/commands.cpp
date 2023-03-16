#include "commands.h"
#include "logging/logger.h"
#include "logging/logging_tags.h"
#include "util/helper.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <string>

static void up_time_cli_handler()
{
    ESP_LOGI(COMMAND_TAG, "Uptime: %lld milliseconds", esp_timer_get_time() / 1000);
}

static void task_dump_cli_handler()
{
    auto num_of_tasks = uxTaskGetNumberOfTasks();
    const auto task_array = reinterpret_cast<TaskStatus_t *>(heap_caps_calloc(num_of_tasks, sizeof(TaskStatus_t), MALLOC_CAP_SPIRAM));
    if (!task_array)
    {
        ESP_LOGE(COMMAND_TAG, "Memory allocation for task list failed.");
        return;
    }

    num_of_tasks = uxTaskGetSystemState(task_array, num_of_tasks, NULL);

    ESP_LOGI(COMMAND_TAG, "Name            Number  Priority        Runtime   StackWaterMark");
    for (int i = 0; i < num_of_tasks; i++)
    {
        ESP_LOGI(COMMAND_TAG, "%-16s   %4d   %6d   %12lu  %15lu", task_array[i].pcTaskName, task_array[i].xTaskNumber,
                 task_array[i].uxCurrentPriority, task_array[i].ulRunTimeCounter, task_array[i].usStackHighWaterMark);
    }
    free(task_array);
}

static void cpu_dump_cli_handler()
{
    auto buf = reinterpret_cast<char *>(heap_caps_calloc(1, 2 * 1024, MALLOC_CAP_SPIRAM));
    if (!buf)
    {
        ESP_LOGE(COMMAND_TAG, "Memory allocation for cpu dump failed.");
        return;
    }
    vTaskGetRunTimeStats(buf);
    ESP_LOGI(COMMAND_TAG, "Run Time Stats:%s", buf);
    free(buf);
}

static void mem_dump_cli_handler()
{
    ESP_LOGI(COMMAND_TAG, "Description           Internal    SPIRAM");
    ESP_LOGI(COMMAND_TAG, "Current Free Memory   %8s   %6s",
             esp32::string::stringify_size(heap_caps_get_free_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_SPIRAM), 1).c_str(),
             esp32::string::stringify_size(heap_caps_get_free_size(MALLOC_CAP_SPIRAM), 1).c_str());
    ESP_LOGI(COMMAND_TAG, "Largest Free Block    %8s   %6s",
             esp32::string::stringify_size(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), 1).c_str(),
             esp32::string::stringify_size(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), 1).c_str());
    ESP_LOGI(COMMAND_TAG, "Min. Ever Free Size   %8s   %6s",
             esp32::string::stringify_size(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), 1).c_str(),
             esp32::string::stringify_size(heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM), 1).c_str());

    if (heap_caps_check_integrity_all(true))
    {
        ESP_LOGI(COMMAND_TAG, "No memory integrity issues found");
    }
}

static void sock_dump_cli_handler()
{
    int i, ret, used_sockets = 0;

    struct sockaddr_in local_sock, peer_sock;
    socklen_t local_sock_len = sizeof(struct sockaddr_in), peer_sock_len = sizeof(struct sockaddr_in);
    char local_ip_addr[16], peer_ip_addr[16];
    unsigned int local_port, peer_port;

    int sock_type;
    socklen_t sock_type_len;

#define TOTAL_NUM_SOCKETS MEMP_NUM_NETCONN

    ESP_LOGI(COMMAND_TAG, "sock_fd   protocol   local_addr        local_port   peer_addr          peer_addr_port");
    for (i = LWIP_SOCKET_OFFSET; i < LWIP_SOCKET_OFFSET + TOTAL_NUM_SOCKETS; i++)
    {
        memset(&local_sock, 0, sizeof(struct sockaddr_in));
        memset(&peer_sock, 0, sizeof(struct sockaddr_in));
        local_sock_len = sizeof(struct sockaddr);
        peer_sock_len = sizeof(struct sockaddr);
        memset(local_ip_addr, 0, sizeof(local_ip_addr));
        memset(peer_ip_addr, 0, sizeof(peer_ip_addr));
        local_port = 0;
        peer_port = 0;
        sock_type = 0;
        sock_type_len = sizeof(int);

        ret = getsockname(i, (struct sockaddr *)&local_sock, &local_sock_len);
        if (ret >= 0)
        {
            std::string log;
            used_sockets++;
            inet_ntop(AF_INET, &local_sock.sin_addr, local_ip_addr, sizeof(local_ip_addr));
            local_port = ntohs(local_sock.sin_port);
            getsockopt(i, SOL_SOCKET, SO_TYPE, &sock_type, &sock_type_len);
            log = esp32::string::sprintf("%7d   %-8s   %-15s   %10d", sock_type,
                                         sock_type == SOCK_STREAM  ? "tcp"
                                         : sock_type == SOCK_DGRAM ? "udp"
                                                                   : "raw",
                                         local_ip_addr, local_port);

            ret = getpeername(i, (struct sockaddr *)&peer_sock, &peer_sock_len);
            if (ret >= 0)
            {
                inet_ntop(AF_INET, &peer_sock.sin_addr, peer_ip_addr, sizeof(peer_ip_addr));
                peer_port = ntohs(peer_sock.sin_port);
                log += esp32::string::sprintf("   %-15s   %15d", peer_ip_addr, peer_port);
            }

            ESP_LOGI(COMMAND_TAG, "%s", log.c_str());
        }
    }
    ESP_LOGI(COMMAND_TAG, "Remaining sockets: %d", TOTAL_NUM_SOCKETS - used_sockets);
}

void run_command(const std::string_view &command)
{
    esp_log_level_set(COMMAND_TAG, ESP_LOG_INFO);
    if (command == "up-time")
    {
        up_time_cli_handler();
    }
    else if (command == "mem-dump")
    {
        mem_dump_cli_handler();
    }
    else if (command == "task-dump")
    {
        task_dump_cli_handler();
    }
    else if (command == "cpu-dump")
    {
        cpu_dump_cli_handler();
    }
    else if (command == "sock-dump")
    {
        sock_dump_cli_handler();
    }
}
