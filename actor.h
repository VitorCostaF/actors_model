
#include<pthread.h>

#ifndef MAX_MESSAGES
#define MAX_MESSAGES 50
#endif

#ifndef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE 100
#endif

#ifdef OLD_IMPL
typedef struct 
{
    int top;
    char messages[MAX_MESSAGES][MAX_MESSAGE_SIZE];
} mailbox;
#endif

#ifndef OLD_IMPL
typedef struct 
{   
    int start;
    int finish;
    int flag;
    pthread_mutex_t finish_mutex;
    char messages[MAX_MESSAGES][MAX_MESSAGE_SIZE];
} mailbox;
#endif

void *actor(void* address);

void send_message(mailbox* mail, char * message, int send_address);

int send_message_atomic(mailbox* mail, char * message, int send_address);

char* read_message_time(mailbox *mail, int address, struct timespec time);

char* read_message(mailbox* mail, int address);

void init_mailserver(mailbox * list_mail, int list_size);