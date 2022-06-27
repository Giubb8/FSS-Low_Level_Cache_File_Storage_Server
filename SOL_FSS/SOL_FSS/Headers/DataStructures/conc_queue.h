/* DESCRIZIONE CODA CONCORRENTE */

#ifndef conc_queue_h
#define conc_queue_h

#include "./conc_elem.h"

typedef struct conc_queue {
    conc_node head;     // pointer to the actual queue
    pthread_mutex_t queue_mtx;      // mutex to implement coarse-grained locking
    pthread_cond_t queue_cv;        // cond var to let threads wait on conditions related to the queue (to use with coarse-grained locking)
} conc_queue;

conc_queue* conc_queue_create(void*);      // Creates and returns an empty concurrent queue, with arg as head's data
int conc_queue_push(conc_queue*, void*);     // Inserts a generic node at the tail of the queue
void* conc_queue_pop(conc_queue*);     // Removes the generic node at the queue's head
int conc_queue_isEmpty(conc_queue*);     // Returns TRUE if the queue is empty, FALSE otherwise
int queue_dealloc_full(conc_queue*);     // Deallocates each generic node of the queue and all of their data
void queue_print(conc_queue*);
#endif // conc_queue_h