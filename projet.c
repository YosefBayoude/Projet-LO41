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

#define NBRE_PRIORITE 10

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

int processus_en_cours = 0;

void traitantSIGINT();
void generateurDeProcessus();
void ordonnaceur();
void gererProcessus(Element* liste[NBRE_PRIORITE]);
void supprimerProcessusTermines(Element* tableau[NBRE_PRIORITE]);


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
    prc->priorite = randomNumber(0, NBRE_PRIORITE - 1);
    prc->temps_exec = randomNumber(2, 5);
    prc->mon_pid = getpid();
    prc->date_soumission = *temps;
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
    printf("%sLe processus %d s'est termine%s\n", RED, getpid(), NORMAL);
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



int tableauEstVide(Element* liste[NBRE_PRIORITE]){
    for(int i = 0; i < NBRE_PRIORITE; i++){
        if (liste[i] != NULL){
            return 0;
        }
    }
    return 1;
}



int prochainePositionDansListeDePriorite(){
    if(_position_liste_priorite == 99) _position_liste_priorite = 0;
    else _position_liste_priorite++;
    
    return _position_liste_priorite;
}



int prochainePriorite(int priorite){
    if(priorite == (NBRE_PRIORITE - 1)) priorite = 0;
    else priorite++;
    
    return priorite;
}



int calculerPriorite(Element* tableau[NBRE_PRIORITE]){
    
    priorite_courante = liste_priorite[prochainePositionDansListeDePriorite()]; //On cherche la prochaine priorite de la liste de priorite

    printf("%sPriorite courante : %d%s  ",BOLDCYAN, priorite_courante, NORMAL);

    Element *e = listeValeurTete(tableau[priorite_courante]);
    while(e == NULL){ //On cherche si il y a un processus avec la priorite = priorite_courante
        priorite_courante = prochainePriorite(priorite_courante); //On change la priorite si il n'y a pas de procesuss dans la file "priorite_courante"
        e = listeValeurTete(tableau[priorite_courante]);
    }

    printf("%sDevenu : %d%s\n",BOLDBLUE, priorite_courante, NORMAL); 

    return priorite_courante;
}



void afficherFiles(Element* tableau[NBRE_PRIORITE]){
    for (int i = 0; i < NBRE_PRIORITE; i++) {
            printf("File %d : ", i);
            printListeProcessus(tableau[i]);
    }
}



int nombreMessages(int msgid){
    struct msqid_ds buf;
    int nb_messages = msgctl(msgid, IPC_STAT, &buf);
    return buf.msg_qnum;
}



void receptionMessages(Element* tableau[NBRE_PRIORITE]){
    while (nombreMessages(msgid) > 0){ //Tant qu'il y a des messages
        Processus* p = (Processus*)malloc(sizeof(Processus));
        msgrcv(msgid, p, sizeof(Processus) - sizeof(long), FILE, 0);
        tableau[p->priorite] = listeAjouterQueue(tableau[p->priorite], p);
        printf("%sProcessus %d avec priorite %d est arrive, temps d'exec %d, temps d'arrive %d: %s\n",GREEN, p->mon_pid, p->priorite, p->temps_exec, p->date_soumission, NORMAL);           
    }
}



int main()
{
    
    //***** Traitant signal *****//
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);


    //***** Initialisations objets IPC ******//
    initMsg();
    initSem();
    initShm();    


    //***** Initialisations variables ******//
	*temps = 0; //Compteur de temps (symbolise le quantum de temps)
    srand(time(0)); //Generateur nombre aleatoire


    printf("Lancement du programme\n");
    

    //***** Lancement des programmes *****//
    pid_fils_global = fork();  
    switch (pid_fils_global)
    {
        case -1:
            perror("Erreur fork");
            break;

        case 0:
            //***** Genere des processus (fils) *****//
            generateurDeProcessus();
            break;

        default:
            //***** Gere l'algorithme d'ordonnancement (pere) *****//
            ordonnaceur();
            break;
    }

    
    //***** Supprimer les objets IPC *****//
    supprimerMessages();
	supprimerSemaphores();
    supprimerMemoirePartage();


    printf("Fin du programme\n");


    return 0;
}

void generateurDeProcessus(){

    //***** Traitant signal *****//
    signal(SIGCHLD, SIG_IGN); //Evite que le fils attende le pere pour mourir = pour ne pas avoir de processus zombies

    for(int i = 0; i < 3; i++) { //Nombre de fois qu'on genere des processuss
        //P();
        for (int i = 0; i < randomNumber(0, 3); i++) //Genere un nbre aleatoire de processus en une fois
        {
            int pid = fork();
            if (!pid){

                //***** Traitant signal *****//
                struct sigaction sa;
                sa.sa_handler = traitantSIGINTfils;
                sigemptyset(&(sa.sa_mask));
                sigaddset(&(sa.sa_mask), SIGINT);
                sigaction(SIGINT, &sa, NULL);


                //***** Generation des données processus *****//
                Processus* prc = genererProcessus();


                //***** Envoie de ses donnees a l'ordonnanceur *****//
                msgsnd(msgid, prc, sizeof(Processus) - sizeof(long), 0);


                //***** Le proessus s'arrete immediatement apres avoir ete crée *****//
                kill(getpid(), SIGSTOP);


                //***** Simulation travail *****//
                while(1){ }
                
            
                printf("%sProcessus %d s'est termine par lui meme%s\n",RED, prc->mon_pid, NORMAL);
                exit(EXIT_SUCCESS);

            }
        }

        //***** Temps d'attente *****//
        sleep(1.5); //Pause entre la generation des processus
    }

    exit(0);
}

void ordonnaceur(){
    
    //***** Traitant signal *****//
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);
    

    //***** Initialisations du tableau ******//
    Element* tableau[NBRE_PRIORITE] = {NULL};


    while(1) { //Ordonnance à l'infini
        //P();

        //***** Reception des donnees des processus qui arrivent *****//
        receptionMessages(tableau);

        
        //***** Affichage du tableau *****//
        afficherFiles(tableau);


        //***** Incrementation du temps *****//
        (*temps)++;
        printf("%sTemps courant : %d%s\n",BOLDWHITE, *temps, NORMAL);


        //***** Gestion des processus *****//
        gererProcessus(tableau);


        //***** Quantum de temps ******//
        sleep(2);

        //***** Suppression des processus finis *****//
        supprimerProcessusTermines(tableau); //Supprime les processus qui ont un temps d'execution de 0

    }

}

void gererProcessus(Element* tableau[NBRE_PRIORITE]){

    //***** Arrete le processus en cours *****//
    if (processus_en_cours){
            kill(processus_en_cours, SIGTSTP);
    }


    //***** Algorithme ordonnanceur *****//
    if(!tableauEstVide(tableau)){

        Element *e = listeValeurTete(tableau[calculerPriorite(tableau)]);

        if(e) {
            Processus *p = e->data;
            if(p->temps_exec > 0){
                kill(p->mon_pid, SIGCONT); //Ralance le processus
                p->temps_exec--;

                if(p->priorite < (NBRE_PRIORITE-1)){
                    tableau[p->priorite + 1] = listeAjouterQueue(tableau[p->priorite + 1], listeValeurTete(tableau[priorite_courante])->data);
                    p->priorite++;
                    tableau[priorite_courante] = listeSupprimerTete(tableau[priorite_courante]);
                } else if (p->priorite == (NBRE_PRIORITE-1)){
                    tableau[p->priorite] = listeAjouterQueue(tableau[p->priorite], listeValeurTete(tableau[priorite_courante])->data);
                    tableau[p->priorite] = listeSupprimerTete(tableau[p->priorite]);
                }

            }
        }
    }
    
}


void supprimerProcessusTermines(Element* tableau[NBRE_PRIORITE]){
    for (int i = 0; i < NBRE_PRIORITE; i++){
        if (tableau[i]){
            if (listeValeurQueue(tableau[i])->data->temps_exec == 0){
                kill(listeValeurQueue(tableau[i])->data->mon_pid, SIGINT);
                kill(pid_fils_global, SIGCHLD); //Evite les processus zombies
                tableau[i] = listeSupprimerQueue(tableau[i]);
                processus_en_cours = 0;
            }
        }
    }
}