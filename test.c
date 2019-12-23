//
// Exo1 by Ph.D
//
// Retourne le Status du processus
//

/***
 *
 * @param status : valeur  du fils
 * @param pid : valeur de retour du fork
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

void erreurFin(const char* msg){
    perror(msg);
}

void traitantSIGCHLD(int num){
    int wstatus;
    wait(&wstatus);
    printf("signal recu\n");
    printf("Le pid est %d\n", wstatus);
    exit(0);
}

int main() {
	pid_t pid ;
	int status ;
	
	pid = fork();
	switch(pid) {
		case -1 : 
			erreurFin("Erreur de Fork");
			exit(1);
            break;
		case 0 : 
			printf("Je suis dans le processus fils %d \n",getpid());
            //while(1){
                printf("OK\n");
                sleep(1);
            //}
            printf("fin\n");
            return getpid();
            break;
			
		default :
			printf("Je suis dans le processus pere : %d \n",getpid());
            //signal(SIGCHLD, traitantSIGCHLD);
            while(1){
                int * i= (int*)malloc(sizeof(int)) ;
                *i = 1;
                /*kill(pid, SIGSTOP);
                sleep(4);
                kill(pid, SIGCONT);
                sleep(4);*/
                *i=3;
                ;

            }
			
	}
}
		
			