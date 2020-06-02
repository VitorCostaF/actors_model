
#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H 1

#ifndef MAX_MESSAGES
#define MAX_MESSAGES 50
#endif

#ifndef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE 100
#endif

//Macros de erro
#define SUCCESS 0
#define BUFFER_EMPTY 1
#define BUFFER_FULL 2

#ifdef OLD_IMPL
typedef struct 
{
    int top;
    char messages[MAX_MESSAGES][MAX_MESSAGE_SIZE];
} ringbuffer;
#endif

#ifndef OLD_IMPL
typedef struct 
{   
    int start;
    int finish;
    int ready[MAX_MESSAGES];
    char messages[MAX_MESSAGES][MAX_MESSAGE_SIZE];
} ringbuffer;
#endif


int ring_buffer_add(ringbuffer* rg_bf, char* message);
int ring_buffer_pop(ringbuffer* rg_bf, char** message);
void init_ring_buffer_vector(ringbuffer* rg_bf_vector, int vector_size);

#endif