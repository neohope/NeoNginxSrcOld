/*
 * UNIX 平台错误字符串封装。
 * - 对不同实现的 strerror_r 进行统一封装
 * - 返回实际拷贝到 errstr 中的错误信息长度
 * - 供日志子系统将 errno 转换为可读字符串
 */


#include <ngx_config.h>
#include <ngx_core.h>


#if (HAVE_STRERROR_R)

ngx_int_t ngx_strerror_r(int err, char *errstr, size_t size)
{
    size_t  len;

    if (size == 0) {
        return 0;
    }

    errstr[0] = '\0';

    strerror_r(err, errstr, size);

    for (len = 0; len < size; len++) {
        if (errstr[len] == '\0') {
            break;
        }
    }

    return len;
}

#elif (HAVE_GNU_STRERROR_R)

/* Linux strerror_r() */

ngx_int_t ngx_strerror_r(int err, char *errstr, size_t size)
{
    char    *str;
    size_t   len;

    if (size == 0) {
        return 0;
    }

    errstr[0] = '\0';

    str = strerror_r(err, errstr, size);

    if (str != errstr) {
        return ngx_cpystrn(errstr, str, size) - (u_char *) errstr;
    }

    for (len = 0; len < size; len++) {
        if (errstr[len] == '\0') {
            break;
        }
    }

    return len;
}

#endif
