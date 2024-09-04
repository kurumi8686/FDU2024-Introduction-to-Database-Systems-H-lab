#include"str.h"
#include "table.h"
#include<string.h>
#include<stdlib.h>

void read_string(Table* table, RID rid, StringRecord* record) {
    StringChunk* str_chunk = &record->chunk;
    table_read(table, rid, (ItemPtr)str_chunk);
    record->idx = 0;
    record->chunk = *str_chunk;
    return;
}

int has_next_char(StringRecord* record) {
    StringRecord strrecord = *record;
    StringChunk* strchunk = &record->chunk;
    short size = get_str_chunk_size(strchunk);
    short real_size = size - sizeof(RID) - sizeof(short);
    off_t rid_block_addr = get_rid_block_addr(get_str_chunk_rid(strchunk));
    return (strrecord.idx + 1 == real_size && rid_block_addr == -1) ? 0 : 1;
}

char next_char(Table* table, StringRecord* record) {
    StringChunk* strchunk = &record->chunk;
    char* string = get_str_chunk_data_ptr(strchunk);
    short strsize = get_str_chunk_size(strchunk) - sizeof(RID) - sizeof(short);
    RID next_rid = get_str_chunk_rid(strchunk);
    off_t rid_addr = get_rid_block_addr(next_rid); 
    short rid_idx = get_rid_idx(next_rid);
    if (record->idx == strsize - 1) {
        record->idx = 0;
        if (rid_addr == -1) return 0;
        Block* block = (Block*)get_page(&table->data_pool, rid_addr);
        release(&table->data_pool, rid_addr);
        StringChunk* next_chunk = (StringChunk*)get_item(block, rid_idx);
        record->chunk = *next_chunk;
        char* next_str = get_str_chunk_data_ptr(next_chunk);
        return next_str[0];
    }
    else {
        return string[++record->idx];
    }
}

int compare_string_record(Table* table, const StringRecord* a, const StringRecord* b) {
    StringRecord record_a = *a, record_b = *b;
    record_a.idx = 0; record_b.idx = 0;
    char* string_a = get_str_chunk_data_ptr(&record_a.chunk);
    char* string_b = get_str_chunk_data_ptr(&record_b.chunk);
    if (string_a[0] > string_b[0])  return 1; 
    if (string_a[0] < string_b[0])  return -1; 
    while (1) {
        if (has_next_char(&record_a) == 1 && has_next_char(&record_b) == 1) {
            string_a[0] = next_char(table, &record_a);
            string_b[0] = next_char(table, &record_b);
            if (string_a[0] && !string_b[0]) return 1;
            if (!string_a[0] &&  string_b[0]) return -1;
            if (!string_a[0] && !string_b[0]) return 0;

            if (string_a[0] > string_b[0]) return 1;
            else if (string_a[0] < string_b[0]) return -1;
        }
        else {
            if (has_next_char(&record_a)) return 1;
            else if (has_next_char(&record_b)) return -1;
            else return 0;
        }
    }
}

RID write_string(Table* table, const char* data, off_t size) {
    short real_size = STR_CHUNK_MAX_SIZE - sizeof(RID) - sizeof(short);
    short mask_size = 20, i = 0, j = 0;
    short end_size = (size % mask_size == 0) ? mask_size : (short)size % real_size;
    short nums = (size % mask_size == 0) ? (short)size / real_size - 1 : (short)size / real_size;

    StringChunk* chunk = (StringChunk*)malloc(sizeof(StringChunk));
    get_rid_block_addr(get_str_chunk_rid(chunk)) = -1;
    get_rid_idx(get_str_chunk_rid(chunk)) = 0;
    get_str_chunk_size(chunk) = calc_str_chunk_size(end_size);
    for (i = 0; i < end_size; i++) {
        chunk->data[sizeof(RID) + sizeof(short) + i] = data[nums * real_size + i];
    }
    RID rid = table_insert(table, (ItemPtr)chunk->data, end_size + sizeof(RID) + sizeof(short));
    for (i = nums - 1; i > 0; i--) {
        get_rid_block_addr(get_str_chunk_rid(chunk)) = get_rid_block_addr(rid);
        get_rid_idx(get_str_chunk_rid(chunk)) = get_rid_idx(rid);
        get_str_chunk_size(chunk) = calc_str_chunk_size(real_size);
        for (j = 0; j < real_size; j++) {
            chunk->data[sizeof(RID) + sizeof(short) + j] = data[i * real_size + j];
        }
        rid = table_insert(table, chunk->data, STR_CHUNK_MAX_SIZE);
    }
    if (size > real_size) {
        get_rid_block_addr(get_str_chunk_rid(chunk)) = get_rid_block_addr(rid);
        get_rid_idx(get_str_chunk_rid(chunk)) = get_rid_idx(rid);
        get_str_chunk_size(chunk) = calc_str_chunk_size(real_size);
        for (i = 0; i < real_size; i++) {
            chunk->data[sizeof(RID) + sizeof(short) + i] = data[i];
        }
        rid = table_insert(table, chunk->data, STR_CHUNK_MAX_SIZE);
    }
    return rid;
}

void delete_string(Table* table, RID rid) {
    off_t rid_addr = get_rid_block_addr(rid);
    Block* block = (Block*)get_page(&table->data_pool, rid_addr);
    short rid_idx = get_rid_idx(rid);
    StringChunk* str_chunk = (StringChunk*)get_item(block, rid_idx);
    release(&table->data_pool, rid_addr);
    RID nxt_rid = get_str_chunk_rid(str_chunk);
    rid_addr = get_rid_block_addr(nxt_rid);
    rid_idx = get_rid_idx(nxt_rid);
    RID cur_rid = nxt_rid;
    table_delete(table, rid);
    while (rid_addr != -1) {
        block = (Block*)get_page(&table->data_pool, rid_addr);
        str_chunk = (StringChunk*)get_item(block, rid_idx);
        nxt_rid = get_str_chunk_rid(str_chunk);
        table_delete(table, cur_rid);
        rid_addr = get_rid_block_addr(nxt_rid);
        rid_idx = get_rid_idx(nxt_rid);
        cur_rid = nxt_rid;
        release(&table->data_pool, rid_addr);
    }
    return;
}

size_t load_string(Table* table, const StringRecord* record, char* dest, size_t max_size) {
    StringRecord strrecord = *record;
    strrecord.idx = 0;
    size_t cur_size = 1;
    char* chunk_data_ptr = get_str_chunk_data_ptr(&strrecord.chunk);
    dest[0] = chunk_data_ptr[0];
    while (cur_size < max_size) {
        if (has_next_char(&strrecord)) dest[cur_size++] = next_char(table, &strrecord);
        else return cur_size;
    }
    return cur_size;
}

/*
void print_string(Table* table, const StringRecord* record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec)) {
        printf("%c", next_char(table, &rec));
    }
    printf("\"");
}


void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk* chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk), i;
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk));
    printf(", %d, \"", size);
    for (i = 0; i < size; i++) {
        printf("%c", get_str_chunk_data_ptr(chunk)[i]);
    }
    printf("\")");
}
*/