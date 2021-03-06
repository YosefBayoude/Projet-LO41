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

#define FILE_MESSAGES_TYPE 1
#define CLE 314




//***** Variables modifiables *****//
#define NBRES_DE_PRIORITE 10
#define QUANTUM_DE_TEMPS 2 // en sec
#define TEMPS_EXEC_MAX 5


int* temps; // Variable accedé par le pere ET les fils


struct sembuf sem_oper_P ;  /* Operation P */
struct sembuf sem_oper_V ;  /* Operation V */


//***** Variables fonctionnement algo ordonnancement *****//
int liste_priorite[100] = {5,0,5,9,7,0,9,8,7,5,1,9,6,7,0,8,4,4,6,8,1,3,0,5,0,5,0,9,1,6,6,2,6,3,1,4,5,3,3,4,3,7,0,6,7,2,4,4,8,9,7,2,0,4,5,0,3,7,3,5,5,9,6,6,7,8,5,1,9,1,3,7,1,4,6,1,1,9,7,1,5,9,7,6,7,0,7,8,3,1,6,7,2,9,0,7,1,0,2,0};
int priorite_courante = -1;
int position_liste_priorite = 0;
int processus_en_cours = 0;

int pid_generateur_processus;

int msgid;
int semid;
int shmid;





void traitantSIGINT();
void generateurDeProcessus();
void ordonnanceur();
void gererProcessus(Element* liste[NBRES_DE_PRIORITE], FILE* sortie);
void supprimerProcessusTermines(Element* tableau[NBRES_DE_PRIORITE]);
void ecrireResultat(FILE* fichier, Processus* p, int mode);

/**
 * Initialise les semaphores
 */
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

/**
 * Initialise la memoire partagé
 */
void initShm(){
    shmid = shmget(CLE, sizeof(int), 0666|IPC_CREAT);
	temps = (int *)shmat(shmid, NULL, 0);
}

/**
 * Initialise la file de message
 */
void initMsg(){
    if ((msgid = msgget(CLE, 0750 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("Erreur creation file de message\n");
    }
}

/**
 * Suppression des objets IPC
 */
void supprimerMessages(){ msgctl(msgid, IPC_RMID, NULL); }
void supprimerSemaphores(){ semctl(semid, 0, IPC_RMID, NULL); }
void supprimerMemoirePartage(){ shmctl(shmid, IPC_RMID, NULL); }

void P(int semnum) {
    sem_oper_P.sem_num = semnum;
    sem_oper_P.sem_op  = -1 ;
    sem_oper_P.sem_flg = 0 ;
    semop(semid, &sem_oper_P, 1);
}

void V(int semnum) {
    sem_oper_V.sem_num = semnum;
    sem_oper_V.sem_op  = 1 ;
    sem_oper_V.sem_flg  = 0 ;
    semop(semid, &sem_oper_V, 1);
}


/**
 * Genere un nombre aleatoire entre min et max
 */
int randomNumber(int min, int max){
    if (min > max) perror("Erreur randomNumber\n");
    return (rand() %(max - min + 1)) + min;
}


void traitantSIGINT(int num){
    printf("Arret du program par SIGINT\n");
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


/**
 * Traitant pour les processus qui passent par l'ordonnanceur
 */
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


/**
 * Genere les données d'un processus, 
 * crée une priorite aleatoire, un temps d'exec aleatoire, ...
 */
Processus* genererProcessus(){
    Processus* prc = (Processus*)malloc(sizeof(Processus));;
    prc->type = FILE_MESSAGES_TYPE;
    prc->priorite = randomNumber(0, NBRES_DE_PRIORITE - 1);
    prc->temps_exec = randomNumber(1, TEMPS_EXEC_MAX);
    prc->mon_pid = getpid();
    prc->date_soumission = *temps;
    return prc;
}


/**
 * Affiche toutes les files de toutes les priorites, et les processus de chaque file
 */
void afficherFiles(Element* tableau[NBRES_DE_PRIORITE]){
    printf("Files d'attentes\n");
    for (int i = 0; i < NBRES_DE_PRIORITE; i++) {
            printf("File %d : ", i);
            printListeProcessus(tableau[i]);
    }
}



int tableauEstVide(Element* tableau[NBRES_DE_PRIORITE]){
    for(int i = 0; i < NBRES_DE_PRIORITE; i++){
        if (tableau[i] != NULL){
            return 0;
        }
    }
    return 1;
}


/**
 * Fait le tour de la liste des priorite,
 * il incremente la position de 1 à chaque fois,
 * et si il arrive a la derniere position il repart à 0
 *  - position_liste_priorite : variable globale
 */
int prochainePositionDansListeDePriorite(){
    if(position_liste_priorite == 99) position_liste_priorite = 0;
    else position_liste_priorite++;
    
    return position_liste_priorite;
}


/**
 * Fait le tour des priorités (= les files) en fonction du parametre,
 * il incremente la position de 1 à chaque fois,
 * et si il arrive a la derniere file, il repart à 0
 */
int prochainePriorite(int priorite){
    if(priorite >= (NBRES_DE_PRIORITE - 1)) priorite = 0;
    else priorite++;
    
    return priorite;
}


/**
 * Calcule la prochaine priorité en fonction du tableau.
 * Il cherche la position courante (position_liste_priorite) de la liste la priorité donné, 
 * si la file (de la priorité donné) est vide, il passe à la priorité du dessus
 * si il y a un processus dans la file, on renvoie la priorité calculé
 */
int calculerPriorite(Element* tableau[NBRES_DE_PRIORITE]){
    
    priorite_courante = liste_priorite[prochainePositionDansListeDePriorite()]; //On cherche la prochaine priorite de la liste de priorite

    printf("\n%sPriorite suivante : %d%s  ",BOLDCYAN, priorite_courante, NORMAL);

    Element *e = listeValeurTete(tableau[priorite_courante]);
    while(e == NULL){ //On cherche si il y a un processus avec la priorite = priorite_courante
        priorite_courante = prochainePriorite(priorite_courante); //On change la priorite si il n'y a pas de procesuss dans la file "priorite_courante"
        e = listeValeurTete(tableau[priorite_courante]);
    }

    printf("%sDevenu : %d%s\n",BOLDBLUE, priorite_courante, NORMAL);

    return priorite_courante;
}


/**
 * Cherche le nombre de messages dans la file de messages
 */
int nombreMessages(int msgid){
    struct msqid_ds buf;
    int nb_messages = msgctl(msgid, IPC_STAT, &buf);
    return buf.msg_qnum;
}


/**
 * Recupere les données des processus crées, dans la file de message,
 * et les ajoute dans le tableau (à la bonne position)
 */
void receptionMessages(Element* tableau[NBRES_DE_PRIORITE], FILE* sortie){
    while (nombreMessages(msgid) > 0){ //Tant qu'il y a des messages
        Processus* p = (Processus*)malloc(sizeof(Processus));
        msgrcv(msgid, p, sizeof(Processus) - sizeof(long), FILE_MESSAGES_TYPE, 0);
        tableau[p->priorite] = listeAjouterQueue(tableau[p->priorite], p);
        printf("%sProcessus %d avec priorite %d est arrive, temps d'exec %d, temps d'arrive %d: %s\n",GREEN, p->mon_pid, p->priorite, p->temps_exec, p->date_soumission, NORMAL);           
        ecrireResultat(sortie, p, 1);
    }
    printf("\n");
}

FILE* ouvrirFichierSortie(){
    FILE* file;
    file = fopen("resultat_executions.txt", "w");
    if (file == NULL){
        printf("Erreur ouverture de fichier\n");
        return NULL;
    }
    return file;
}

void ecrireResultat(FILE* fichier, Processus* p, int mode){
    if(fichier == NULL){
        printf("Erreur ecriture de fichier");
        return;
    }

    if (mode == 1) fprintf(fichier, "------------ P: %d(p = %d, t = %d) arrivé\n", p->mon_pid, p->priorite, p->temps_exec);
    if (mode == 0) fprintf(fichier, "P: %d(p = %d, t = %d)\n", p->mon_pid, p->priorite, p->temps_exec);
    
}


/**
 * Permet a l'utilisateur de fournir sa propre liste de priorite
 */
int ouvrirFichierPriorite(){
    FILE* file;
    file = fopen("liste_priorites.txt", "r");
    if (file == NULL){
        printf("Erreur ouverture de fichier\n");
        return -1;
    }
    char virgule = ',';
    for (int i = 0; i < 100; i++)
    {
        fscanf(file, "%d", &liste_priorite[i]);
        if(virgule != ','){ return -1; };
        fscanf(file, "%c", &virgule);
    }
    fclose(file);
    return 0;
   
}


int interface(){
    int choix = -1;

    printf("====== Menu ======\n");
    printf("1) Jeu d'essai par defaut\n");
    printf("2) Tableau de priorite par fichier (liste_priorites.txt)\n");

    scanf("%d", &choix);

    switch (choix)
    {
    case -1:
        printf("Erreur choix, essayez a nouveau\n");
        return -1;
        break;
    case 1:
        return 0;
        break;
    
    case 2:
        if ( ouvrirFichierPriorite() ) {
            printf("Erreur fichier, essayez a nouveau\n");
            return -1;
        };
        return 0;
        break;
    
    default:
        break;
    }   
}


/**
 * main(), lance le processus de generation des processus et l'ordonnanceur,
 * initialise l'environnement
 */
int main()
{
    //***** Verification des variables  *****//
    if(TEMPS_EXEC_MAX < 1) {perror("Temps d'exec max est trop petit"); exit(EXIT_FAILURE);}
    if(NBRES_DE_PRIORITE < 1) {perror("Nombres de priorites trop petit"); exit(EXIT_FAILURE);}

    
    //***** Traitant signal *****//
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);

    signal(SIGSEGV, traitantSIGSEGV); //debug

    //***** Initialisations objets IPC ******//
    initMsg();
    initSem();
    initShm();    


    //***** Initialisations variables ******//
	*temps = 0; //Compteur de temps (symbolise le quantum de temps)
    srand(time(0)); //Generateur nombre aleatoire


    printf("Lancement du programme\n");

    FILE* sortie = ouvrirFichierSortie();
    while (interface()){;};
    

    //***** Lancement des programmes *****//
    pid_generateur_processus = fork();  
    switch (pid_generateur_processus)
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
            ordonnanceur(sortie);
            break;
    }

    
    //***** Supprimer les objets IPC *****//
    supprimerMessages();
	supprimerSemaphores();
    supprimerMemoirePartage();


    printf("Fin du programme\n");


    return 0;
}


/**
 * Genere des processus, et envoie leurs informations à l'ordonnanceur, par la file de message, 
 */
void generateurDeProcessus(){

    //***** Traitant signal *****//
    signal(SIGCHLD, SIG_IGN); //Evite que le fils attende le pere pour mourir = pour ne pas avoir de processus zombies

    //for(int i = O; i < 20; i++){ //Remplacer while par cette ligne si on veut qu'il s'arrete tout seul
    while(1){ 
        for (int i = 0; i < randomNumber(0, 3); i++) //Genere un nbre aleatoire de processus en une fois
        {
            int pid = fork();
            if (!pid){
                P(0);
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

                V(0);

                //***** Le proessus s'arrete immediatement apres avoir ete crée *****//
                kill(getpid(), SIGSTOP);


                //***** Simulation travail *****//
                while(1){ }
                
            
                printf("%sProcessus %d s'est termine par lui meme%s\n",RED, prc->mon_pid, NORMAL);
                exit(EXIT_SUCCESS);
            }
        }

        //***** Temps d'attente *****//
        sleep(3); //Pause entre la generation des processus
    }

    exit(0);
}


/**
 * Gere les processus, il fait cela en gardant un tableau,
 * chaque ligne represente un file,
 * et dans chaque ligne il y a N processus stockés
 */
void ordonnanceur(FILE* sortie){
    
    //***** Traitant signal *****//
    struct sigaction sa;
    sa.sa_handler = traitantSIGINT;
    sigemptyset(&(sa.sa_mask));
    sigaddset(&(sa.sa_mask), SIGINT);
    sigaction(SIGINT, &sa, NULL);
    

    //***** Initialisations du tableau ******//
    Element* tableau[NBRES_DE_PRIORITE] = {NULL};

    //for(int i = O; i < 100; i++){ //Remplacer while par cette ligne si on veut qu'il s'arrete tout seul
    while(1) {
        P(0);

        //***** Suppression des processus finis *****//
        supprimerProcessusTermines(tableau);

        
        //***** Affichage du tableau *****//
        afficherFiles(tableau);


        //***** Incrementation du temps *****//
        (*temps)++;
        printf("%sTemps courant : %d%s\n",BOLDWHITE, *temps, NORMAL);


        //***** Gestion des processus *****//
        gererProcessus(tableau, sortie);

        V(0);
        //***** Quantum de temps ******//
        sleep(QUANTUM_DE_TEMPS);

    }

}


/**
 * Algorithme d'ordonnancement,
 * cette fonction arrete et redemarre les processus en fonction de leur priorité et de leur temps d'execution.
 * Il repositionne egualement les processus à la bonne file une fois executé et decremente leur temps d'execution.
 */
void gererProcessus(Element* tableau[NBRES_DE_PRIORITE],FILE* sortie){

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
                processus_en_cours = p->mon_pid;
                p->temps_exec--;

                ecrireResultat(sortie, p, 0);

                receptionMessages(tableau, sortie);

                if(p->priorite < (NBRES_DE_PRIORITE-1)){ //Pour les  processus des files de 0 - 8, on incremente la priorité un fois executé
                    tableau[p->priorite + 1] = listeAjouterQueue(tableau[p->priorite + 1], listeValeurTete(tableau[priorite_courante])->data);
                    p->priorite++;
                    tableau[priorite_courante] = listeSupprimerTete(tableau[priorite_courante]);
                } else if (p->priorite == (NBRES_DE_PRIORITE-1)){ //Pour les processus de la file 9, on remet juste le processus à la fin de la meme file
                    tableau[p->priorite] = listeAjouterQueue(tableau[p->priorite], listeValeurTete(tableau[priorite_courante])->data);
                    tableau[p->priorite] = listeSupprimerTete(tableau[p->priorite]);
                }

            }
        }
    } else {
        receptionMessages(tableau, sortie);
    }
    
}

/**
 * Lorsqu'un processus avec un temps d'exectution 1 est lancé, ce temps est decrementé, mais ses données restent toujours dans le tableau,
 * cette fonction parcours le tableau et cherche les processus avec un temps d'execution = 0,
 * dés qu'il en trouve un, il arrete le processus, et le supprime du tableau.
 * Cette fonction est executé avant la gestion des processus.
 */
void supprimerProcessusTermines(Element* tableau[NBRES_DE_PRIORITE]){
    for (int i = 0; i < NBRES_DE_PRIORITE; i++){
        if (tableau[i]){
            if (listeValeurQueue(tableau[i])->data->temps_exec == 0){
                kill(listeValeurQueue(tableau[i])->data->mon_pid, SIGINT);
                kill(pid_generateur_processus, SIGCHLD); //Evite les processus zombies
                tableau[i] = listeSupprimerQueue(tableau[i]);
                processus_en_cours = 0;
            }
        }
    }
}