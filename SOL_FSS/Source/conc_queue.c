#include "../Headers/DataStructures/conc_queue.h"
#include<stdio.h>
#include<stdint.h>
#define SUCCESS 5
#define ERR -1
#define TRUE 1
#define FALSE 0 
conc_queue* conc_queue_create(void* data) {
    int temperr;

    conc_queue* queue=(conc_queue*)malloc(sizeof(conc_queue));
    if(!queue) return NULL;

    temperr=pthread_mutex_init(&(queue->queue_mtx), NULL);
    if(temperr) {errno=temperr; return NULL;}

    temperr=pthread_cond_init(&(queue->queue_cv), NULL);
    if(temperr) {errno=temperr; return NULL;}

    queue->head=conc_node_create(data);
    if(!(queue->head)) {free(queue); return NULL;}

    return queue;
}


int conc_queue_push(conc_queue* queue, void* data) {
    if(!queue) {errno=EINVAL; return ERR;}     // Uninitialized queue
    if(!(queue->head)) {errno=EINVAL; return ERR;}     // Uninitialized queue

    int temperr;
    conc_node newelement=conc_node_create(data);
    if(!newelement) return ERR;     // errno already set

    temperr=pthread_mutex_lock(&((queue->head)->node_mtx));
    if(temperr) {errno=temperr; free(newelement); return ERR;}

    if(!((queue->head)->next)) {         // If queue is empty, append element and return
        (queue->head)->next=newelement;
        temperr=pthread_mutex_unlock(&((queue->head)->node_mtx));
        if(temperr) {errno=temperr; free(newelement); return ERR;}
        return SUCCESS;
    }     
    else {
        conc_node aux1=queue->head, aux2;
        while(aux1->next!=NULL) {
            aux2=aux1;
            aux1=aux1->next;
            temperr=pthread_mutex_lock(&(aux1->node_mtx));
            if(temperr) {errno=temperr; free(newelement); return ERR;}
            temperr=pthread_mutex_unlock(&(aux2->node_mtx));
        }
        aux1->next=newelement;
        temperr=pthread_mutex_unlock(&(aux1->node_mtx));
        if(temperr) {errno=temperr; free(newelement); return ERR;}
    }
    return SUCCESS;
}


// Removes the generic node at the queue's head
void* conc_queue_pop(conc_queue* queue) {
    if(!queue) {errno=EINVAL; return (void*)NULL;}     // Uninitialized queue
    if(!(queue->head)) {errno=EINVAL; return (void*)NULL;}     // Uninitialized queue
    int temperr;
    errno=0;
    temperr=pthread_mutex_lock(&((queue->head)->node_mtx));

    if(temperr) {errno=temperr; return (void*)NULL;}

    if(!((queue->head)->next)) {

        temperr=pthread_mutex_unlock(&((queue->head)->node_mtx));
        if(temperr) {errno=temperr; return (void*)NULL;}

        return (void*)NULL;
    }

    conc_node aux=(queue->head)->next;
    temperr=pthread_mutex_lock(&(aux->node_mtx));
    if(temperr) {errno=temperr; return (void*)NULL;}

    (queue->head)->next=aux->next;

    temperr=pthread_mutex_unlock(&((queue->head)->node_mtx));
    if(temperr) {errno=temperr; return (void*)NULL;}
    temperr=pthread_mutex_unlock(&(aux->node_mtx));
    if(temperr) {errno=temperr; return (void*)NULL;}
    return (conc_node_destroy(aux));
}

// Returns TRUE if the queue is empty, ELSE otherwise
int conc_queue_isEmpty(conc_queue* queue) {
  if(!queue) {errno=EINVAL; return ERR;}     // Uninitialized queue
  if(!(queue->head)) {errno=EINVAL; return ERR;}     // Uninitialized queue

  int temperr;
  temperr=pthread_mutex_lock(&((queue->head)->node_mtx));
  if(temperr) {errno=temperr; return ERR;}

  if(!((queue->head)->next)) {      // If empty, return TRUE
    temperr=pthread_mutex_unlock(&((queue->head)->node_mtx));
    if(temperr) {errno=temperr; return ERR;}
    return TRUE;
  }
  
  // else, unlock mutex and return FALSE
  temperr=pthread_mutex_unlock(&((queue->head)->node_mtx));
  if(temperr) {errno=temperr; return ERR;}

  return FALSE;
}


// Deallocates each generic node of the queue and all of their data; WARNING: NON CONCURRENT
int queue_dealloc_full(conc_queue* queue) {
    if(!queue) {errno=EINVAL; return ERR;}     // Uninitialized queue
    if(!(queue->head)) {errno=EINVAL; return ERR;}     // Uninitialized queue
    //printf("DENTRO head next filenamequeue %s\n",queue->head->next->data);
    /*if(queue->head==NULL){
        printf("head null\n");
    }
    if(queue->head->next==NULL){
        printf("next null\n");
    }
    /*if(queue->head->next->data==NULL){
        printf("data null\n");
    }*/
    int temperr;
    void* tempres=NULL;

    if(!((queue->head)->next)) {// For an empty queue, deallocating its head node and pointer suffices   
        if((tempres=conc_node_destroy(queue->head))) {
            return ERR;
        }     // errno already set by the call

        free(tempres);
        temperr=pthread_mutex_destroy(&(queue->queue_mtx));
        if(temperr) {errno=temperr; printf("mutex errno %d\n",errno); return ERR;}
        temperr=pthread_cond_destroy(&(queue->queue_cv));
        if(temperr) {errno=temperr; printf("cond\n"); return ERR;}
        free(queue);
        return SUCCESS;
    }

    conc_node aux1=queue->head;
    conc_node aux2=NULL;
    while(aux1!=NULL) {
        aux2=aux1;
        aux1=aux1->next;
        if(!(tempres=conc_node_destroy(aux2))) {
            return ERR;}     // errno already set by the call
        free(tempres);
    }

    temperr=pthread_mutex_destroy(&(queue->queue_mtx));    

    if(temperr) {errno=temperr; return ERR;}
    temperr=pthread_cond_destroy(&(queue->queue_cv));

    if(temperr) {errno=temperr; return ERR;}
    free(queue);
    return SUCCESS;
}


void queue_print(conc_queue * queue){
    if(queue==NULL){
        perror("errore queue print");
        exit(EXIT_FAILURE);
    }
    conc_node aux=queue->head;
    while(aux->next!=NULL){        
        aux=aux->next;
        printf("%d->",(int)(uintptr_t)aux->data);
    }
    printf("\n");
}


void queue_print_char(conc_queue * queue){
    if(queue==NULL){
        perror("errore queue print");
        exit(EXIT_FAILURE);
    }
    conc_node aux=queue->head;
    while(aux->next!=NULL){        
        aux=aux->next;
        printf("%s->",(char*)aux->data);
    }
    printf("\n");
}