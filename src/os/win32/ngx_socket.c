/*
 * Windows Socket 辅助函数。
 * 实现了设置 Socket 阻塞/非阻塞模式的函数 (ngx_nonblocking, ngx_blocking)，
 * 是对 ioctlsocket 的封装。
 */


#include <ngx_config.h>
#include <ngx_core.h>


int ngx_nonblocking(ngx_socket_t s)
{
    unsigned long  nb = 1;

    return ioctlsocket(s, FIONBIO, &nb);
}


int ngx_blocking(ngx_socket_t s)
{
    unsigned long  nb = 0;

    return ioctlsocket(s, FIONBIO, &nb);
}
