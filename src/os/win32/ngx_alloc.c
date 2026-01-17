/*
 * Nginx 内存分配函数的 Windows 实现。
 * 包含 ngx_alloc (malloc 封装) 和 ngx_calloc (分配并清零) 函数，
 * 提供基本的内存管理并集成了日志记录功能。
 */

#include <ngx_config.h>
#include <ngx_core.h>


int ngx_pagesize;


void *ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    if (!(p = malloc(size))) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "malloc() " SIZE_T_FMT " bytes failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "malloc: " PTR_FMT ":" SIZE_T_FMT, p, size);

    return p;
}


void *ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}
