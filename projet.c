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
#include "liste_doublement_chaine.c"

//#ifndef PROJET
//#define PROJET

#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#define FILE 1

#define CLE 314

int msgid;
int semid;
int shmid;

int* temps;

struct sembuf sem_oper_P ;  /* Operation P */
struct sembuf sem_oper_V ;  /* Operation V */



void traitantSIGINT();
void fils();
void ordonnaceur();
void gererListe(Element** liste);

int processus_en_cours = 0;

void initSem() {
    int status;
   	union semun {
		int val;
		struct semid_ds *stat;
		short * array;
	} ctl_arg;

    short array[1] = {1};

    ctl_arg.array = array;

    if ((semid = semget(CLE, 1, 0666|IPC_CREAT)) > 0) {
        status = semctl(semid, 0, SETALL, ctl_arg);
    }

    if (semid == -1 || status == -1) { 
        perror("Erreur initsem\n");
    }
}

void initShm(){
    shmid = shmget(CLE, sizeof(int), 0666|IPC_CREAT);
	temps = (int *)shmat(shmid, NULL, 0);
}

void initMsg(){
    if ((msgid = msgget(CLE, 0750 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("Erreur creation file de message\n");
    }
}

void supprimerMessages(){ msgctl(msgid, IPC_RMID, NULL); }

void supprimerSemaphores(){ semctl(semid, 0, IPC_RMID, NULL); }

void supprimerMemoirePartage(){ shmctl(shmid, IPC_RMID, NULL); }

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


Processus genererProcessus(){
    Processus prc;
    prc.type = FILE;
    prc.priorite = randomNumber(0, 10);
    prc.temps_exec = randomNumber(2, 10);
    prc.mon_pid = getpid();
    return prc;
}


void traitantSIGINT(int num){
    if (num){
        supprimerMessages();
	    supprimerSemaphores();
        supprimerMemoirePartage();
        exit(1);
    } else {
        perror("Erreur traitant, files non supprimé\n");
        exit(1);
    }
}

void traitantSIGINTfils(int num){
    if (num == SIGINT){
        printf("%sLe processus %d s'est terminé%s\n", RED, getpid(), NORMAL);
        supprimerMessages();
	    supprimerSemaphores();
        supprimerMemoirePartage();
        kill(getpid(), SIGINT);
        exit(1);
    } else {
        perror("Erreur traitant, files non supprimé\n");
        exit(1);
    }
}

void traitantSIGSEGV(int num){
    if (num == SIGSEGV){
        printf("Erreur segmentation fault\n");
        kill(getpid(), SIGSEGV);
        exit(1);
    } else {
        perror("Erreur traitant, files non supprimé\n");
        exit(1);
    }
}

int nombreMessages(int msgid){
    struct msqid_ds buf;
    int nb_messages = msgctl(msgid, IPC_STAT, &buf);
    return buf.msg_qnum;
}


int main()
{
    printf("Lancement du programme\n");
    srand(time(0));
    
    signal(SIGINT, traitantSIGINT);
    signal(SIGTERM, traitantSIGINT);
    signal(SIGQUIT, traitantSIGINT);
    signal(SIGSEGV, traitantSIGSEGV);

    initMsg();
    initSem();
    initShm();    

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
            //FILS
            //Genere des processus
            fils();
            break;
        /****************************************************/
        default:
            //PERE
            //Gere l'algorithme d'ordonnancement
            ordonnaceur();
            break;
    }

    
    supprimerMessages();
	supprimerSemaphores();
    supprimerMemoirePartage();

    printf("Fin du programme\n");
    return 0;
}

void fils(){
    for(int i = 0; i < 10; i++) {
        P();
        for (int i = 0; i < randomNumber(0, 3); i++) //Genere un nbre aleatoire de processus à la fois
        {

            int pid = fork();
            if (!pid){
                signal(SIGINT, traitantSIGINTfils);
                Processus prc = genererProcessus();
                prc.date_soumission = *temps;
                msgsnd(msgid, &prc, sizeof(Processus) - sizeof(long), 0);

                kill(getpid(), SIGTSTP);

                while(1){ //simule du travail
                    //printf("%d : travail\n", getpid());
                    sleep(2);
                }
                
                printf("%sProcessus %d s'est termine%s\n",RED, prc.mon_pid, NORMAL);

                exit(EXIT_SUCCESS);
            }

        }
        V();
        sleep(5);
    }
    exit(0);
}

void ordonnaceur(){
    
    Element* liste = NULL;

    while(1) {
        P();
        while (nombreMessages(msgid) > 0) //Tant qu'il y a des messages
        {
            Processus* p = (Processus*)malloc(sizeof(Processus));
            msgrcv(msgid, p, sizeof(Processus) - sizeof(long), FILE, 0);
            printf("%sProcessus %d avec priorite %d est arrive, temps d'exec %d: %s\n",GREEN, p->mon_pid, p->priorite, p->temps_exec, NORMAL);           
            liste = listeAjouterQueue(liste, listeNouvelElement(p));
        }

        printListeProcessus(liste);

        (*temps)++;
        printf("%sTemps courant : %d%s\n",BOLDWHITE, *temps, NORMAL);
        V();

        gererListe(&liste);

        sleep(2);

    }

}

void gererListe(Element** liste){
    if (processus_en_cours) {
        kill(processus_en_cours, SIGTSTP);
    }
    Element *e = listeValeurTete(*liste);
    if(e) {
        Processus *p = e->data;
        if(p->temps_exec > 0){
            kill(p->mon_pid, SIGCONT);
            processus_en_cours = p->mon_pid;
            p->temps_exec--;
        } else {
            kill(p->mon_pid, SIGINT);
            *liste = listeSupprimerTete(*liste);
            processus_en_cours = 0;
        }
    }
}
