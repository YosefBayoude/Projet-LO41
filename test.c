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
            while(1){
                printf("OK\n");
                sleep(1);
            }
            break;
			
		default :
			printf("Je suis dans le processus pere : %d \n",getpid());
            while(1){
                kill(pid, SIGSTOP);
                sleep(4);
                kill(pid, SIGCONT);
                sleep(4);
            }
			
	}
}
		
			