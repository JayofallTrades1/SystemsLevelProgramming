#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <string.h>
using namespace std;

#define SHM_SIZE 1024 // 1k shared memory segment 

int shmid; // shared memory
key_t key;
char *data; //pointer to shared mem buffer
int front = 0, back = 0; // two shared memory indices
						 // producer puts a byte to shared memory's back
						 // consumer gets a byte from shared memory's front
int semid; // semaphore
int table_index = 0; //used for monitoring table
vector<char> table; // monitoring table

void handler(int);
void producer_proc(char *argv[], struct sembuf[], struct sembuf[]);
void consumer_proc(char *argv[], struct sembuf[], struct sembuf[]);

int main(int argc, char* argv[]) 
{
	// 1. create shared memory. hint: use shmget
    if(argc != 3)
    {
        cout <<"Illegal arguments: proj2.cpp file1 file2" << endl;
        exit(1);
    }

    //create the segment
    if((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    data = (char*)shmat(shmid, NULL, 0);
    if(data == (char*) -1)
    {
        perror("shmat faled");
        exit(1);
    }
    sembuf CONSUMER[3],PRODUCER[3], SIGNAL[1];

       //Defining PRODUCER
       //Code for the binary semaphore
       PRODUCER[0].sem_num = 0;
       PRODUCER[0].sem_op = -1;
       PRODUCER[0].sem_flg = SEM_UNDO;

       //Defining INCREMENT
       //Our FULL semaphore
       PRODUCER[1].sem_num = 0;
       PRODUCER[1].sem_op = 1;
       PRODUCER[1].sem_flg = SEM_UNDO;

       //Defining INCREMENT
       //Our EMPTY semaphore
       PRODUCER[2].sem_num = 0;
       PRODUCER[2].sem_op = -1;
       PRODUCER[2].sem_flg = SEM_UNDO;

       CONSUMER[0].sem_num = 0;
       CONSUMER[0].sem_op = -1;
       CONSUMER[0].sem_flg = SEM_UNDO;

       //Defining DECREMENT
       //Our FULL semaphore
       CONSUMER[1].sem_num = 0;
       CONSUMER[1].sem_op = 1;
       CONSUMER[1].sem_flg = SEM_UNDO;

       //Defining DECREMENT
       //Our EMPTY semaphore
       CONSUMER[2].sem_num = 0;
       CONSUMER[2].sem_op = -1;
       CONSUMER[2].sem_flg = SEM_UNDO;

       SIGNAL[0].sem_num = 0;
       SIGNAL[0].sem_op = 1;
       SIGNAL[0].sem_flg = SEM_UNDO;

    if((semid = semget(IPC_PRIVATE, 1, IPC_CREAT) == -1))
    {
        perror("semget failed");
        exit(1);
    }

    pid_t producer, consumer; 

    producer = fork();
    consumer = fork();

    if(producer != 0 || consumer != 0)
    {
        perror("producer process error");
        perror("consumer process error");  
    }      
    else
    {
        producer_proc(argv, PRODUCER, SIGNAL);
        consumer_proc(argv, CONSUMER, SIGNAL);
    }

    if(shmdt(data) == -1)
    {
        perror("shmdt error");
        exit(1);
    }
    if(semctl(semid, 0, IPC_RMID, 0) == -1)
    {
        perror("semctl error");
        exit(1);
    }
            
}

/* handler function for signals from producer and consumer process */
void handler(int signum)
{
    if(signum == SIGUSR1)
        table[table_index++] = 'P';
    else if(signum == SIGUSR2)
        table[table_index++] = 'C';
    return;
}

/* producer process function */
void producer_proc(char *argv[], struct sembuf PRODUCER[], struct sembuf SIGNAL[]) 
{
    FILE *fp;
    fp = fopen(argv[1], "R");
    if(fp == NULL)
    {
        cout << "File could not be opened"<< endl;
        exit(1);
    }
    semop(semid, PRODUCER, 3);    
    cout << "Producer begins..." << endl;
    // 3. move bytes from file1 to shared memory
    strncpy(data, (char*)fp, SHM_SIZE);

    while (true)
    {
		// sleep random time
        usleep((rand()%3+1)*100000); // sleep 100~300 ms
        semop(semid, SIGNAL, 1);
        handler(SIGUSR1);
    }
    cout << "Producer ends." << endl;
    fclose(fp);
}

/* consumer process function */
void consumer_proc(char *argv[], struct sembuf CONSUMER[], struct sembuf SIGNAL[]) 
{
    FILE *fp;
    fp = fopen(argv[2], "W");
    if(fp == NULL)
    {
        cout << "File could not be opened" << endl;
        exit(1);
    }
    semop(semid, CONSUMER, 3);
    cout << "Consumer begins..." << endl;
    strncpy((char*)fp, data, SHM_SIZE);
    while (true) {
		// sleep random time
        usleep((rand()%3+1)*100000); // sleep 100~300 ms
        semop(semid, SIGNAL, 1);
    }
    cout << "Consumer ends." << endl;
    fclose(fp);
}