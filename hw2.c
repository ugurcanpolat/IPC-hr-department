#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SEMAPHORE  1000
#define SEMAPHORE2 1010

void mysignal(void) {}

void sem_signal(int semid, int val) {
	struct sembuf semafor;
	semafor.sem_num = 0;
	semafor.sem_op = val;
	semafor.sem_flg = 1;
	semop(semid, &semafor, 1);
}

void sem_wait(int semid, int val) {
	struct sembuf semafor;
	semafor.sem_num = 0;
	semafor.sem_op = (-1 * val);
	semafor.sem_flg = 1;
	semop(semid, &semafor, 1);
}

void mysigset(int num) {
	struct sigaction mysigaction;
	mysigaction.sa_handler = (void*)mysignal;
	mysigaction.sa_flags = 0;
	sigaction(num, &mysigaction, NULL);
}

typedef struct applicant {
	int id;
	int ni;
} Applicant; 

Applicant *applicants;
Applicant *registered;
int num_applicants = 0;
int num_waiting = 0;
int num_interviewed = 0;
int nr;
int sem, sem_rep;
pthread_t threads[4];

void* receptionist(void *arg) {
	int i;
	for(i = 0; i < num_applicants; i++) {
		printf("Applicant %d applied to the receptionist\n", (applicants+i)->id);
	}
	for(i = 0; i < num_applicants; i++) {
		sleep(nr); 
		memcpy(registered+i, applicants+i, sizeof(Applicant));
		printf("Applicant %d’s registeration is done\n", (applicants+i)->id);
		num_waiting++;
		sem_signal(sem, 1);
	}
	sem_signal(sem, 1);
	pthread_exit(NULL);
}

void* interviewer(void *arg) {
	int i_id, position = 0;
	
	if(pthread_equal(pthread_self(),threads[1])) 
		i_id = 1;
		
	else if(pthread_equal(pthread_self(),threads[2])) 
		i_id = 2;
		
	else 
		i_id = 3;	
	
	while (num_interviewed < num_applicants) {
		sem_wait(sem, 1);
	
		position = num_interviewed;
		num_interviewed++;
		
		if(num_interviewed > num_applicants)
			break;
		
		printf("Interviewer %d started interview with Applicant %d\n",i_id,(registered+position)->id);
		sleep(5); 
		printf("Interviewer %d finished interview with Applicant %d\n",i_id,(registered+position)->id);
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	// signal handler with num=12
	mysigset(12);
	
	sem = semget(SEMAPHORE, 1, 0700|IPC_CREAT);
	semctl(sem, 0, SETVAL, 0);
	
	sem_rep = semget(SEMAPHORE2, 1, 0700|IPC_CREAT);
	semctl(sem_rep, 0, SETVAL, 1);
	
	if (argc < 3) {
		printf("Please provide the necessary arguments: Input file ");
		printf("and |nr|\nExiting...\n");
		exit(EXIT_FAILURE);
	}
	
	FILE *fp = fopen(*(argv+1), "r");
	
	if(fp == NULL) {
		printf("Invalid input file\nExiting...\n");
		exit(EXIT_FAILURE);
	}
	
	nr = atoi(*(argv+2));
	int ni[100];
	int i, j;
	
	for(i=0; !feof(fp); i++) {
		fscanf(fp, "%d", &ni[i]);
	}
	
	num_applicants = i;
	applicants = malloc(sizeof(Applicant)*num_applicants);
	registered = malloc(sizeof(Applicant)*num_applicants);
	for(j=0; j<num_applicants; j++) {
		(applicants+j)->id = j+1;
		(applicants+j)->ni = ni[j];
	}
	
	int rc;
	for(i=0; i<4;i++) {
		if(i==0)
			rc = pthread_create(&threads[0], NULL, receptionist, NULL);
		else 
			rc = pthread_create(&threads[i], NULL, interviewer, NULL);
		if (rc) {
		  printf("Thread creation error ...\n");
		  exit(EXIT_FAILURE);
		}
	}

	for(i=0; i<4; i++) {
		pthread_join(threads[i], NULL);
	}
	
	printf("All applicants have interviewed successfully.\n");
	fclose(fp);
	free(applicants);
	free(registered);
	pthread_exit(NULL);
	return 0;
}
