#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define CLE 314

int msgid;
int semid;
int shmid;

int* temps;

struct sembuf sem_oper_P ;  /* Operation P */
struct sembuf sem_oper_V ;  /* Operation V */

typedef struct type_processus
{
    long type;
    int priorite;
    int date_soumission;
    int temps_exec;
} processus;

void P() {
    sem_oper_P.sem_num = 0;
    sem_oper_P.sem_op  = -1 ;
    sem_oper_P.sem_flg = 0 ;
    semop(semid, &sem_oper_P, 1);
}

void V() {
    sem_oper_V.sem_num = 0;
    sem_oper_V.sem_op  = 1 ;
    sem_oper_V.sem_flg  = 0 ;
    semop(semid, &sem_oper_V, 1);
}

int randomNumber(int min, int max){
    if (min > max) perror("Erreur randomNumber\n");
    return (rand() %(max - min + 1)) + min;
}

int initsem(key_t semkey) 
{
    int status;
	int semid_init;
   	union semun {
		int val;
		struct semid_ds *stat;
		short * array;
	} ctl_arg;
    short array[1] = {1};
    ctl_arg.array = array;

    if ((semid_init = semget(semkey, 1, 0666|IPC_CREAT)) > 0) {
        status = semctl(semid_init, 0, SETALL, ctl_arg);
    }
    if (semid_init == -1 || status == -1) { 
        perror("Erreur initsem\n");
        return (-1);
    }else{
        return semid_init;
    }
}


processus genererProcessus(){
    processus prc;
    prc.priorite = randomNumber(0, 10);
    prc.temps_exec = randomNumber(0, 20);
    return prc;
}

void supprimerMessages(){
    msgctl(msgid, IPC_RMID, NULL);
}


void traitandSIGINT(int num){
    if (num == SIGINT){
        supprimerMessages();
	    shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID, NULL);
        exit(1);
    } else {
        perror("Erreur traitant, files non supprimé\n");
    }
}

int main()
{
    printf("Lancement du programme\n");
    srand(time(0));


    signal(SIGINT, traitandSIGINT);
    msgid;
    if ((msgid = msgget(CLE, 0750 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("Erreur creation file de message\n");
    }

    semid = initsem(CLE);

    shmid = shmget(CLE, sizeof(int), 0666|IPC_CREAT);
	temps = (int *)shmat(shmid, NULL, 0);
	*temps = 0; //initialisation a 0 du compteur


    printf("Creation du fils\n");
    int pid_fils = fork();  
    switch (pid_fils)
    {
        case -1:
            perror("Erreur fork");
            break;


        /****************************************************/
        case 0:
            {
                //FILS
                //Genere des processus


                
                while(1) {
                    P();
                    for (int i = 0; i < randomNumber(0, 10); i++)
                    {
                        processus prc = genererProcessus();
                        prc.type = getpid();
                        prc.date_soumission = *temps;
                        msgsnd(msgid, &prc, sizeof(processus) - sizeof(long), 0);
                        printf("Processus arrivé avec prio : %d\n", prc.priorite);
                    }
                    sleep(2);
                    V();
                }

                exit(0);
            }
            break;


        /****************************************************/
        default:
            //PERE
            //Gere l'algorithme d'ordonnancement
            {
                //int status;
                //wait(&status);
                
                processus liste_prc[20];

                while(1) {
                    P();
                    struct msqid_ds buf;
                    int nb_messages = msgctl(msgid, IPC_STAT, &buf);
                    while (buf.msg_qnum > 0) //Tant qu'il y a des messages
                    {
                        msgrcv(msgid, &liste_prc[0], sizeof(processus) - sizeof(long), pid_fils, 0);
                        nb_messages = msgctl(msgid, IPC_STAT, &buf);
                        printf("Message recu : prio = %d, temps = %d\n", liste_prc[0].priorite, liste_prc[0].date_soumission);
                    }
                    sleep(5);
                    (*temps)++;
                    printf("Temps courant : %d\n", *temps);
                    V();
                }

                //printf("Il reste %ld messages dans la file\n", buf.msg_qnum);

            }
            break;
    }

    msgctl(msgid, IPC_RMID, NULL);

    printf("Fin du programme\n");
    return 0;
}
