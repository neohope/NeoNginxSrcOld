/*
 * Nginx 链表（ngx_list_t）实现。
 * - 底层是若干连续数组块组成的单向链表
 * - 在保持元素地址稳定的同时支持尾部追加
 * - 常用于保存请求头、开放文件等小对象集合
 */


#include <ngx_config.h>
#include <ngx_core.h>


void *ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last;

    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */

        if (!(last = ngx_palloc(l->pool, sizeof(ngx_list_part_t)))) {
            return NULL;
        }

        if (!(last->elts = ngx_palloc(l->pool, l->nalloc * l->size))) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}
