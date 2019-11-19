#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


typedef struct type_processus
{
    int priorite;
    int date_soumission;
    int temps_exec;
} processus ;


int randomNumber(int max, int min){
    if (min > max) perror("Erreur randomNumber\n");
    return (rand() %(max - min + 1)) + min;
}


processus genererProcessus(){
    processus prc;
    prc.priorite = (rand() %(10 - 0 + 1)) + 0;
    prc.temps_exec = (rand() %(20 - 0 + 1)) + 0;
    return prc;
}


int main()
{
    printf("Lancement du programme\n");
    srand(time(0));



    printf("Creation du fils\n");
    int pid_fils = fork();  
    switch (pid_fils)
    {
        case -1:
            perror("Erreur fork");
            break;
        case 0:
            //FILS
            //Genere des processus
            for (int i = 0; i < randomNumber(10, 0); i++)
            {
                processus prc = genererProcessus();
                printf("Processus prio : %d\n", prc.priorite);
            }

            exit(0);
            break;
        default:
            //PERE
            //Gere l'algorithme d'ordonnancement
            break;
    }

    printf("Fin du programme\n");
    return 0;
}
