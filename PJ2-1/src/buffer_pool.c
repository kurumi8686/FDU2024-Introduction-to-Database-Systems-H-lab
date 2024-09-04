#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

// open filename and connect pool as its buffer-pool
void init_buffer_pool(const char* filename, BufferPool* pool) {
    open_file(&pool->file, filename);
    short i = 0;
    for (i; i < CACHE_PAGE; i++) {
        read_page(&pool->pages[i], &pool->file, PAGE_SIZE * i);
        pool->addrs[i] = -1;
        pool->cnt[i] = 0;
        pool->ref[i] = 0;
        pool->avail[i] = 1;
    }
}
// close this buffer-pool and write out all
void close_buffer_pool(BufferPool* pool) {
    short i = 0;
    for (i; i < CACHE_PAGE; i++) {
        write_page(&pool->pages[i], &pool->file, pool->addrs[i]);
    }
    close_file(&pool->file);
}
// get the page where at addr, and lock it
Page* get_page(BufferPool* pool, off_t addr) {
    short i = 0, t = -1, tt = -1;
    size_t not_full = 0;
    for (i; i < CACHE_PAGE; i++) {
        // if has page available, remind it
        if (pool->avail[i] == 1) {
            tt = i;
        }
        // not full and find
        if (pool->addrs[i] == addr) {
            pool->ref[i]++;
            pool->cnt[i] = 0;
            pool->avail[i] = 0;
            t = i;
            break;
        }
        pool->cnt[i]++;
        not_full += pool->avail[i];
    }

    // full or not find : LRU
    if (t == -1) {
        // full, means all not available
        if (not_full == 0) { 
            size_t max_cnt = 0;
            // find the max cnt(LRU), replace it
            for (i = 0; i < CACHE_PAGE; i++) {
                if (pool->cnt[i] > max_cnt) {
                    max_cnt = pool->cnt[i];
                    t = i;
                }
            }
            release(pool, pool->addrs[t]);
            pool->addrs[t] = addr;
            pool->ref[t]++;
            pool->cnt[t] = 0;
            pool->avail[t] = 0;
            return &pool->pages[t];
        }
        // not find and has page available
        else {
            read_page(&pool->pages[tt], &pool->file, addr);
            pool->addrs[tt] = addr;
            pool->ref[tt]++;
            pool->cnt[tt] = 0;
            pool->avail[tt] = 0;
            return &pool->pages[tt];
        }
    }
    // this cachepage can be used, return it.
    else {
        return &pool->pages[t];
    }
}

void release(BufferPool* pool, off_t addr) {
    short i = 0;
    for (i; i < CACHE_PAGE; i++) {
        if (pool->addrs[i] == addr) {
            write_page(&pool->pages[i], &pool->file, pool->addrs[i]);
            pool->ref[i]++;
            pool->avail[i] = 1;
            return;
        }
    }
}
