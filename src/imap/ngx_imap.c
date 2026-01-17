/*
 * IMAP 子系统入口与 imap 模块。
 * - 定义 imap {} 主配置块，创建 IMAP 监听端口
 * - 为 8110 等端口设置监听 socket 属性与连接处理回调
 * - 作为整个 IMAP 协议处理的入口，与 ngx_imap_handler 等协作
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_imap.h>


static char *ngx_imap_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


static ngx_command_t  ngx_imap_commands[] = {

    { ngx_string("imap"),
      NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_imap_block,
      0,
      0,
      NULL },

      ngx_null_command
};

    
static ngx_core_module_t  ngx_imap_module_ctx = {
    ngx_string("imap"),
    NULL,
    NULL
};  


ngx_module_t  ngx_imap_module = {
    NGX_MODULE, 
    &ngx_imap_module_ctx,                  /* module context */
    ngx_imap_commands,                     /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init module */
    NULL                                   /* init process */
};


static char *ngx_imap_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_listening_t  *ls;

    /* STUB */

    ls = ngx_listening_inet_stream_socket(cf, 0, 8110);
    if (ls == NULL) {
        return NGX_CONF_ERROR;
    }

    ls->backlog = -1;
    ls->addr_ntop = 1;
    ls->handler = ngx_imap_init_connection;
    ls->pool_size = 16384;
    /* ls->post_accept_timeout = 0; */
    ls->log = cf->cycle->new_log;

    /* */

    return NGX_CONF_OK;
}
