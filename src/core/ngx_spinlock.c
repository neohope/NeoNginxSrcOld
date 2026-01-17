/*
 * Nginx 自旋锁实现。
 * - 基于原子操作 ngx_atomic_cmp_set 构造简单自旋锁
 * - 在多 CPU 上先自旋一段时间，随后主动让出调度
 * - 在线程场景下保护全局或共享数据结构
 */


#include <ngx_config.h>
#include <ngx_core.h>


void ngx_spinlock(ngx_atomic_t *lock, ngx_uint_t spin)
{

#if (NGX_HAVE_ATOMIC_OPS)

    ngx_uint_t  tries;

    tries = 0;

    for ( ;; ) {

        if (*lock) {
            if (ngx_ncpu > 1 && tries++ < spin) {
                continue;
            }

            ngx_sched_yield();

            tries = 0;

        } else {
            if (ngx_atomic_cmp_set(lock, 0, 1)) {
                return;
            }
        }
    }

#else

#if (NGX_THREADS)

#error ngx_spinlock() or ngx_atomic_cmp_set() are not defined !

#endif

#endif

}
