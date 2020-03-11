# Projet-LO41

Le but est de créer un algorithme qui gère les processus entrants, et de les lancer en fonction de leur priorité et de la table d’allocation.



# Fonctionnement

L’ordonnanceur fonctionne de la façon suivante :
Tout d’abord l’ordonnanceur contient un tableau ou il mémorise les processus existants, avec leurs données. On peut le représenter de la façon suivante :

| Priorités | Processus |             |             |      |
| ---------- | ----------- | ----------- | ----------- | ---- |
| Priorité 0 | Processus X |             |             |      |
| Priorité 1 | Processus X |             |             |      |
| Priorité 2 | Processus X | Processus X | Processus X |      |
| Priorité 3 |             |             |             |      |
| Priorité 4 | Processus X |             |             |      |
| Priorité 5 |             |             |             |      |
| Priorité 6 | Processus X | Processus X |             |      |
| Priorité 7 |             |             |             |      |
| Priorité 8 | Processus X |             |             |      |
| Priorité 9 | Processus X | Processus X |             |      |

*(Chaque processus X représente un processus diffèrent)*



Ces processus sont ajoutés au fur et mesure qu’ils arrivent. Le rôle de l’ordonnanceur est d’exécuter / arrêté le processus en fonction de la table d’allocation. La table peut être représenter comme ceci :

| Temps    |  0   |  1   |  2   |  3   |  4   |  5   |  6   |  7   |  …   |  99  |
| :------- | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: |
| Priorité |  0   |  1   |  2   |  3   |  0   |  4   |  5   |  0   |  …   |  8   |

L’ordonnanceur regarde sur quel temps où il se trouve couramment, et regarde la priorité à appliquer grâce à la table d’allocation. S’il y a un processus dans la file (dans le tableau) de la priorité à appliquer, il arrête le processus en cours et lance le processus du début de la file durant un quantum de temps. Sinon, il prend un processus de la file suivante. Une fois ce processus lancé, il regarde s’il y a des nouveaux processus arrivés. Ces derniers sont ajoutés à la fin des files du tableau. Une fois le quantum de temps écoulé, il ajoute le processus qui vient de s’exécuter à la fin de la file de la priorité suivante et diminue sont temps d’exécution. Le temps courant augmente et l’ordonnanceur recommence depuis le début.

Quelques points à noter :

- Si à la dernière file il n’y a toujours pas de processus à exécuter, il fait le tour et repart de la file 0.
- Une fois arrivé à la fin de la table d’allocation, il recommence à 0.
- Quand les processus de la dernière file sont lancés, ils sont juste déplacés à la fin de cette même file.



# Utilisation

Mon programme contient une interface simple qui permet à l’utilisateur de de choisir entre deux modes de fonctionnement.

Il y a un mode « jeu d’essai », fait avec une table d’allocation intégré dans le code, et un mode utilisateur, ou l’utilisateur peut fournir sa propre table d’allocation. Pour cela il suffit d’insérer la liste des priorités, dans l’ordre de la table d’allocation des processus, dans le fichier texte nommé « liste_priorites.txt ». Le fichier contient des instructions sur comment l’utiliser.
En sortie, le programme affiche le déroulement du travail effectué sur le terminal, en affichant les files, le quantum de temps actuel, les processus arrivés, le processus terminés ainsi que la priorité suivante.
Le programme enregistre aussi l’ordre d’exécution dans un fichier externe appelé « resultat_executions.txt ». Cela permet une vérification plus pratique du résultat.
Une fois le programme lancé, il ne s’arrête jamais, pour le stopper il suffit d’envoyer un SIGINT (CTRL + C), et le programme se termine comme il faut. Cependant, si on le souhaite, il y a 2 lignes de code commentés, qu’il faut dé-commenté pour permettre au programme de s’arrêter au bout de N boucles.



# Outils

Ce programme contient deux processus principaux :

Un pour jouer le rôle d’ordonnanceur

Un pour générer des processus en boucle de façon aléatoire, afin de simuler un système utilisé par un vrai utilisateur
Les deux sont lancés par la fonction main().

**Communication :**
Pour communiquer, mon programme utilise les files de messages. Utiliser les files de messages permet aux processeurs de communiquer entre eux sans devoir se partager de variables ou de se manipuler les données entre eux.
Elle a l’avantage aussi d’avoir une gestion facile, il est possible de voir si des messages sont arrivées sans devoir bloquer le programme.

**Gestion des processus :**
Pour permettre à l’ordonnanceur de gérer les processus, on avait que le choix d’utiliser les signaux. L’ordonnanceur lance un signal pour continuer ou pour s’arrêter au processus correspondant. Les signaux permettent de contrôler les processus tel comme un vrai système l’aurait fait. Dans ce projet ils sont donc utilisés pour continuer, mettre en pause, ou terminer un processus.

**Gestion de temps :**
Tous les processus se partagent une variable, celle du temps. Pour cela, j’ai créé une variable dans la mémoire partagée. Elle accédé par pratiquement par tous les processus. Cette variable permet aux processus de savoir quand elle est a été créé.

**Synchronisation :**
Enfin pour synchroniser cette dernière variable, j’ai utilisé les sémaphores. On effet, on travaille avec des processus et pour faire cela il convient d’utiliser les sémaphores.
