#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <error.h>
#include <time.h>
#include <string.h>
#include <arch/x86/include/generated/uapi/asm/unistd_64.h>
#include <include/lab3/messagebox.h>

#define N 9

typedef sem_t Semaphore;

void semaphore_wait(Semaphore *sem){
    int n = sem_wait(sem);
    if (n != 0){
        perror("sem_wait failed");
        exit(-2);
    }
}

void semaphore_signal(Semaphore *sem){
    int n = sem_post(sem);
    if (n != 0){ 
        perror("sem_post failed");
        exit(-2);
    }
}

Semaphore *mutex;

int *message_id;
int *message_length;

void chinese_whispers(int mb_id, int msg_length, int player_id){
    char* msg = malloc(sizeof(char)*(msg_length));
    int test;
    int pos;
    for(;;){
        semaphore_wait(mutex);//grab semaphore
        syscall(__NR_get_list_of_messages, mb_id, message_id, message_length, player_id);
        test = syscall(__NR_get_message, mb_id, *(message_id+sizeof(int)*(player_id-1)), msg, msg_length, player_id); 
        if(test == 1){ // the message is meant for this player
            srand(player_id*time(0));
            do{
                pos = rand() % msg_length;
            }while(*(msg + pos*sizeof(char)) == ' ');
            *(msg + pos*sizeof(char)) = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 26]; //good enough random character
            printf("Iteration %i: %s\n", player_id, msg);
            if(syscall(__NR_send_message, mb_id, msg, msg_length, player_id+1) != 0){
                perror("Couldn't send message.");
                exit(1);
            }
            semaphore_signal(mutex);//release semaphore
            return;
        }
        semaphore_signal(mutex);//release semaphore
    }
}


int main(int argc, char ** argv){
    
    mutex = malloc(sizeof(Semaphore));
    if(mutex == NULL){
        perror("Couldn't allocate memory");
        exit(-2);
    }
    int n = sem_init(mutex, 0, 1);
    if (n != 0) {
        perror("sem_init failed");
        exit(-2);
    }
   
    int segment_id = shmget(2000, sizeof(struct messagebox), IPC_CREAT | 0666);
    struct messagebox *mbox = shmat(segment_id, NULL, 0);
    
    if(mbox == (void*)-1){
        perror("shmat: no shared memory at the provided shared memory id");
        exit(1);
    }

    int mb_id = syscall(__NR_create_messagebox, mbox);
    if(mb_id == -1){
        perror("Could not create messagebox");
        exit(2);
    }

    char* msg = "KNOW THYSELF";
    printf("Original Message(Iteration 0): %s \n", msg);
    int test = syscall(__NR_send_message, mb_id, msg, 12, 1); 

    message_id = malloc(N*sizeof(int));
    message_length = malloc(N*sizeof(int));
    test = syscall(__NR_get_list_of_messages, mb_id, message_id, message_length, 1);

    int pid=0;
    int number_of_players = 8;
    while(pid < N){
        test = fork();
        if(test < 0){
            perror("Error starting another process.");
            exit(-1);
        }
        else if(test != 0){ //parent process
            if(pid == 0)
                break;
            chinese_whispers(mb_id, *message_length,  pid);
            break;
        }
        pid++;
    }
    return 0;
}
