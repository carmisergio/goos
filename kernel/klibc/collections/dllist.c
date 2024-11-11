#include "collections/dllist.h"

#include "mem/kalloc.h"

// Internal function prototypes
dllist_node_t *find_element(dllist_t *list, void *el);
void remove_node(dllist_t *list, dllist_node_t *node);

void dllist_init(dllist_t *list)
{
    list->head = NULL;
    list->tail = NULL;
}

void dllist_insert_head(dllist_t *list, void *data)
{
    dllist_node_t *node = kalloc(sizeof(dllist_node_t));

    node->data = data;
    node->next = list->head;
    node->prev = NULL;

    if (list->head != NULL)
        list->head->prev = node;

    list->head = node;

    if (list->tail == NULL)
        list->tail = node;
}

void dllist_insert_tail(dllist_t *list, void *data)
{
    dllist_node_t *node = kalloc(sizeof(dllist_node_t));

    node->data = data;
    node->prev = list->tail;
    node->next = NULL;

    if (list->tail != NULL)
        list->tail->next = node;

    list->tail = node;

    if (list->head == NULL)
        list->head = node;
}

// void *dllist_remove(dllist_t *list, void *el)
// {
//     // Find node
//     dllist_node_t *node = find_element(list, el);
//     if (node == NULL)
//         return;

//     void *data = node->data;

//     // Remove from list
//     remove_node(list, node);

//     // Free node
//     kfree(node);

//     return data;
// }

// void dllist_free(dllist_t *list)
// {
//     dllist_node_t *cur = list->head;
//     dllist_node_t *next = cur;
//     while (cur != NULL)
//     {
//         next = cur->next;

//         // Free node
//         kfree(cur);

//         cur = next;
//     }
// }

/* Internal functions */
dllist_node_t *find_element(dllist_t *list, void *el)
{
    dllist_node_t *cur = list->head;
    while (cur != NULL)
    {
        if (cur->data == el)
            return cur;

        cur = cur->next;
    }

    return NULL;
}

void remove_node(dllist_t *list, dllist_node_t *node)
{
    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
}

// TODO: handle kalloc failures!