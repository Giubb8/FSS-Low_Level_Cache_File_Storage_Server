#pragma once

#include "./conc_elem.h"

typedef struct llist {
    conc_node head;     // list's head
} llist;

llist* ll_create();    // Creates and returns an empty list, with arg as head's data
int ll_insert_head(llist**, void*,
    int(*cmp_fnc)(const void*, const void*));      // inserts a generic node at the head of the list
int ll_remove(llist**, void*,
    int(*cmp_fnc)(const void*, const void*));       // removes the specified element from the list
int ll_search(llist*, void*,
    int(*cmp_fnc)(const void*, const void*));       // returns true if the list contains the specified element
int ll_isEmpty(llist*);     // returns true if the list is empty
int ll_dealloc_full(llist*);        // deallocates the list with each node's data

