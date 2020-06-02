#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>
#include<errno.h>

#include "actor.h"
#include "ring_buffer.h"

#define MAX_ACTOR 10

#ifndef TIME_OUT
#define TIME_OUT 1.0
#endif 

#ifndef OUT_FILE
#define OUT_FILE "output_file.txt"
#endif


int actor_count;
int actor_active_count = 0;

mailbox email_server[MAX_ACTOR];

int mailboxes_waiting[MAX_ACTOR];
pthread_mutex_t mailboxes_mutex[MAX_ACTOR];
pthread_mutex_t barrier_mutex;
pthread_mutex_t file_mutex;
pthread_cond_t actor_wait_cond;
pthread_cond_t mail_wait_cond[MAX_ACTOR];


FILE * output_file;

void barrier() {
    pthread_mutex_lock(&barrier_mutex);
    actor_active_count++;

    if(actor_active_count == actor_count) 
        pthread_cond_broadcast(&actor_wait_cond);
    else 
        while (pthread_cond_wait(&actor_wait_cond, &barrier_mutex) != 0) ;

    pthread_mutex_unlock(&barrier_mutex);
}

#ifdef OLD_IMPL
void init_mailserver(mailbox * list_mail, int list_size) {
    for(int i = 0; i < list_size; i++) 
        list_mail[i].top = -1;
}

void *actor(void* address) {
    int count_aux = 0;
    long my_address = (long) address;
    char my_message[MAX_MESSAGE_SIZE];
    clock_t init_time;
    mailbox* my_mailbox = &email_server[my_address];    
    
    init_time = clock();

    barrier();
    
    while(count_aux < 20 || my_mailbox->top >= 0) {
        
        long send_address = (my_address+1)%actor_count;

        if(count_aux < 20) {
            sprintf(my_message, "Hello actor %ld I am actor %ld.", send_address, my_address);
            send_message(&email_server[send_address], my_message, send_address);
        }
        
        if(my_mailbox->top >= 0) {
            char* recv_msg = read_message(my_mailbox, my_address);
            printf("Actor %ld received > %s\n",my_address, recv_msg);
        }
        
        count_aux++;
    }
    
    pthread_mutex_lock(&file_mutex);
    fprintf(output_file, "Actor %ld time: %lf\n", my_address, (double)(clock() - init_time)/CLOCKS_PER_SEC);
    pthread_mutex_unlock(&file_mutex);

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
#endif

#ifndef OLD_IMPL
void init_mailserver(mailbox * list_mail, int list_size) {
    init_ring_buffer_vector(list_mail, list_size);
}

void *actor(void* address) {
    int count_aux = 0;
    clock_t init_time;
    long my_address = (long) address;
    char my_message[MAX_MESSAGE_SIZE];
    mailbox* my_mailbox = &email_server[my_address];    

    init_time = clock();

    barrier();

    while(count_aux < 20) {
        long send_address = (my_address+1)%actor_count;
        struct timespec timeout;
        timeout.tv_nsec = 0;
        timeout.tv_sec = 2;
        
        sprintf(my_message, "Hello actor %ld I am actor %ld.", send_address, my_address);

        if(send_message_atomic(&email_server[send_address], my_message, send_address))
            printf("Actor %ld mailbox is full=\n", send_address);

        char* recv_msg = read_message_time(my_mailbox, my_address, timeout);
        
        if(recv_msg)
            printf("Actor %ld received > %s\n",my_address, recv_msg);      
        count_aux++;
    }

    pthread_mutex_lock(&file_mutex);
    fprintf(output_file, "Actor %ld time: %lf\n", my_address, (double)(clock() - init_time)/CLOCKS_PER_SEC);
    pthread_mutex_unlock(&file_mutex);

    return NULL;
}

// void send_message(mailbox *mail, char * message, int send_address) {
    
//     pthread_mutex_lock(&mail->finish_mutex);
//     if(mail->flag) { 
//         printf("Actor %d mailbox is full\n", send_address);
//     }
//     else {
//         strcpy(mail->messages[mail->finish], message);
//         mail->finish = (mail->finish + 1)%MAX_MESSAGES;
//         if(mail->finish == mail->start)
//             mail->flag=1;
//     }
//     pthread_mutex_unlock(&mail->finish_mutex);
// }

int send_message_atomic(mailbox *mail, char * message, int send_address) {

    int retorno;

    retorno = ring_buffer_add(mail, message);

    if(retorno == SUCCESS) {

        if(mailboxes_waiting[send_address]) {
            mailboxes_waiting[send_address]--;
            pthread_cond_signal(&mail_wait_cond[send_address]);
        }
    }

    return retorno;
}

// char* read_message(mailbox *mail, int address) {

//     if((mail->start >= mail->finish) && !mail->flag)
//         return NULL;
//     char* message = mail->messages[mail->start];
//     mail->start = (mail->start + 1)%MAX_MESSAGES;
//     __atomic_store_n(&(mail->flag), 0, __ATOMIC_RELEASE);
//     return message;
// }

char* read_message_time(mailbox *mail, int address, struct timespec timeout) {
    int retorno;
    char* message = NULL;
    char** message_pt = &message;
    pthread_mutex_t cond_mutex;
    pthread_mutex_init(&cond_mutex, NULL);

    if(ring_buffer_pop(mail, message_pt) == BUFFER_EMPTY) {

        pthread_mutex_lock(&mailboxes_mutex[address]);
        mailboxes_waiting[address]++;

        do {
            retorno = pthread_cond_timedwait(&mail_wait_cond[address], &mailboxes_mutex[address], &timeout);
            if(retorno == ETIMEDOUT) 
                break;
        } while (retorno);

        if(!retorno)
            ring_buffer_pop(mail, message_pt);

        pthread_mutex_unlock(&mailboxes_mutex[address]);
        mailboxes_waiting[address]--;

    }      
    return message;
}
#endif

int main(int argc, char* argv[]) {
    pthread_t* actor_handles;
    clock_t global_initial_time;

    global_initial_time = clock();
    actor_count = strtol(argv[1], NULL, 10);
    output_file = fopen(OUT_FILE, "a+");

    fprintf(output_file, "starting the application\n\n");

    if(actor_count > MAX_ACTOR) {
        printf("max number of actors (%d) exceded\n", MAX_ACTOR);
        return 1;    
    }

    fprintf(output_file, "Actors number: %d\n\n", actor_count);

    for(int i = 0; i < actor_count; i++) 
        pthread_mutex_init(&mailboxes_mutex[i], NULL);
    
    memset(mailboxes_waiting, 0, MAX_ACTOR*sizeof(int));

    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    pthread_cond_init(&actor_wait_cond, NULL);

    for(int i = 0; i < actor_count; i++) 
        pthread_cond_init(&mail_wait_cond[i], NULL);

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
    pthread_mutex_destroy(&file_mutex);
    pthread_cond_destroy(&actor_wait_cond);
    for(int i = 0; i < actor_count; i++) 
        pthread_cond_destroy(&mail_wait_cond[i]);

    free(actor_handles);
    
    fprintf(output_file,"\nglobal time: %lf s\n", (double)(clock() - global_initial_time) / CLOCKS_PER_SEC);
    fprintf(output_file, "\n++++++++++++++++++++++++++++++++++++++++++++\n");
    fclose(output_file);

    return 0;
}