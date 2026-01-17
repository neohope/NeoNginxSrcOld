/*
 * Unix 时间处理函数。
 * 封装了 ngx_localtime (localtime_r 的线程安全封装) 等时间转换函数，
 * 确保在多线程或信号处理上下文中的安全性。
 */



#include <ngx_config.h>
#include <ngx_core.h>


void ngx_localtime(ngx_tm_t *tm)
{
#if (HAVE_LOCALTIME_R)
    time_t     now;

    now = ngx_time();
    localtime_r(&now, tm);

#else
    time_t     now;
    ngx_tm_t  *t;

    now = ngx_time();
    t = localtime(&now);
    *tm = *t;

#endif

    tm->ngx_tm_mon++;
    tm->ngx_tm_year += 1900;
}
