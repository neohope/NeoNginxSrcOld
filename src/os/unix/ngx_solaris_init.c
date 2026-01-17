/*
 * Solaris 平台初始化与 sendfilev 封装。
 * - 使用 sysinfo 读取系统名、版本等基础信息
 * - 填充 ngx_os_io，选择 sendfilev 或 writev_chain 作为发送接口
 * - 最终调用 ngx_posix_init 完成通用 POSIX 初始化
 */


#include <ngx_config.h>
#include <ngx_core.h>


char ngx_solaris_sysname[20];
char ngx_solaris_release[10];
char ngx_solaris_version[50];


ngx_os_io_t ngx_os_io = {
    ngx_unix_recv,
    ngx_readv_chain,
    ngx_unix_send,
#if (HAVE_SENDFILE)
    ngx_solaris_sendfilev_chain,
    NGX_IO_SENDFILE
#else
    ngx_writev_chain,
    0
#endif
};


ngx_int_t ngx_os_init(ngx_log_t *log)
{
    if (sysinfo(SI_SYSNAME, ngx_solaris_sysname, sizeof(ngx_solaris_sysname))
                                                                         == -1)
    {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      "sysinfo(SI_SYSNAME) failed");
        return NGX_ERROR;
    }

    if (sysinfo(SI_RELEASE, ngx_solaris_release, sizeof(ngx_solaris_release))
                                                                         == -1)
    {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      "sysinfo(SI_RELEASE) failed");
        return NGX_ERROR;
    }

    if (sysinfo(SI_VERSION, ngx_solaris_version, sizeof(ngx_solaris_version))
                                                                         == -1)
    {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      "sysinfo(SI_SYSNAME) failed");
        return NGX_ERROR;
    }

    return ngx_posix_init(log);
}


void ngx_os_status(ngx_log_t *log)
{

    ngx_log_error(NGX_LOG_INFO, log, 0, "OS: %s %s",
                  ngx_solaris_sysname, ngx_solaris_release);

    ngx_log_error(NGX_LOG_INFO, log, 0, "version: %s",
                  ngx_solaris_version);

    ngx_posix_status(log);
}
