#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <sys/msg.h>


#define cle 314

typedef struct type_processus
{
    long type;
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

    int msgid;
    if ((msgid = msgget(cle, 0750 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("Erreur creation file de message\n");
    }


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


            
            for (int i = 0; i < randomNumber(10, 0); i++)
            {
                processus prc = genererProcessus();
                prc.type = getpid();
                msgsnd(msgid, &prc, sizeof(processus) - sizeof(long), 0);
                printf("Processus prio : %d\n", prc.priorite);
            }

            exit(0);
            }
            break;


        /****************************************************/
        default:
            //PERE
            //Gere l'algorithme d'ordonnancement
            {
            int status;
            wait(&status);
            processus liste_prc[20];

            struct msqid_ds buf;
            int nb_messages = msgctl(msgid, IPC_STAT, &buf);

            msgrcv(msgid, &liste_prc[0], sizeof(processus) - sizeof(long), pid_fils, 0);

            printf("Message recu = %d\n", liste_prc[0].priorite);

            printf("Il reste %ld messages dans la file\n", buf.msg_qnum);
            }
            break;
    }

    msgctl(msgid, IPC_RMID, NULL);

    printf("Fin du programme\n");
    return 0;
}
