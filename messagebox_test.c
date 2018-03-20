#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"
#include <string.h>
#include <include/lab3/messagebox.h>

int main(int argc, char ** argv){
	int mb_number = 10;
	int i;
	int id;
	struct messagebox *mb;

    /*------------------sys_create_messagebox TEST------------------------*/
	for(i=0; i < mb_number; i++){
		mb = (struct messagebox*)malloc(sizeof(struct messagebox));
		mb->description = "Description";
		if(i & 1)
			strncpy(mb->topic, "Art", sizeof(mb->topic));
		else
			strncpy(mb->topic, "Science", sizeof(mb->topic));
		id = syscall(__NR_create_messagebox, mb);
		if(id == -1)
			printf("Could not create messagebox\n");
		else
			printf("Created messagebox with id %i and topic %s\n", id, mb->topic);
        free(mb);
	}	
	

    /*----------------sys_list_all_messageboxes TEST-----------------------*/

	printf("\nListing all messageboxes: \n\n");

	struct messagebox *listed_mbs;
	listed_mbs = (struct messagebox*)malloc(mb_number*sizeof(struct messagebox));
	
	int listed_num;
	listed_num = syscall(__NR_list_all_messageboxes,listed_mbs, mb_number);
	printf("Number of messageboxes listed: %i \n\n", listed_num);
	for(i=0; i < listed_num; i++){
		mb = listed_mbs + i*sizeof(struct messagebox);
        printf("Reading messagebox with id %i\n", mb->mb_id);
	}

    /*-------------sys_list_messageboxes_by_topic TEST-----------------------*/
    char* topic = "Art";
	printf("\nListing all messageboxes whose topic is %s: \n\n", topic);
	listed_num = syscall(__NR_list_messageboxes_by_topic, topic, strlen(topic), listed_mbs, mb_number);
	printf("Number of messageboxes whose topic is Art: %i \n\n", listed_num); 
	for(i=0; i < listed_num; i++){
		mb = listed_mbs + i;
		printf("Reading messagebox with id %i\n", mb->mb_id);
	}

    /*-----------sys_subscribe_to_messagebox TEST---------------------------*/
    for(i=1; i <= 10; i++){
        printf("\nUser %i has subscribed to messagebox %i", i, id);
        syscall(__NR_subscribe_to_messagebox, id, i);
    }
    
    /*----------sys_get_number_of_subscribers TEST-------------------------*/
    int nb_of_subs = syscall(__NR_get_number_of_subscribers, id);
    printf("\n\nMessagebox %i's number of subscribers: %i\n", id, nb_of_subs);


    /*---------sys_list_subscribers TEST----------------------------------*/
    int *pids;
    int *pid;
    int max_pids = 20;
    pids = malloc(20*sizeof(int));
    listed_num = syscall(__NR_list_subscribers, id, pids, max_pids);
    printf("\nNumber of subscribers to messagebox %i that were listed: %i\nList:\n", id, listed_num);
    for(i=0; i < listed_num; i++){
        pid = pids + i;
        printf("User %i\n", *pid);
    }

    /*--------sys_send_message TEST--------------------------------------*/
    char* msg = "Hello Messagebox";
    int msg_length = 16;
    int dest_id = 4;
    listed_num = syscall(__NR_send_message, id, msg, msg_length, 0);
    if(listed_num == 0)
        printf("\nMessage %s was sent successfuly to messagebox %i as a Broadcast.\n", msg, id);
    else
        printf("\nMessage could not be sent.\n");
    listed_num = syscall(__NR_send_message, id, msg, msg_length, dest_id);
    if(listed_num == 0)
        printf("\nMessage %s was sent successfuly to messagebox %i as a Unicast to User %i.\n", msg, id, dest_id);
    else
        printf("\nMessage could not be sent.\n");
    /*-----------------sys_get_list_of_messages TEST-----------------------*/
    int *msg_ids, *msg_lengths;
    msg_ids = malloc(5*sizeof(int));
    msg_lengths = malloc(5*sizeof(int));
    listed_num = syscall(__NR_get_list_of_messages, id, msg_ids, msg_lengths ,5);
    printf("\nListing all messages found in messagebox %i:\n", id);
    printf("Number of messages found: %i\n", listed_num);
    for(i=0; i < listed_num; i++){
        printf("%i - Message id: %i, Length: %i\n", i+1, *(msg_ids+i*sizeof(int)), *(msg_lengths+i*sizeof(int)));
    }
    
    /*----------------sys_get_message TEST------------------------------------*/
    char* rec_msg;
    rec_msg = malloc(msg_length*sizeof(char));
    listed_num = syscall(__NR_get_message, id, *msg_ids, rec_msg, msg_length);
    if(listed_num == 1)
        printf("\nMessage Received (ID %i): %s\n", *msg_ids, rec_msg);
    else
        printf("\nCould not get message with ID %i\n", *msg_ids);

	return 0;
}
