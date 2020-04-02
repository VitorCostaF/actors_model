#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>

#include "actor.h"

#define MAX_ACTOR 10
#define MAX_MESSAGE_SIZE 50

int actor_count;
int actor_active_count = 0;

mailbox email_server[MAX_ACTOR];

pthread_mutex_t mailboxes_mutex[MAX_ACTOR];
pthread_mutex_t barrier_mutex;
pthread_cond_t actor_wait_cond;

void barrier() {
    pthread_mutex_lock(&barrier_mutex);
    actor_active_count++;

    if(actor_active_count == actor_count) 
        pthread_cond_broadcast(&actor_wait_cond);
    else 
        while (pthread_cond_wait(&actor_wait_cond, &barrier_mutex) != 0) ;

    pthread_mutex_unlock(&barrier_mutex);
}

void init_mailserver(mailbox * list_mail, int list_size) {
    for(int i = 0; i < list_size; i++) 
        list_mail[i].top = -1;
}

void *actor(void* address) {
    int count_aux = 0;
    long my_address = (long) address;
    char my_message[MAX_MESSAGE_SIZE];
    mailbox* my_mailbox = &email_server[my_address];    

    barrier();

    while(count_aux < 20 || my_mailbox->top >= 0) {
        long send_address = (my_address+1)%actor_count;
        sprintf(my_message, "Hello actor %ld I am actor %ld.", send_address, my_address);
        send_message(&email_server[send_address], my_message, send_address);
        if(my_mailbox->top >= 0) {
            char* recv_msg = read_message(my_mailbox, my_address);
            printf("Actor %ld received > %s\n",my_address, recv_msg);
        }
        count_aux++;
    }
    
    return NULL;
}

void send_message(mailbox *mail, char * message, int send_address) {
    
    pthread_mutex_lock(&mailboxes_mutex[send_address]);
    strcpy(mail->messages[++(mail->top)], message);
    pthread_mutex_unlock(&mailboxes_mutex[send_address]);
}

char* read_message(mailbox *mail, int address) {

    pthread_mutex_lock(&mailboxes_mutex[address]);
    char* message = mail->messages[(mail->top)--];
    pthread_mutex_unlock(&mailboxes_mutex[address]);
    return message;
}

int main(int argc, char* argv[]) {
    pthread_t* actor_handles;
    actor_count = strtol(argv[1], NULL, 10);

    if(actor_count > MAX_ACTOR) {
        printf("max number of actors (%d) exceded\n", MAX_ACTOR);
        return 1;    
    }

    for(int i = 0; i < actor_count; i++) 
        pthread_mutex_init(&mailboxes_mutex[i], NULL);
    
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&actor_wait_cond, NULL);

    init_mailserver(email_server, actor_count);

    actor_handles = malloc(actor_count * sizeof(pthread_t));

    for(long i  = 0; i < actor_count; i++)
        pthread_create(&actor_handles[i], NULL, actor, (void*) i);

    for(int i = 0; i < actor_count; i++) 
        pthread_join(actor_handles[i], NULL);

    for(int i = 0; i < actor_count; i++) {
        pthread_mutex_destroy(&mailboxes_mutex[i]);
    }

    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&actor_wait_cond);

    free(actor_handles);
    return 0;
}