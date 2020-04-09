#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>

#include "actor.h"

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

pthread_mutex_t mailboxes_mutex[MAX_ACTOR];
pthread_mutex_t barrier_mutex;
pthread_mutex_t file_mutex;
pthread_cond_t actor_wait_cond;

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
    for(int i = 0; i < list_size; i++)  {
        list_mail[i].start = 0;
        list_mail[i].finish = 0;
        list_mail[i].flag = 0;
        pthread_mutex_init(&list_mail[i].finish_mutex, NULL);
    }        
}

void *actor(void* address) {
    int count_aux = 0;
    int time_out = 0, time_init = 1;
    clock_t init_wait_time, finish_wait_time;
    clock_t init_time;
    long my_address = (long) address;
    char my_message[MAX_MESSAGE_SIZE];
    mailbox* my_mailbox = &email_server[my_address];    

    init_time = clock();

    barrier();

    init_wait_time = clock();
    finish_wait_time = clock();

    while(count_aux < 20 || !time_out) {
        if(count_aux < 20) {
            long send_address = (my_address+1)%actor_count;
            sprintf(my_message, "Hello actor %ld I am actor %ld.", send_address, my_address);
            send_message(&email_server[send_address], my_message, send_address);    
        }
        char* recv_msg = read_message(my_mailbox, my_address);
        //printf("flag %d start %d finish %d \n", my_mailbox->flag, my_mailbox->start, my_mailbox->finish);
        if(recv_msg) {
            printf("Actor %ld received > %s\n",my_address, recv_msg);
            time_init = 1;
            time_out = 0;
        }
        else {
            if(time_init) {
                init_wait_time = clock();
                time_init = 0;
            }
            else  {
                finish_wait_time = clock();
                if((finish_wait_time - init_wait_time)/CLOCKS_PER_SEC > TIME_OUT)
                    time_out = 1;
            }       
        }        
        count_aux++;
    }

    pthread_mutex_lock(&file_mutex);
    fprintf(output_file, "Actor %ld time: %lf\n", my_address, (double)(clock() - init_time)/CLOCKS_PER_SEC);
    pthread_mutex_unlock(&file_mutex);

    return NULL;
}

void send_message(mailbox *mail, char * message, int send_address) {
    
    pthread_mutex_lock(&mail->finish_mutex);
    if(mail->flag) { 
        printf("Actor %d mailbox is full\n", send_address);
    }
    else {
        strcpy(mail->messages[mail->finish], message);
        mail->finish = (mail->finish + 1)%MAX_MESSAGES;
        if(mail->finish == mail->start)
            mail->flag=1;
    }
    pthread_mutex_unlock(&mail->finish_mutex);
}

char* read_message(mailbox *mail, int address) {

    if(mail->start == mail->finish && !mail->flag)
        return NULL;
    char* message = mail->messages[mail->start];
    mail->start = (mail->start + 1)%MAX_MESSAGES;
    mail->flag = 0;
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
    
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
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
    pthread_mutex_destroy(&file_mutex);
    pthread_cond_destroy(&actor_wait_cond);
    free(actor_handles);
    
    fprintf(output_file,"\nglobal time: %lf s\n", (double)(clock() - global_initial_time) / CLOCKS_PER_SEC);
    fprintf(output_file, "\n++++++++++++++++++++++++++++++++++++++++++++\n");
    fclose(output_file);

    return 0;
}