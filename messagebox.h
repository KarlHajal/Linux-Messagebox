#ifndef __MESSAGEBOX_H
#define __MESSAGEBOX_H

struct message {
    int m_id;
    char* message;
	int length;
	int type; // ->  1-Broadcast / 2-Unicast / 3-Both
    int pid; //user the message is intended to in case of a unicast
};

struct messagebox {
    char topic[20];
	char* description;
	int len;
	int mb_id;
    int subscribers_id[10];
    int max_subscribers;
    int number_of_subscribers;
    struct message **messages;
    int max_messages;
    int number_of_messages;
};

#endif
