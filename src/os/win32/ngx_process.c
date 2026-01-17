/*
 * Windows 进程管理存根 (Stub)。
 * 目前仅包含一个空的 ngx_execute 函数，表明在早期版本中
 * Windows 下的进程执行功能尚未完全实现或与 Unix 版本不同。
 */


#include <ngx_config.h>
#include <ngx_core.h>


ngx_pid_t ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx)
{
    return /* STUB */ 0;
}
