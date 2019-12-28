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
#include <pthread.h>
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

int pid_fils_global;

int* temps;

pthread_mutex_t lock;

struct sembuf sem_oper_P ;  /* Operation P */
struct sembuf sem_oper_V ;  /* Operation V */

int priorite_courante = -1;
int liste_priorite[100] = {5,0,5,9,7,0,9,8,7,5,1,9,6,7,0,8,4,4,6,8,1,3,0,5,0,5,0,9,1,6,6,2,6,3,1,4,5,3,3,4,3,7,0,6,7,2,4,4,8,9,7,2,0,4,5,0,3,7,3,5,5,9,6,6,7,8,5,1,9,1,3,7,1,4,6,1,1,9,7,1,5,9,7,6,7,0,7,8,3,1,6,7,2,9,0,7,1,0,2,0};
int _position_liste_priorite = 0;

Element* processus_en_cours = NULL;

void traitantSIGINT();
void fils();
void ordonnaceur();
Element* gererListe(Element* liste[10]);
void supprimerProcessusTermines();


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


Processus* genererProcessus(){
    Processus* prc = (Processus*)malloc(sizeof(Processus));;
    prc->type = FILE;
    prc->priorite = randomNumber(0, 9);
    prc->temps_exec = randomNumber(2, 5);
    prc->mon_pid = getpid();
    return prc;
}


void traitantSIGINT(int num){
    printf("Arret du program\n");
    if (num){
        supprimerMessages();
	    supprimerSemaphores();
        supprimerMemoirePartage();
        exit(0);
    } else {
        perror("Erreur traitant, files non supprimé\n");
        exit(1);
    }
}

void traitantSIGINTfils(int num){
    printf("%sLe processus %d s'est terminé%s\n", RED, getpid(), NORMAL);
    exit(0);
}

void traitantSIGSEGV(int num){
    if (num == SIGSEGV){
        printf("Erreur segmentation fault\n");
        supprimerMessages();
	    supprimerSemaphores();
        supprimerMemoirePartage();
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

int prochainePositionDansListeDePriorite(){
    if(_position_liste_priorite == 99) _position_liste_priorite = 0;
    else _position_liste_priorite++;
    
    return _position_liste_priorite;
}

int prochainePriorite(int priorite){
    if(priorite == 9) priorite = 0;
    else priorite++;
    
    return priorite;
}


int tableauEstVide(Element* liste[10]){
    for(int i = 0; i < 10; i++){
        if (liste[i] != NULL){
            return 0;
        }
    }
    return 1;
}

int calculerPriorite(Element* tableau[]){
    
    priorite_courante = liste_priorite[prochainePositionDansListeDePriorite()]; //On cherche la prochaine priorite de la liste de priorite

    printf("%sPriorite courante (normalement) : %d%s  ",BOLDWHITE, priorite_courante, NORMAL);

    Element *e = listeValeurTete(tableau[priorite_courante]);
    while(e == NULL){ //On cherche si il y a un processus avec la priorite = priorite_courante
        priorite_courante = prochainePriorite(priorite_courante);
        e = listeValeurTete(tableau[priorite_courante]);
    }

    printf("%sDevenu : %d%s\n",BOLDWHITE, priorite_courante, NORMAL);

    return priorite_courante;
}

int main()
{
    printf("Lancement du programme\n");
    srand(time(0));
    
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);

    signal(SIGSEGV, traitantSIGSEGV);

    initMsg();
    initSem();
    initShm();    

	*temps = 0; //initialisation a 0 du compteur

    //processus_en_cours = listeCreer();

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
            pid_fils_global = pid_fils;
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
    
    signal(SIGCHLD, SIG_IGN); //Evite que le fils attende le pere pour mourir

    for(int i = 0; i < 10; i++) {
        //P();
        for (int i = 0; i < randomNumber(0, 3); i++) //Genere un nbre aleatoire de processus à la fois
        {


            int pid = fork();
            if (!pid){

                struct sigaction sa;
                sa.sa_handler = traitantSIGINTfils;
                sigemptyset(&(sa.sa_mask));
                sigaddset(&(sa.sa_mask), SIGINT);
                sigaction(SIGINT, &sa, NULL);

                if(sigaction(SIGINT, &sa, NULL) != 0)
                {
                    perror( "sigaction failed" );
                    exit( EXIT_FAILURE );
                }


                Processus* prc = genererProcessus();
                prc->date_soumission = *temps;
                msgsnd(msgid, prc, sizeof(Processus) - sizeof(long), 0);



                //kill(getpid(), SIGTSTP);
                //kill(getpid(), SIGCONT);
                kill(getpid(), SIGSTOP);



                while(1){ //simule du travail
                    //printf("%d : travail\n", getpid());
                    
                }
                
                printf("%sProcessus %d s'est termine%s\n",RED, prc->mon_pid, NORMAL);

                exit(EXIT_SUCCESS);

            }
        }
        //V();
        sleep(2);
    }

    exit(0);
}

void ordonnaceur(){
    
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);
    

    Element* liste[10] = {NULL};

    for(int i = 0; i < 200; i++) {
        //P();
        while (nombreMessages(msgid) > 0) //Tant qu'il y a des messages
        {
            Processus* p = (Processus*)malloc(sizeof(Processus));
            msgrcv(msgid, p, sizeof(Processus) - sizeof(long), FILE, 0);
            printf("%sProcessus %d avec priorite %d est arrive, temps d'exec %d, temps d'arrive %d: %s\n",GREEN, p->mon_pid, p->priorite, p->temps_exec, p->date_soumission, NORMAL);           
            liste[p->priorite] = listeAjouterQueue(liste[p->priorite], listeNouvelElement(p));
        }

        
        for (int i = 0; i < 10; i++) {
            printListeProcessus(liste[i]);
        }

        (*temps)++;
        printf("%sTemps courant : %d%s\n",BOLDWHITE, *temps, NORMAL);
        //V();

        processus_en_cours = gererListe(liste);

        sleep(2);

        supprimerProcessusTermines(); //Supprime les processus qui ont un temps d'execution

    }

}

Element* gererListe(Element* tableau[10]){

    printf("erreur ici");
    if (processus_en_cours){
            kill(processus_en_cours->data->mon_pid, SIGTSTP);
    }

    if(!tableauEstVide(tableau)){

        Element *e = listeValeurTete(tableau[calculerPriorite(tableau)]);

        if(e) {
            Processus *p = e->data;
            if(p->temps_exec > 0){
                kill(p->mon_pid, SIGCONT);
                p->temps_exec--;
                //processus_en_cours = e;

                if(p->priorite < (10-2)){
                    tableau[p->priorite + 1] = listeAjouterQueue(tableau[p->priorite + 1], listeNouvelElement( listeValeurTete(tableau[priorite_courante])->data ));
                    p->priorite++;
                    tableau[priorite_courante] = listeSupprimerTete(tableau[priorite_courante]);
                }

            }
            return e;
        }
    }
    return NULL;
    
}


void supprimerProcessusTermines(){
    if(processus_en_cours) {
        if (processus_en_cours->data){
            if(processus_en_cours->data->temps_exec == 0){
                kill(processus_en_cours->data->mon_pid, SIGINT);
                kill(pid_fils_global, SIGCHLD);
                processus_en_cours = listeSupprimerTete(processus_en_cours);
                processus_en_cours = NULL;
            }
        }
    }
}