#include <string.h>

#include "ring_buffer.h"

int ring_buffer_add(ringbuffer *rg_bf,  char * message) {

    int new_finish, finish, atualizou = 0, start;

    while(!atualizou) {
        //TODO checar se tem problema nesse start e finish
        finish = __atomic_load_n(&(rg_bf->finish), __ATOMIC_ACQUIRE);
        start = __atomic_load_n(&(rg_bf->start), __ATOMIC_ACQUIRE);

        if((start == 0 && finish == MAX_MESSAGES -1) || (finish == start - 1))
            return BUFFER_FULL;

        new_finish = (finish + 1)%MAX_MESSAGES;
        atualizou = __atomic_compare_exchange_n(&(rg_bf->finish), &finish, new_finish, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);

        if(atualizou) {
            strcpy(rg_bf->messages[finish], message);
            __atomic_store_n(&(rg_bf->ready[finish]), 1, __ATOMIC_RELEASE);
        }
    }
    return SUCCESS;
}

int ring_buffer_pop(ringbuffer* rg_bf, char ** message_pt) {
    int atualizou = 0;
    int start, finish, new_start;
    char* message;

    finish = __atomic_load_n(&(rg_bf->finish), __ATOMIC_ACQUIRE);
    start = __atomic_load_n(&(rg_bf->start), __ATOMIC_ACQUIRE);

    if((start >= finish))
        return BUFFER_EMPTY;

    while(!atualizou) {
        start = __atomic_load_n(&(rg_bf->start), __ATOMIC_ACQUIRE);
        new_start = (start + 1)%MAX_MESSAGES;

        atualizou = __atomic_compare_exchange_n(&(rg_bf->start), &start, new_start, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);

        if(atualizou) {

            while (__atomic_load_n(&(rg_bf->ready[start]), __ATOMIC_ACQUIRE) == 0) ;            

            message = rg_bf->messages[start];
            *message_pt = message;
        }
    }
    
    return SUCCESS;
}

void init_ring_buffer_vector(ringbuffer* rg_bf_vector, int vector_size) {
    
    for(int i = 0; i < vector_size; i++)  {
        rg_bf_vector[i].start = 0;
        rg_bf_vector[i].finish = 0;
        for(int j = 0; j < MAX_MESSAGES; j++)
            rg_bf_vector[i].ready[j] = 0;
    }
}