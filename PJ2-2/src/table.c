#include "table.h"
#include "hash_map.h"
#include <stdio.h>
#include <string.h>

void table_init(Table* table, const char* data_filename, const char* fsm_filename) {
    init_buffer_pool(data_filename, &table->data_pool);
    hash_table_init(fsm_filename, &table->fsm_pool, PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE);
}

void table_close(Table* table) {
    close_buffer_pool(&table->data_pool);
    hash_table_close(&table->fsm_pool);
}

off_t table_get_total_blocks(Table* table) {
    return table->data_pool.file.length / PAGE_SIZE;
}

short table_block_get_total_items(Table* table, off_t block_addr) {
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    short n_items = block->n_items;
    release(&table->data_pool, block_addr);
    return n_items;
}

void table_read(Table* table, RID rid, ItemPtr dest) {
    off_t block_addr = get_rid_block_addr(rid);
    short idx = get_rid_idx(rid);
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    ItemPtr itemptr = get_item(block, idx);
    short item_size = get_item_id_size(get_item_id(block, idx));
    if (sizeof(dest) >= sizeof(itemptr)) {
        for (short i = 0; i < item_size; i++) {
            dest[i] = itemptr[i];
        }
    }
    release(&table->data_pool, block_addr);
    return;
}

RID table_insert(Table* table, ItemPtr src, short size) {
    Block* block;
    short block_size, rid_idx;
    RID rid;
    off_t rid_block_addr = hash_table_pop_lower_bound(&table->fsm_pool, size);
    if (rid_block_addr != -1) {//have size
        block = (Block*)get_page(&table->data_pool, rid_block_addr);
        block_size = block->tail_ptr - block->head_ptr;
        rid_idx = new_item(block, src, size);
        if (rid_idx != -1) {
            hash_table_pop(&table->fsm_pool, block_size, rid_block_addr);
            get_rid_block_addr(rid) = rid_block_addr;
            get_rid_idx(rid) = rid_idx;
            hash_table_insert(&table->fsm_pool, block->tail_ptr - block->head_ptr, rid_block_addr);
            release(&table->data_pool, rid_block_addr);
            return rid;
        }
        
    }
    release(&table->data_pool, rid_block_addr);
    // end_insert a free block
    rid_block_addr = table->data_pool.file.length;
    block = (Block*)get_page(&table->data_pool, rid_block_addr);
    init_block(block);
    table->data_pool.file.length += PAGE_SIZE;
    rid_idx = new_item(block, src, size);
    get_rid_block_addr(rid) = rid_block_addr;
    get_rid_idx(rid) = rid_idx;
    // insert block addr at hashmap
    hash_table_insert(&table->fsm_pool, block->tail_ptr - block->head_ptr, rid_block_addr);
    release(&table->data_pool, rid_block_addr);
    return rid;
}

void table_delete(Table* table, RID rid) {
    off_t block_addr = get_rid_block_addr(rid);
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    hash_table_pop(&table->fsm_pool, block->tail_ptr - block->head_ptr, block_addr);
    delete_item(block, get_rid_idx(rid));
    hash_table_insert(&table->fsm_pool, block->tail_ptr - block->head_ptr, block_addr);
    release(&table->data_pool, block_addr);
    return;
}

void print_rid(RID rid) {
    printf("RID(" FORMAT_OFF_T ", %d)", get_rid_block_addr(rid), get_rid_idx(rid));
}


/*
void print_table(Table* table, printer_t printer) {
    printf("\n---------------TABLE---------------\n");
    off_t i, total = table_get_total_blocks(table);
    off_t block_addr;
    Block* block;
    for (i = 0; i < total; ++i) {
        block_addr = i * PAGE_SIZE;
        block = (Block*)get_page(&table->data_pool, block_addr);
        printf("[" FORMAT_OFF_T "]\n", block_addr);
        print_block(block, printer);
        release(&table->data_pool, block_addr);
    }
    printf("***********************************\n");
    print_hash_table(&table->fsm_pool);
    printf("-----------------------------------\n\n");
}

void analyze_table(Table* table) {
    block_stat_t stat, curr;
    off_t i, total = table_get_total_blocks(table);
    off_t block_addr;
    Block* block;
    stat.empty_item_ids = 0;
    stat.total_item_ids = 0;
    stat.available_space = 0;
    for (i = 0; i < total; ++i) {
        block_addr = i * PAGE_SIZE;
        block = (Block*)get_page(&table->data_pool, block_addr);
        analyze_block(block, &curr);
        release(&table->data_pool, block_addr);
        accumulate_stat_info(&stat, &curr);
    }
    
    printf("++++++++++ANALYSIS++++++++++\n");
    printf("total blocks: " FORMAT_OFF_T "\n", total);
    total *= PAGE_SIZE;
    printf("total size: " FORMAT_OFF_T "\n", total);
    printf("occupancy: %.4f\n", 1. - 1. * stat.available_space / total);
    printf("ItemID occupancy: %.4f\n", 1. - 1. * stat.empty_item_ids / stat.total_item_ids);
    printf("++++++++++++++++++++++++++++\n\n");
}
*/
