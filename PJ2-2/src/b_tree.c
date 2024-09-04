#include "b_tree.h"
#include "buffer_pool.h"
#include <stdio.h>

off_t InsertElement(BufferPool* pool, int insert_rid, off_t parent_addr, off_t target_addr, RID rid, int loc, int dst) {
    int k;
    BNode* target_node = (BNode*)get_page(pool, target_addr);
    BNode* parent_node = (BNode*)get_page(pool, parent_addr);
    if (insert_rid) { //insert rid
        k = (int)target_node->n - 1;
        while (k >= dst) { target_node->row_ptr[k + 1] = target_node->row_ptr[k--]; }
        target_node->row_ptr[dst] = rid;
        if (parent_addr != -1) { parent_node->row_ptr[loc] = target_node->row_ptr[0]; }
        target_node->n++;
    }
    else { //insert node
        if (target_node->child[0] == -1) {
            if (loc > 0 && parent_node->child[loc - 1] != -1) {
                BNode* child1 = (BNode*)get_page(pool, parent_node->child[loc - 1]);
                release(pool, parent_node->child[loc - 1]);
                child1->next = target_addr;
            }
            target_node->next = parent_node->child[loc];
        }
        k = (int)parent_node->n - 1;
        while (k >= loc) {
            parent_node->child[k + 1] = parent_node->child[k];
            parent_node->row_ptr[k + 1] = parent_node->row_ptr[k--];
        }
        parent_node->row_ptr[loc] = target_node->row_ptr[0];
        parent_node->child[loc] = target_addr;
        parent_node->n++;
    }
    release(pool, target_addr);
    release(pool, parent_addr);
    return target_addr;
}

off_t RemoveElement(BufferPool* pool, int delete_rid, off_t parent_addr, off_t target_addr, int dst, int loc) {
    int k, max;
    BNode* target_node = (BNode*)get_page(pool, target_addr);
    BNode* parent_node = (BNode*)get_page(pool, parent_addr);
    RID un; get_rid_block_addr(un) = -1; get_rid_idx(un) = 0;
    if (delete_rid) { //delete rid
        k = loc + 1;
        max = (int)target_node->n;
        while (k < max) { target_node->row_ptr[k - 1] = target_node->row_ptr[k++]; }
        target_node->row_ptr[target_node->n - 1] = un;
        parent_node->row_ptr[dst] = target_node->row_ptr[0];
        target_node->n--;
    }
    else { //delete node
        if (target_node->child[0] == -1 && dst > 0 && parent_node->child[dst - 1] != -1) {
            BNode* child1 = (BNode*)get_page(pool, parent_node->child[dst - 1]);
            release(pool, parent_node->child[dst - 1]);
            child1->next = parent_node->child[dst - 1];
        }
        k = dst + 1;
        max = (int)parent_node->n;
        while (k < max) {
            parent_node->child[k - 1] = parent_node->child[k];
            parent_node->row_ptr[k - 1] = parent_node->row_ptr[k++];
        }
        parent_node->child[parent_node->n - 1] = -1;
        parent_node->row_ptr[parent_node->n - 1] = un;
        parent_node->n--;
    }
    release(pool, parent_addr);
    release(pool, target_addr);
    return target_addr;
}

off_t MoveElement(BufferPool* pool, off_t src_addr, off_t dst_addr, off_t parent_addr, int i, int n, b_tree_row_row_cmp_t cmp) {
    RID key; off_t child_addr; int j = 0;
    RID un; get_rid_block_addr(un) = -1; get_rid_idx(un) = 0;
    BNode* parent_node, * ri;
    BNode* src_node = (BNode*)get_page(pool, src_addr);
    BNode* dst_node = (BNode*)get_page(pool, dst_addr);
    release(pool, dst_addr); release(pool, src_addr);
    off_t mr_addr, ml_addr;
    if (cmp(src_node->row_ptr[0], dst_node->row_ptr[0]) < 0) {
        if (src_node->child[0] != -1) {
            while (j++ < n) {
                src_node = (BNode*)get_page(pool, src_addr); release(pool, src_addr);
                child_addr = src_node->child[src_node->n - 1];
                RemoveElement(pool, 0, src_addr, child_addr, (int)src_node->n - 1, -1);
                InsertElement(pool, 0, dst_addr, child_addr, un, 0, -1);
            }
        }
        else {
            while (j++ < n) {
                src_node = (BNode*)get_page(pool, src_addr); release(pool, src_addr);
                key = src_node->row_ptr[src_node->n - 1];
                RemoveElement(pool, 1, parent_addr, src_addr, i, (int)src_node->n - 1);
                InsertElement(pool, 1, parent_addr, dst_addr, key, i + 1, 0);
            }
        }
        parent_node = (BNode*)get_page(pool, parent_addr);
        dst_node = (BNode*)get_page(pool, dst_addr);
        src_node = (BNode*)get_page(pool, src_addr);
        parent_node->row_ptr[i + 1] = dst_node->row_ptr[0];
        release(pool, src_addr); release(pool, dst_addr); release(pool, parent_addr);
        if (src_node->n > 0) {
            mr_addr = src_addr; BNode* node = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr);
            while (mr_addr != -1 && node->child[node->n - 1] != -1) {
                node = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr);
                mr_addr = node->child[node->n - 1];
            }
            ml_addr = dst_addr; node = (BNode*)get_page(pool, ml_addr); release(pool, ml_addr);
            while (ml_addr != -1 && node->child[0] != -1) {
                node = (BNode*)get_page(pool, ml_addr); release(pool, ml_addr);
                ml_addr = node->child[0];
            }
            ri = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr);
            ri->next = ml_addr;
        }
    }
    else {
        if (src_node->child[0] != -1) {
            while (j++ < n) {
                src_node = (BNode*)get_page(pool, src_addr); dst_node = (BNode*)get_page(pool, dst_addr);
                release(pool, dst_addr); release(pool, src_addr);
                child_addr = src_node->child[0];
                RemoveElement(pool, 0, src_addr, child_addr, 0, -1);
                src_node = (BNode*)get_page(pool, src_addr); dst_node = (BNode*)get_page(pool, dst_addr);
                release(pool, dst_addr); release(pool, src_addr);
                InsertElement(pool, 0, dst_addr, child_addr, un, (int)dst_node->n, -1);
            }
        }
        else {
            while (j++ < n) {
                src_node = (BNode*)get_page(pool, src_addr); dst_node = (BNode*)get_page(pool, dst_addr);
                release(pool, dst_addr); release(pool, src_addr);
                key = src_node->row_ptr[0];
                RemoveElement(pool, 1, parent_addr, src_addr, i, 0);
                src_node = (BNode*)get_page(pool, src_addr); dst_node = (BNode*)get_page(pool, dst_addr);
                release(pool, dst_addr); release(pool, src_addr);
                InsertElement(pool, 1, parent_addr, dst_addr, key, i - 1, (int)dst_node->n);
            }
        }
        parent_node = (BNode*)get_page(pool, parent_addr); src_node = (BNode*)get_page(pool, src_addr);
        parent_node->row_ptr[i] = src_node->row_ptr[0];
        release(pool, src_addr); release(pool, parent_addr);
        if (src_node->n > 0) {
            BNode* node;
            mr_addr = src_addr; node = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr);
            while (mr_addr != -1 && node->child[node->n - 1] != -1) {
                node = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr);
                mr_addr = node->child[node->n - 1];
            }
            ml_addr = dst_addr; node = (BNode*)get_page(pool, ml_addr); release(pool, ml_addr);
            while (ml_addr != -1 && node->child[0] != -1) {
                node = (BNode*)get_page(pool, ml_addr); release(pool, ml_addr);
                ml_addr = node->child[0];
            }
            ri = (BNode*)get_page(pool, mr_addr); release(pool, mr_addr); 
            ri->next = ml_addr;
        }
    }
    return parent_addr;
}

off_t SplitNode(BufferPool* pool, off_t parent_addr, off_t target_addr, int i) {
    short ii = 0; int j, k = 0, max; off_t newnode_addr;
    BNode* NewNode, * freenode, * target_node, * parent_node;
    RID un; get_rid_block_addr(un) = -1; get_rid_idx(un) = 0;
    BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0);
    if (ctrlblock->free_node_head != -1) {
        newnode_addr = ctrlblock->free_node_head; freenode = (BNode*)get_page(pool, ctrlblock->free_node_head);
        release(pool, ctrlblock->free_node_head); ctrlblock->free_node_head = freenode->next;
    }
    else { newnode_addr = PAGE_SIZE * ctrlblock->n_node; ctrlblock->n_node++; }
    release(pool, 0);
    NewNode = (BNode*)get_page(pool, newnode_addr); NewNode->n = 0; NewNode->next = -1;
    for (ii = 0; ii < 2 * DEGREE + 1; ii++) {
        NewNode->child[ii] = -1;
        get_rid_block_addr(NewNode->row_ptr[ii]) = -1;
        get_rid_idx(NewNode->row_ptr[ii]) = 0;
    }
    target_node = (BNode*)get_page(pool, target_addr);
    max = (int)target_node->n; j = max / 2;
    while (j < max) {
        if (target_node->child[0] != -1) { NewNode->child[k] = target_node->child[j]; target_node->child[j] = -1; }
        NewNode->row_ptr[k++] = target_node->row_ptr[j]; target_node->row_ptr[j++] = un; NewNode->n++; target_node->n--;
    }
    release(pool, newnode_addr); release(pool, target_addr);
    if (parent_addr != -1) { InsertElement(pool, 0, parent_addr, newnode_addr, un, i + 1, -1); return target_addr; }
    else {
        ctrlblock = (BCtrlBlock*)get_page(pool, 0);
        if (ctrlblock->free_node_head != -1) {
            parent_addr = ctrlblock->free_node_head; freenode = (BNode*)get_page(pool, ctrlblock->free_node_head);
            release(pool, ctrlblock->free_node_head); ctrlblock->free_node_head = freenode->next;
        }
        else { parent_addr = PAGE_SIZE * ctrlblock->n_node; ctrlblock->n_node++; }
        ctrlblock->root_node = parent_addr; release(pool, 0);
        parent_node = (BNode*)get_page(pool, parent_addr); parent_node->n = 0; parent_node->next = -1;
        for (ii = 0; ii < 2 * DEGREE + 1; ii++) {
            parent_node->child[ii] = -1;
            get_rid_block_addr(parent_node->row_ptr[ii]) = -1;
            get_rid_idx(parent_node->row_ptr[ii]) = 0;
        }
        release(pool, parent_addr);
        InsertElement(pool, 0, parent_addr, target_addr, un, 0, -1);
        InsertElement(pool, 0, parent_addr, newnode_addr, un, 1, -1);
        return parent_addr;
    }
    return target_addr;
}

off_t MergeNode(BufferPool* pool, off_t parent_addr, off_t target_addr, off_t sibor_addr, int i, b_tree_row_row_cmp_t cmp) {
    BNode* sibor_node = (BNode*)get_page(pool, sibor_addr);
    BNode* target_node = (BNode*)get_page(pool, target_addr);
    release(pool, target_addr); release(pool, sibor_addr);
    if (sibor_node->n > LIMIT_DEGREE_2) { MoveElement(pool, sibor_addr, target_addr, parent_addr, i, 1, cmp); }
    else {
        int lim = (int)target_node->n;
        MoveElement(pool, target_addr, sibor_addr, parent_addr, i, lim, cmp);
        RemoveElement(pool, 0, parent_addr, target_addr, i, -1);
        BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0); target_node = (BNode*)get_page(pool, target_addr);
        target_node->next = ctrlblock->free_node_head; target_node->n = 0; ctrlblock->free_node_head = target_addr;
        for (short i = 0; i < 2 * DEGREE + 1; i++) {
            target_node->child[i] = -1;
            get_rid_block_addr(target_node->row_ptr[i]) = -1; get_rid_idx(target_node->row_ptr[i]) = 0;
        }
        release(pool, target_addr); release(pool, 0);
    }
    return parent_addr;
}

RID search(BufferPool* pool, void* key, size_t size, b_tree_ptr_row_cmp_t cmp, off_t root_addr) {
    RID un; get_rid_block_addr(un) = -1; get_rid_idx(un) = 0;
    RID rid = un; int j = 0;
    if (root_addr != -1) {
        BNode* root = (BNode*)get_page(pool, root_addr); release(pool, root_addr);
        if (cmp(key, size, root->row_ptr[0]) < 0) { return un; }
        while (j < root->n && cmp(key, size, root->row_ptr[j]) >= 0) {
            if (cmp(key, size, root->row_ptr[j]) == 0) { return root->row_ptr[j]; } 
            else { j++; }
        }
        if (root->child[0] == -1) { if (cmp(key, size, root->row_ptr[j]) != 0 || j == root->n) { return un; } }
        else { if (cmp(key, size, root->row_ptr[j]) < 0 || j == root->n) { j--; } }
        if (root->child[0] == -1) { return root->row_ptr[j]; }
        else { rid = search(pool, key, size, cmp, root->child[j]); }
    }
    return rid;
}

off_t logic_insert(BufferPool* pool, off_t root_addr, RID rid, int i, off_t parent_addr, b_tree_row_row_cmp_t cmp) {
    int j = 0;
    off_t sibor_addr, tmp_addr;
    BNode* root = (BNode*)get_page(pool, root_addr);
    while (j < root->n && cmp(rid, root->row_ptr[j]) >= 0) { if (cmp(rid, root->row_ptr[j]) == 0) { return root_addr; } j++; }
    if (j != 0 && root->child[0] != -1) { j--; }
    if (root->child[0] == -1) { release(pool, root_addr); root_addr = InsertElement(pool, 1, parent_addr, root_addr, rid, i, j); }
    else {
        release(pool, root_addr); tmp_addr = logic_insert(pool, root->child[j], rid, j, root_addr, cmp);
        root = (BNode*)get_page(pool, root_addr);  root->child[j] = tmp_addr; release(pool, root_addr);
    }
    root = (BNode*)get_page(pool, root_addr); release(pool, root_addr);
    if (root->n > 2 * DEGREE) {
        if (parent_addr == -1) { root_addr = SplitNode(pool, parent_addr, root_addr, i); }
        else {
            sibor_addr = -1; BNode* Parent, * child_1 = NULL, * child_2 = NULL, * child_3 = NULL;
            Parent = (BNode*)get_page(pool, parent_addr);
            if (Parent->child[1] != -1) { child_1 = (BNode*)get_page(pool, Parent->child[1]); }
            if (Parent->child[i - 1] != -1) { child_2 = (BNode*)get_page(pool, Parent->child[i - 1]); }
            if (Parent->child[i + 1] != -1) { child_3 = (BNode*)get_page(pool, Parent->child[i + 1]); }
            release(pool, Parent->child[1]); release(pool, Parent->child[i - 1]); release(pool, Parent->child[i + 1]);
            if (i == 0) { if (child_1->n < 2 * DEGREE && Parent->child[1] != -1) sibor_addr = Parent->child[1]; }
            else if (child_2->n < 2 * DEGREE && Parent->child[i - 1] != -1) { sibor_addr = Parent->child[i - 1]; }
            else if (i + 1 < Parent->n && child_3->n < 2 * DEGREE && Parent->child[i + 1] != -1) { sibor_addr = Parent->child[i + 1]; }
            release(pool, parent_addr);
            if (sibor_addr != -1) { MoveElement(pool, root_addr, sibor_addr, parent_addr, i, 1, cmp); }
            else { root_addr = SplitNode(pool, parent_addr, root_addr, i); }
        }
    }
    if (parent_addr != -1) {
        BNode* parent_node = (BNode*)get_page(pool, parent_addr); root = (BNode*)get_page(pool, root_addr);
        release(pool, root_addr); parent_node->row_ptr[i] = root->row_ptr[0]; release(pool, parent_addr);
    }
    return root_addr;
}

off_t logic_delete(BufferPool* pool, off_t root_addr, RID rid, int i, off_t parent_addr, b_tree_row_row_cmp_t cmp) {
    int j = 0; off_t sibor_addr = -1, tmp_addr, child_addr; int ii = 0;
    BNode* tmp_node, * parent_node, * child_node;  BCtrlBlock* ctrlblock;
    if (root_addr != -1) {
        BNode* root = (BNode*)get_page(pool, root_addr); release(pool, root_addr); root = (BNode*)get_page(pool, root_addr);
        off_t test_rid_addr, test_row_addr;
        while (cmp(rid, root->row_ptr[j]) >= 0 && j < root->n) {
            test_rid_addr = get_rid_block_addr(rid); test_row_addr = get_rid_block_addr(root->row_ptr[j]);
            if (cmp(rid, root->row_ptr[j]) == 0) { break; }    j++;
        }
        if (root->child[0] == -1) { if (cmp(rid, root->row_ptr[j]) != 0 || j == root->n) { return root_addr; } }
        else { if (j == root->n || cmp(rid, root->row_ptr[j]) < 0) { j--; } }
        release(pool, root_addr);
        if (root->child[0] == -1) { root_addr = RemoveElement(pool, 1, parent_addr, root_addr, i, j); }
        else {
            tmp_addr = logic_delete(pool, root->child[j], rid, j, root_addr, cmp);
            root = (BNode*)get_page(pool, root_addr); root->child[j] = tmp_addr; release(pool, root_addr);
        }
        if ((parent_addr == -1 && root->child[0] != -1 && root->n < 2) ||(parent_addr != -1 && root->n < LIMIT_DEGREE_2)) {
            if (parent_addr == -1) {
                if (root->child[0] != -1 && root->n < 2) {
                    tmp_addr = root_addr; root_addr = root->child[0]; ctrlblock = (BCtrlBlock*)get_page(pool, 0);
                    tmp_node = (BNode*)get_page(pool, tmp_addr); tmp_node->next = ctrlblock->free_node_head;
                    ctrlblock->free_node_head = tmp_addr;  tmp_node->n = 0;
                    for (ii = 0; ii < 2 * DEGREE + 1; ii++) {
                        tmp_node->child[ii] = -1; get_rid_idx(tmp_node->row_ptr[ii]) = 0;
                        get_rid_block_addr(tmp_node->row_ptr[ii]) = -1;
                    }
                    ctrlblock->root_node = root_addr; release(pool, tmp_addr); release(pool, 0);
                    return root_addr;
                }
            }
            else {
                sibor_addr = -1; BNode * child_1 = NULL, * child_2 = NULL, * child_3 = NULL;
                parent_node = (BNode*)get_page(pool, parent_addr);
                if (parent_node->child[1] != -1) { child_1 = (BNode*)get_page(pool, parent_node->child[1]); }
                if (parent_node->child[i - 1] != -1) { child_2 = (BNode*)get_page(pool, parent_node->child[i - 1]); }
                if (parent_node->child[i + 1] != -1) { child_3 = (BNode*)get_page(pool, parent_node->child[i + 1]); }
                release(pool, parent_node->child[1]);  release(pool, parent_node->child[i - 1]);  release(pool, parent_node->child[i + 1]);
                if (i == 0) { if (child_1->n > LIMIT_DEGREE_2 && parent_node->child[1] != -1) { sibor_addr = parent_node->child[1]; j = 1; } }
                else {
                    if (child_2->n > LIMIT_DEGREE_2 && parent_node->child[i - 1] != -1) { sibor_addr = parent_node->child[i - 1];  j = i - 1; }
                    else if (i + 1 < parent_node->n && child_3->n > LIMIT_DEGREE_2 && parent_node->child[i + 1] != -1) { sibor_addr = parent_node->child[i + 1]; j = i + 1; }
                }
                release(pool, parent_addr);
                if (sibor_addr != -1) { MoveElement(pool, sibor_addr, root_addr, parent_addr, j, 1, cmp); }
                else {
                    parent_node = (BNode*)get_page(pool, parent_addr); release(pool, parent_addr);
                    if (i == 0) { sibor_addr = parent_node->child[1]; }
                    else { sibor_addr = parent_node->child[i - 1]; }
                    parent_addr = MergeNode(pool, parent_addr, root_addr, sibor_addr, i, cmp);
                    parent_node = (BNode*)get_page(pool, parent_addr); release(pool, parent_addr);  root_addr = parent_node->child[i];
                }
            }
        }
    }
    if (parent_addr != -1) {
        parent_node = (BNode*)get_page(pool, parent_addr);  child_addr = parent_node->child[i];
        child_node = (BNode*)get_page(pool, child_addr); parent_node->row_ptr[i] = child_node->row_ptr[0];
        release(pool, child_addr);  release(pool, parent_addr);
    }
    return root_addr;
}


void b_tree_init(const char* filename, BufferPool* pool) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    // file not exists
    if (pool->file.length == 0) {
        short i = 0;
        Page* init = get_page(pool, 0);
        release(pool, 0);
        init = get_page(pool, PAGE_SIZE);
        release(pool, PAGE_SIZE);
        // initialize ctrlblock
        BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0);
        ctrlblock->n_node = 2;
        ctrlblock->root_node = PAGE_SIZE;
        ctrlblock->free_node_head = -1;
        release(pool, 0);
        // initialize b-tree root
        BNode* root = (BNode*)get_page(pool, PAGE_SIZE);
        root->next = -1;
        for (i = 0; i < 2 * DEGREE + 1; i++) {
            root->child[i] = -1;
            get_rid_block_addr(root->row_ptr[i]) = -1;
            get_rid_idx(root->row_ptr[i]) = 0;
        }
        release(pool, PAGE_SIZE);
    }
    return;
}

void b_tree_close(BufferPool* pool) {
    close_buffer_pool(pool);
}

RID b_tree_search(BufferPool* pool, void* key, size_t size, b_tree_ptr_row_cmp_t cmp) {
    BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0); release(pool, 0);
    return search(pool, key, size, cmp, ctrlblock->root_node);
}

RID b_tree_insert(BufferPool* pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t insert_handler) {
    BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0); release(pool, 0);
    logic_insert(pool, ctrlblock->root_node, rid, 0, -1, cmp); return rid;
}

void b_tree_delete(BufferPool* pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t insert_handler, b_tree_delete_nonleaf_handler_t delete_handler) {
    BCtrlBlock* ctrlblock = (BCtrlBlock*)get_page(pool, 0);  release(pool, 0);
    logic_delete(pool, ctrlblock->root_node, rid, 0, -1, cmp);
}
