/*
 * Nginx slab 分配器的占位实现。
 * - slab 分配器用于在共享内存中按固定大小块分配对象
 * - 早期版本尚未完整实现，这里仅保留接口占位
 */


void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size)
{
   return NULL;
}
