
#ifndef __ACTOR_H
#define __ACTOR_H 1

#include<pthread.h>
#include "ring_buffer.h"

typedef ringbuffer mailbox; 

void *actor(void* address);

void send_message(mailbox* mail, char* message, int send_address);

int send_message_atomic(mailbox* mail, char* message, int send_address);

char* read_message_time(mailbox* mail, int address, struct timespec time);

char* read_message(mailbox* mail, int address);

void init_mailserver(mailbox* list_mail, int list_size);

#endif