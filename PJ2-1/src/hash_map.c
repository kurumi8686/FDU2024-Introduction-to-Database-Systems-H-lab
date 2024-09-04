#include "hash_map.h"
#include <stdio.h>
#include <string.h>

void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if (pool->file.length == 0) {
        short i;
        for (i = 0; i < n_directory_blocks + 2; i++) {
            get_page(pool, i * PAGE_SIZE); // use as initialize
            release(pool, i * PAGE_SIZE); // after get_page, must release
        }
        HashMapControlBlock* ctr_block = (HashMapControlBlock*)get_page(pool, 0);
        HashMapDirectoryBlock* dir_block;
        HashMapBlock* block;
        ctr_block->n_blocks = n_directory_blocks + 2;  // ctrl+dir+block
        ctr_block->free_block_head = (n_directory_blocks + 1) * PAGE_SIZE;
        ctr_block->n_directory_blocks = n_directory_blocks;
        ctr_block->max_size = n_directory_blocks * HASH_MAP_DIR_BLOCK_SIZE;

        for (i = 1; i < n_directory_blocks + 1; i++) {
            dir_block = (HashMapDirectoryBlock*)get_page(pool, i * PAGE_SIZE);
            memset(dir_block->directory, 0, HASH_MAP_DIR_BLOCK_SIZE * sizeof(*(dir_block->directory)));
            release(pool, i * PAGE_SIZE);
        }

        // HashMapBlock initialization
        block = (HashMapBlock*)get_page(pool, ctr_block->free_block_head);
        block->n_items = 0;
        block->next = 0;
        memset(block->table, -1, HASH_MAP_BLOCK_SIZE * sizeof(*(block->table)));
        release(pool, ctr_block->free_block_head);
        release(pool, 0);
    }
    return;
}

void hash_table_close(BufferPool* pool) {
    close_buffer_pool(pool);
}
// mark addr's block has free space of size
void hash_table_insert(BufferPool* pool, short size, off_t addr) {
    HashMapControlBlock* ctr_block = (HashMapControlBlock*)get_page(pool, 0);
    if (size < ctr_block->max_size) {
        HashMapBlock* free_block;
        HashMapBlock* target_block;
        HashMapBlock* end_block;
        short i = 0, flag = 0;
        HashMapDirectoryBlock* target_dir_block = (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE); // directory id
        off_t target_block_addr = target_dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE]; // block id
        if (target_block_addr != 0) {
            target_block = (HashMapBlock*)get_page(pool, target_block_addr);
            while (target_block->n_items >= HASH_MAP_BLOCK_SIZE && target_block->next != 0) {
                for (i = 0; i < HASH_MAP_BLOCK_SIZE; i++) {
                    if (addr == target_block->table[i]) { flag = 1; break; }
                }
                release(pool, target_block_addr);
                target_block_addr = target_block->next;
                target_block = (HashMapBlock*)get_page(pool, target_block_addr);
            }
            if (flag == 1) {
                release(pool, target_block_addr);
                release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
                release(pool, 0);
                return;
            }
            if (target_block->n_items < HASH_MAP_BLOCK_SIZE) {
                for (i = 0; i < HASH_MAP_BLOCK_SIZE; i++) {
                    if (target_block->table[i] == -1) {
                        target_block->table[i] = addr;
                        target_block->n_items++;
                        break;
                    }
                }
            }
            else {
                off_t free_addr = ctr_block->free_block_head;
                off_t end_addr = ctr_block->n_blocks * PAGE_SIZE;
                // if find a free block
                if (free_addr != 0) {
                    free_block = (HashMapBlock*)get_page(pool, free_addr);
                    ctr_block->free_block_head = free_block->next;
                    target_block->next = free_addr;
                    free_block->n_items++;
                    free_block->next = 0;
                    free_block->table[0] = addr;
                    release(pool, free_addr);
                }
                // make a new block
                else {
                    end_block = (HashMapBlock*)get_page(pool, end_addr);
                    ctr_block->n_blocks++;
                    target_block->next = end_addr;
                    end_block->n_items = 1;
                    end_block->next = 0;
                    end_block->table[0] = addr;
                    for (i = 1; i < HASH_MAP_BLOCK_SIZE; i++) {
                        end_block->table[i] = -1;
                    }
                    release(pool, end_addr);
                }
            }
            release(pool, target_block_addr);
        }
        // target block addr == 0
        else {
            off_t free_addr = ctr_block->free_block_head;
            off_t end_addr = ctr_block->n_blocks * PAGE_SIZE;
            if (free_addr != 0) {
                free_block = (HashMapBlock*)get_page(pool, free_addr);
                target_dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = free_addr;
                ctr_block->free_block_head = free_block->next;
                free_block->n_items++;
                free_block->next = 0;
                free_block->table[0] = addr;
                release(pool, free_addr);
            }
            else {
                target_dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = end_addr;
                target_block = (HashMapBlock*)get_page(pool, end_addr);
                ctr_block->n_blocks++;
                target_block->n_items = 1;
                target_block->next = 0;
                target_block->table[0] = addr;
                for (i = 1; i < HASH_MAP_BLOCK_SIZE; i++) {
                    target_block->table[i] = -1;
                }
                release(pool, end_addr);
            }
        }
        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    return;
}
// get space larger than size
off_t hash_table_pop_lower_bound(BufferPool* pool, short size) {
    HashMapControlBlock* ctr_block = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* target_dir_block;
    off_t block_addr, end_addr;
    HashMapBlock* hashmapblock;
    short i = 0;
    off_t tt = HASH_MAP_BLOCK_SIZE;
    while (size < ctr_block->max_size) {
        target_dir_block = (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        block_addr = target_dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
        if (block_addr != 0) {
            while (block_addr != 0) {
                hashmapblock = (HashMapBlock*)get_page(pool, block_addr);
                release(pool, block_addr);
                block_addr = hashmapblock->next;
            }
            if (hashmapblock->n_items != tt) {
                for (i = 0; i < hashmapblock->n_items + 1; i++) {
                    if (hashmapblock->table[i] == -1) { tt = i; break; }
                }
            }
            end_addr = hashmapblock->table[tt - 1];
            hash_table_pop(pool, size, end_addr);
            release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            release(pool, 0);
            return end_addr;
        }
        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        size++;
    }
    release(pool, 0);
    return -1;
}

void hash_table_pop(BufferPool* pool, short size, off_t addr) {
    HashMapControlBlock* ctr_block = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    off_t block_addr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];    
    if (block_addr != 0) {
        HashMapBlock* block = (HashMapBlock*)get_page(pool, block_addr);
        if (block->n_items == 1) {
            block->n_items = 0;
            block->table[0] = -1;
            dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = 0;
            block->next = ctr_block->free_block_head;
            ctr_block->free_block_head = block_addr;
        }
        else {
            off_t next_addr, last_addr;
            do {
                block = (HashMapBlock*)get_page(pool, block_addr);
                next_addr = block->next;
                for (short j = 0; j < block->n_items; ++j) {
                    if (block->table[j] == addr) {
                        block->table[j] = -1;
                        if (j != block->n_items - 1) {
                            block->table[j] = block->table[block->n_items - 1];
                            block->table[block->n_items - 1] = -1;
                        }
                        block->n_items--;
                        if (block->n_items == 0) {
                            HashMapBlock* last_block = (HashMapBlock*)get_page(pool, last_addr);
                            last_block->next = next_addr;
                            block->next = ctr_block->free_block_head;
                            ctr_block->free_block_head = block_addr;
                            release(pool, last_addr);
                        }
                        release(pool, block_addr);
                        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
                        release(pool, 0);
                        return;
                    }
                }
                release(pool, block_addr);
                last_addr = block_addr;
                block_addr = next_addr;
            } while (block->next != 0);
        }
        release(pool, block_addr);
    }
    release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    release(pool, 0);
    return;
}

/*
void print_hash_table(BufferPool* pool) {
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block;
    off_t block_addr, next_addr;
    HashMapBlock* block;
    int i, j;
    printf("----------HASH TABLE----------\n");
    for (i = 0; i < ctrl->max_size; ++i) {
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        if (dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE] != 0) {
            printf("%d:", i);
            block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];
            while (block_addr != 0) {
                block = (HashMapBlock*)get_page(pool, block_addr);
                printf("  [" FORMAT_OFF_T "]", block_addr);
                printf("{");
                for (j = 0; j < HASH_MAP_BLOCK_SIZE; ++j) {
                    if (j != 0) {
                        printf(", ");
                    }
                    printf(FORMAT_OFF_T, block->table[j]);
                }
                printf("}");
                next_addr = block->next;
                release(pool, block_addr);
                block_addr = next_addr;
            }
            printf("\n");
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    printf("------------------------------\n");
}
*/