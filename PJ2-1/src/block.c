#include "block.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void init_block(Block* block) {
    block->n_items = 0;
    block->head_ptr = (short)(block->data - (char*)block);
    block->tail_ptr = (short)sizeof(Block);
    memset(block->data, 0, PAGE_AVAIL_SIZE * sizeof(block->data[0]));
}
// return the start addr of the number idx's item
ItemPtr get_item(Block* block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        printf("get item error: idx is out of range\n");
        return NULL;
    }
    ItemID item_id = get_item_id(block, idx);
    if (get_item_id_availability(item_id)) {
        printf("get item error: item_id is not used\n");
        return NULL;
    }
    short offset = get_item_id_offset(item_id);
    return (char*)block + offset;
}
// item is start addr, item_size is size, add it
short new_item(Block* block, ItemPtr item, short item_size) {
    // space insufficient: itemID points to item, all have been written into block
    // first dont consider existing item, change page
    if (block->tail_ptr - block->head_ptr < item_size + sizeof(ItemID)) { return -1; }
    // space sufficient, add new item
    else {
        short i, idx = block->n_items++;
        block->head_ptr += sizeof(ItemID);
        block->tail_ptr -= item_size;
        get_item_id(block, idx) = compose_item_id(0, block->tail_ptr, item_size);
        short real_offset = get_item_id_offset(get_item_id(block, idx)) - 3 * sizeof(short);
        for (i = 0; i < item_size; i++) { block->data[real_offset + i] = item[i]; }
        return idx;
    }
}
// delete the number idx's item
void delete_item(Block* block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        return;
    }
    else {
        ItemID item_id = get_item_id(block, idx);
        short size = get_item_id_size(item_id);
        ItemID offset = get_item_id_offset(item_id);
        short avail = get_item_id_availability(item_id);
        // only when not availability
        if (avail == 1) { return; }
        else {
            short i;
            if (size == 0) {
                for (i = 0; i < sizeof(ItemID); i++) {
                    block->data[block->head_ptr - 3 * sizeof(short) - i - 1] = 0;
                }
                block->head_ptr -= sizeof(ItemID);
                block->n_items--;
            }
            else {
                ItemID other_item_id, other_offset;
                short other_avail, other_size;
                short num = block->n_items;
                for (i = 0; i < num; i++) {
                    other_item_id = get_item_id(block, i);
                    other_offset = get_item_id_offset(other_item_id);
                    other_avail = get_item_id_availability(other_item_id);
                    other_size = get_item_id_size(other_item_id);
                    // only when not availability and before target
                    if (other_avail == 0 && other_offset < offset) {
                        get_item_id(block, i) = compose_item_id(0, other_offset + size, other_size);
                    }
                }
                ItemPtr itemptr = get_item(block, idx);
                short str_length = (short)itemptr - (short)block - block->tail_ptr;
                char* data_ptr = block->data + block->tail_ptr - 3 * sizeof(short);
                memmove(data_ptr + size, data_ptr, str_length);
                memset(data_ptr, 0, size);
                block->tail_ptr += size;
                if (idx == num - 1) {
                    for (i = 0; i < sizeof(ItemID); i++) {
                        block->data[block->head_ptr - 3 * sizeof(short) - i - 1] = 0;
                    }
                    block->head_ptr -= sizeof(ItemID);
                    block->n_items--;
                }
                else {
                    get_item_id(block, idx) = compose_item_id(1, 0, 0);
                }
            }
        }
        return;
    }
    return;
}

/*
void str_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    short i;
    printf("\"");
    for (i = 0; i < item_size; ++i) {
        printf("%c", item[i]);
    }
    printf("\"");
}
void print_block(Block* block, printer_t printer) {
    short i, availability, offset, size;
    ItemID item_id;
    ItemPtr item;
    printf("----------BLOCK----------\n");
    printf("total = %d\n", block->n_items);
    printf("head = %d\n", block->head_ptr);
    printf("tail = %d\n", block->tail_ptr);
    for (i = 0; i < block->n_items; ++i) {
        item_id = get_item_id(block, i);
        availability = get_item_id_availability(item_id);
        offset = get_item_id_offset(item_id);
        size = get_item_id_size(item_id);
        if (!availability) {
            item = get_item(block, i);
        }
        else {
            item = NULL;
        }
        printf("%10d%5d%10d%10d\t", i, availability, offset, size);
        printer(item, size);
        printf("\n");
    }
    printf("-------------------------\n");
}
void analyze_block(Block* block, block_stat_t* stat) {
    short i;
    stat->empty_item_ids = 0;
    stat->total_item_ids = block->n_items;
    for (i = 0; i < block->n_items; ++i) {
        if (get_item_id_availability(get_item_id(block, i))) {
            ++stat->empty_item_ids;
        }
    }
    stat->available_space = block->tail_ptr - block->head_ptr
        + stat->empty_item_ids * sizeof(ItemID);
}
void accumulate_stat_info(block_stat_t* stat, const block_stat_t* stat2) {
    stat->empty_item_ids += stat2->empty_item_ids;
    stat->total_item_ids += stat2->total_item_ids;
    stat->available_space += stat2->available_space;
}
void print_stat_info(const block_stat_t* stat) {
    printf("==========STAT==========\n");
    printf("empty_item_ids: " FORMAT_SIZE_T "\n", stat->empty_item_ids);
    printf("total_item_ids: " FORMAT_SIZE_T "\n", stat->total_item_ids);
    printf("available_space: " FORMAT_SIZE_T "\n", stat->available_space);
    printf("========================\n");
}
*/