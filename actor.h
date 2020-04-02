



typedef struct 
{
    int top;
    char messages[50][100];
} mailbox;


void *actor(void* address);

void send_message(mailbox* mail, char * message, int send_address);

char* read_message(mailbox* mail, int address);

void init_mailserver(mailbox * list_mail, int list_size);