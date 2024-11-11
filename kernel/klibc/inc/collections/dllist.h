#pragma once

#include <stddef.h>

// Doubly linked list

// Doubly linked list node
typedef struct dllist_node dllist_node_t;
typedef struct dllist_node
{
    void *data;
    dllist_node_t *next;
    dllist_node_t *prev;
};

// Doubly linked list type
typedef struct
{
    dllist_node_t *head;
    dllist_node_t *tail;
} dllist_t;

/*
 * Initialize doubly linked list
 */
void dllist_init(dllist_t *list);

/*
 * Insert element at head of list
 */
void dllist_insert_head(dllist_t *list, void *data);

/*
 * Insert element at tail of list
 */
void dllist_insert_tail(dllist_t *list, void *data);

// /*
//  * Remove element from linked list
//  */
// void dllist_remove(dllist_t *list, void *el);

// /*
//  * Free all contents of list
//  */
// void dllist_free(dllist_t *list);

// Get list head
static inline dllist_node_t *dllist_head(dllist_t *list)
{
    return list->head;
}

// Get list tail
static inline dllist_node_t *dllist_tail(dllist_t *list)
{
    return list->tail;
}

// Get next node
static inline dllist_node_t *dllist_next(dllist_node_t *node)
{
    return node->next;
}

// Get previous node
static inline dllist_node_t *dllist_prev(dllist_node_t *node)
{
    return node->next;
}

// Get data from node
static inline void *dllist_data(dllist_node_t *node)
{
    return node->data;
}
