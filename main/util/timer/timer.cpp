
#include "timer.h"
#include <functional>

namespace esp32::timer
{

timer::timer(const std::function<void()> &timeout_cb, const std::string &timer_name) : timeout_cb_(timeout_cb), name_(timer_name)
{
    assert(timeout_cb);

    esp_timer_create_args_t timer_args{};
    timer_args.callback = esp_timer_cb;
    timer_args.arg = this;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = name_.c_str();

    CHECK_THROW_ESP(esp_timer_create(&timer_args, &timer_handle_));
}

timer::~timer()
{
    // Ignore potential ESP_ERR_INVALID_STATE here to not throw exception.
    esp_timer_stop(timer_handle_);
    esp_timer_delete(timer_handle_);
}

void timer::esp_timer_cb(void *arg)
{
    auto p_this = reinterpret_cast<timer *>(arg);
    p_this->timeout_cb_();
}

} // namespace esp32::timer
