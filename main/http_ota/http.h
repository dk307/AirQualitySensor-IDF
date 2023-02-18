#ifndef _HTTP_H_
#define _HTTP_H_

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C"
{
#endif
    void http_init(void);
    void http_start_webserver(httpd_handle_t *p_server);
    void http_stop_webserver(httpd_handle_t *p_server);
#ifdef __cplusplus
}
#endif

#endif