# Note complète — Architecture d’un système d’exploitation : Ordonnancement

> Cours : **Architecture d’un système d’exploitation — Ordonnancement**  
> Thèmes principaux : processus, primitives UNIX, `fork`, `wait`, `execve`, IPC, signaux, sémaphores, threads, algorithmes d’ordonnancement, ordonnancement multiprocesseur, ordonnanceurs Linux `O(n)`, `O(1)` et **CFS**.

---

## 1. Idée générale du cours

L’**ordonnancement** est le mécanisme par lequel le système d’exploitation décide **quel processus ou thread doit utiliser le processeur à un instant donné**.

Dans un système moderne, plusieurs programmes semblent s’exécuter en même temps. En réalité, sur un processeur donné, un seul flot d’exécution est actif à un instant précis. Le rôle de l’ordonnanceur est donc de partager le temps processeur entre les différentes tâches.

Les grands objectifs de l’ordonnancement sont :

- maximiser l’utilisation du processeur ;
- réduire le temps de réponse pour les applications interactives ;
- éviter qu’un processus soit bloqué indéfiniment ;
- respecter les priorités ;
- gérer efficacement les systèmes multiprocesseurs ;
- limiter les coûts de changement de contexte ;
- garder une certaine équité entre les processus.

---

## 2. Les processus

### 2.1 Définition

Un **processus** correspond à l’**exécution d’un programme**.

Un programme seul est un fichier passif stocké sur disque. Lorsqu’il est lancé, le système crée un processus contenant :

- le code du programme ;
- ses données ;
- sa pile ;
- son tas ;
- ses registres ;
- ses fichiers ouverts ;
- ses signaux ;
- ses statistiques ;
- sa priorité ;
- sa table des pages.

Un processus possède son propre **espace d’adressage**, ce qui permet de l’isoler des autres processus.

---

### 2.2 Processus et modes d’exécution

Un processus peut s’exécuter en deux modes.

#### Mode utilisateur

Dans ce mode, le processus exécute des instructions ordinaires. Il ne peut accéder qu’à son propre espace mémoire.

Exemples :

- calculs ;
- appels de fonctions classiques ;
- manipulation de variables ;
- traitement local de données.

#### Mode noyau

Le mode noyau est utilisé lorsque le processus demande un service au système d’exploitation via un **appel système**.

Exemples :

- lire un fichier ;
- écrire dans un fichier ;
- créer un processus ;
- allouer certaines ressources ;
- communiquer avec un périphérique.

Le mode noyau permet d’exécuter des instructions privilégiées.

---

### 2.3 Pourquoi les processus sont importants ?

Dans UNIX/Linux, les processus sont fondamentaux parce qu’ils permettent :

- le **multitâche** ;
- le **multiutilisateur** ;
- le **cloisonnement** entre programmes ;
- la protection mémoire ;
- l’exécution concurrente ;
- la communication contrôlée entre programmes.

Ils reposent sur des mécanismes bas niveau comme :

- la pagination mémoire ;
- le TLB ;
- les primitives de synchronisation ;
- `test_and_set` ;
- `compare_and_swap`.

---

## 3. Informations contenues dans un processus

Un processus contient plusieurs catégories d’informations.

### 3.1 Registres

Les registres représentent l’état courant du processeur pour ce processus :

- pointeur d’instruction ;
- pointeur de pile ;
- registres entiers ;
- registres flottants ;
- registres système.

Lors d’un changement de contexte, ces registres doivent être sauvegardés puis restaurés.

---

### 3.2 Informations système

Le système associe aussi au processus :

- une table des pages ;
- des descripteurs de fichiers ;
- des statistiques d’exécution ;
- des signaux ;
- une priorité ;
- un état ;
- un identifiant de processus.

---

### 3.3 Organisation mémoire classique d’un processus

Un processus contient généralement :

```text
+----------------------+
| Pile                 |
+----------------------+
| Tas                  |
+----------------------+
| Variables globales   |
+----------------------+
| Code                 |
+----------------------+
```

- **Code** : instructions du programme.
- **Variables globales** : données statiques.
- **Tas** : mémoire dynamique, par exemple `malloc`.
- **Pile** : appels de fonctions, variables locales, adresses de retour.

---

## 4. Primitives générales d’identification

Les primitives UNIX permettent de récupérer l’identité du processus et de l’utilisateur.

### 4.1 Fichier d’en-tête

```c
#include <unistd.h>
```

---

### 4.2 Identité du processus

```c
pid_t getpid(void);
```

Renvoie le PID du processus courant.

```c
pid_t getppid(void);
```

Renvoie le PID du processus père.

---

### 4.3 Identité utilisateur

```c
uid_t getuid(void);
```

Renvoie l’utilisateur réel.

```c
uid_t geteuid(void);
```

Renvoie l’utilisateur effectif.

```c
gid_t getgid(void);
```

Renvoie le groupe réel.

```c
gid_t getegid(void);
```

Renvoie le groupe effectif.

Remarque : l’utilisateur effectif est important pour les droits d’accès aux fichiers.

---

## 5. Statistiques de temps

### 5.1 Fichier d’en-tête

```c
#include <sys/times.h>
```

---

### 5.2 Fonction `times`

```c
clock_t times(struct tms *buf);
```

Elle remplit une structure `tms`.

```c
struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};
```

### 5.3 Signification

| Champ | Signification |
|---|---|
| `tms_utime` | temps CPU du processus en mode utilisateur |
| `tms_stime` | temps CPU du processus en mode système |
| `tms_cutime` | temps utilisateur des fils terminés et attendus |
| `tms_cstime` | temps système des fils terminés et attendus |

Attention : ce temps ne correspond pas forcément au **temps réel écoulé**. Il correspond au temps CPU consommé.

---

## 6. Création et destruction de processus

### 6.1 Création : `fork`

La création d’un processus se fait avec l’appel système :

```c
pid_t fork(void);
```

`fork` crée un processus fils en dupliquant le processus parent.

Le processus fils obtient :

- son propre PID ;
- le PID du parent comme PPID ;
- une copie de l’espace mémoire ;
- une copie des descripteurs de fichiers ;
- ses propres statistiques réinitialisées.

---

### 6.2 Copy-on-write

Sous Linux, `fork` utilise le mécanisme de **copy-on-write**.

Cela signifie que les pages mémoire ne sont pas copiées immédiatement. Elles sont partagées tant qu’aucun processus ne les modifie. Si le parent ou le fils écrit dans une page, alors cette page est réellement copiée.

Avantage : `fork` est beaucoup plus rapide et consomme moins de mémoire au moment de l’appel.

---

### 6.3 Valeur de retour de `fork`

| Cas | Valeur retournée |
|---|---|
| Dans le parent | PID du fils |
| Dans le fils | `0` |
| En cas d’erreur | `-1` |

Exemple :

```c
#include <unistd.h>
#include <stdio.h>

int main(void) {
    pid_t pid = fork();

    if (pid > 0) {
        printf("Je suis le père, pid fils = %d\n", pid);
    } else if (pid == 0) {
        printf("Je suis le fils\n");
    } else {
        perror("fork");
    }

    return 0;
}
```

---

### 6.4 Erreurs possibles

| Erreur | Signification |
|---|---|
| `ENOMEM` | mémoire insuffisante pour créer le processus |
| `EAGAIN` | impossible de trouver une entrée disponible dans la table des processus |

---

### 6.5 Destruction d’un processus

Un processus peut se terminer par :

- un appel à `exit` ;
- la fin de la fonction `main` ;
- une erreur, par exemple segmentation fault ;
- un signal, par exemple `SIGKILL`.

---

## 7. Détails importants sur `fork`

Après un `fork`, parent et fils sont très proches, mais pas identiques.

### 7.1 Ce qui change

Le fils possède :

- un PID différent ;
- un PPID correspondant au PID du parent ;
- des statistiques CPU remises à zéro ;
- un ensemble de signaux pendants initialement vide.

---

### 7.2 Ce qui n’est pas hérité

Le fils n’hérite pas de certains éléments :

- certains verrouillages mémoire ;
- les signaux en attente ;
- certains ajustements de sémaphores ;
- certains temporisateurs ;
- certaines opérations d’entrée/sortie asynchrones.

---

### 7.3 Ce qui est hérité

Le fils hérite notamment :

- des descripteurs de fichiers ouverts ;
- de l’espace d’adressage virtuel initial ;
- des variables au moment du `fork` ;
- de l’environnement ;
- du répertoire courant.

Important : les descripteurs de fichiers du père et du fils peuvent pointer vers la même description de fichier ouvert. Ils partagent donc certains attributs comme la position courante dans le fichier.

---

## 8. Les états d’un processus

Un processus peut passer par plusieurs états.

### 8.1 États principaux

```text
Nouveau → Prêt → Actif utilisateur → Actif noyau → Endormi
                         ↓
                      Zombi
```

On peut aussi avoir un état **suspendu**.

---

### 8.2 Nouveau

Le processus vient d’être créé.

---

### 8.3 Prêt

Le processus a les ressources nécessaires pour s’exécuter, mais il attend que l’ordonnanceur le choisisse.

---

### 8.4 Actif utilisateur

Le processus s’exécute en mode utilisateur.

---

### 8.5 Actif noyau

Le processus s’exécute en mode noyau, souvent à cause d’un appel système ou d’une interruption.

---

### 8.6 Endormi

Le processus attend un événement.

Exemples :

- fin d’une entrée/sortie ;
- disponibilité d’une ressource ;
- réception d’un signal ;
- fin d’un processus fils.

---

### 8.7 Suspendu

Le processus est stoppé par un signal comme :

- `SIGSTOP` ;
- `SIGTSTP`.

Il peut reprendre avec :

- `SIGCONT`.

---

### 8.8 Zombi

Un processus zombi est un processus terminé dont le père n’a pas encore récupéré le statut de terminaison avec `wait` ou `waitpid`.

---

## 9. `wait`

### 9.1 Rôle

`wait` permet à un processus père d’attendre la fin d’un de ses fils.

### 9.2 Fichiers d’en-tête

```c
#include <sys/types.h>
#include <sys/wait.h>
```

### 9.3 Prototype

```c
pid_t wait(int *pointer_status);
```

---

### 9.4 Comportement

| Situation | Résultat |
|---|---|
| Aucun fils | `wait` renvoie `-1`, `errno = ECHILD` |
| Un fils zombi existe | `wait` renvoie son PID |
| Aucun fils terminé | le père est bloqué |
| Signal reçu | `wait` peut renvoyer `-1`, `errno = EINTR` |

---

## 10. `waitpid`

### 10.1 Prototype

```c
pid_t waitpid(pid_t pid, int *pointer_status, int options);
```

`waitpid` est plus précis que `wait`, car il permet de choisir quel fils attendre.

---

### 10.2 Valeurs de `pid`

| Valeur | Signification |
|---|---|
| `pid < -1` | attend un fils dans le groupe `|pid|` |
| `pid = -1` | attend n’importe quel fils |
| `pid = 0` | attend un fils du même groupe que l’appelant |
| `pid > 0` | attend le fils dont le PID vaut `pid` |

---

### 10.3 Valeurs de retour

| Retour | Signification |
|---|---|
| `-1` | erreur |
| `0` | aucun fils terminé en mode non bloquant |
| PID du fils | processus fils pris en compte |

---

## 11. `execve`

### 11.1 Rôle

`execve` remplace le programme courant par un autre programme.

Contrairement à `fork`, `execve` ne crée pas un nouveau processus. Il garde le même PID mais remplace :

- le code ;
- les données ;
- la pile ;
- le tas ;
- l’image mémoire du processus.

---

### 11.2 Prototype

```c
#include <unistd.h>

int execve(const char *fichier, char *const argv[], char *const envp[]);
```

---

### 11.3 Arguments

| Argument | Rôle |
|---|---|
| `fichier` | chemin du programme à exécuter |
| `argv` | tableau des arguments |
| `envp` | tableau des variables d’environnement |

`argv` et `envp` doivent se terminer par `NULL`.

---

### 11.4 Retour

En cas de succès, `execve` **ne revient pas**.

En cas d’échec, il renvoie `-1`.

---

### 11.5 Héritage après `execve`

Le processus garde :

- son PID ;
- certains descripteurs de fichiers ;
- certains attributs.

Mais le programme exécuté remplace l’ancien code.

---

## 12. Utilisation classique : `fork` + `exec`

Le schéma classique sous UNIX est :

1. le processus père appelle `fork` ;
2. le fils prépare son environnement ;
3. le fils redirige éventuellement ses fichiers ;
4. le fils appelle `execve` ;
5. le père appelle `wait` ou continue son travail.

Schéma :

```text
Père
 |
 | fork()
 |
 +---- Fils
 |       |
 |       | redirections éventuelles
 |       | execve()
 |       v
 |    nouveau programme
 |
 | wait()
 v
Père continue
```

Exemple :

```c
#include <unistd.h>
#include <stdio.h>

int main(void) {
    pid_t pid = fork();

    if (pid == 0) {
        char *args[] = {"/bin/date", NULL};
        execv(args[0], args);
        perror("execv");
    } else {
        printf("Processus père\n");
        sleep(5);
    }

    return 0;
}
```

---

## 13. Arborescence de processus UNIX

Dans UNIX, les processus forment une arborescence.

Chaque processus, sauf le processus initial, possède un père.

Exemple simplifié :

```text
init / systemd
 ├── bash
 │    ├── gcc
 │    └── ./programme
 └── sshd
      └── session utilisateur
```

Cette organisation permet :

- de gérer les relations père/fils ;
- de récupérer les processus terminés ;
- de propager certains signaux ;
- d’organiser les sessions et groupes de processus.

---

## 14. Les IPC : Inter-Process Communications

Les IPC permettent à plusieurs processus de communiquer ou de se synchroniser.

Le cours cite notamment :

- les signaux ;
- la mémoire partagée ;
- `mmap` ;
- les mutex ;
- les sémaphores.

---

## 15. Les signaux

### 15.1 Définition

Un signal est une **interruption logicielle** envoyée à un processus.

Il sert à notifier un événement.

Exemples :

- interruption clavier ;
- terminaison demandée ;
- erreur mémoire ;
- alarme ;
- communication entre processus.

---

### 15.2 Signal pendant et signal délivré

Un signal est dit **pendant** s’il a été envoyé mais pas encore pris en compte.

Un signal est dit **délivré** lorsque le processus le prend effectivement en compte.

Un signal peut être perdu si un signal du même identifiant est déjà pendant.

---

### 15.3 Quelques signaux classiques

| Numéro | Nom | Description |
|---:|---|---|
| 1 | `SIGHUP` | fin de session |
| 2 | `SIGINT` | interruption clavier |
| 3 | `SIGQUIT` | quitter |
| 4 | `SIGILL` | instruction illégale |
| 5 | `SIGTRAP` | trace trap |
| 6 | `SIGABRT` | abort |
| 7 | `SIGBUS` | erreur bus |
| 8 | `SIGFPE` | exception arithmétique |
| 9 | `SIGKILL` | termine immédiatement |
| 10 | `SIGUSR1` | signal utilisateur 1 |
| 11 | `SIGSEGV` | violation mémoire |
| 12 | `SIGUSR2` | signal utilisateur 2 |
| 13 | `SIGPIPE` | écriture dans pipe sans lecteur |
| 14 | `SIGALRM` | alarme |
| 15 | `SIGTERM` | demande de terminaison |

---

### 15.4 Envoyer un signal avec `kill`

```c
int kill(pid_t pid, int sig);
```

| Valeur de `pid` | Effet |
|---|---|
| `pid > 0` | envoie au processus `pid` |
| `pid = 0` | envoie au groupe du processus appelant |
| `pid = -1` | envoie à presque tous les processus autorisés |
| `pid < -1` | envoie au groupe `|pid|` |

---

### 15.5 Envoyer un signal à soi-même avec `raise`

```c
int raise(int sig);
```

Exemple :

```c
raise(SIGINT);
```

---

### 15.6 Comportement à la réception d’un signal

Un signal peut provoquer :

- la terminaison du processus ;
- la terminaison avec génération d’un fichier core ;
- l’ignorance du signal ;
- la suspension du processus ;
- la continuation d’un processus stoppé ;
- l’exécution d’une fonction utilisateur appelée **handler**.

---

### 15.7 Gestion des handlers avec `sigaction`

Fichier d’en-tête :

```c
#include <signal.h>
```

Structure :

```c
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};
```

Primitive :

```c
int sigaction(int sig,
              const struct sigaction *p_action,
              struct sigaction *p_action_anc);
```

---

### 15.8 Exemple de handler simple

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Signal reçu : %d\n", sig);
}

int main(void) {
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, NULL);

    while (1) {
        pause();
    }

    return 0;
}
```

---

## 16. Les sémaphores

### 16.1 Définition

Un sémaphore est un entier utilisé pour synchroniser des processus ou des threads.

Sa valeur ne peut jamais devenir négative.

Deux opérations principales :

```c
sem_wait()
```

Décrémente le sémaphore. Si sa valeur est 0, le processus ou thread est bloqué.

```c
sem_post()
```

Incrémente le sémaphore et peut réveiller un processus ou thread bloqué.

---

### 16.2 Sémaphores nommés

Un sémaphore nommé possède un nom de la forme :

```text
/nom_semaphore
```

Il est ouvert avec :

```c
sem_open()
```

Puis utilisé avec :

```c
sem_wait()
sem_post()
```

Et fermé avec :

```c
sem_close()
```

Enfin, il peut être supprimé avec :

```c
sem_unlink()
```

---

### 16.3 Sémaphores anonymes

Un sémaphore anonyme n’a pas de nom.

Il doit être placé dans une zone mémoire partagée entre :

- threads d’un même processus ;
- ou plusieurs processus.

Il est initialisé avec :

```c
sem_init()
```

Détruit avec :

```c
sem_destroy()
```

---

## 17. Les threads

### 17.1 Définition

Un thread est un **processus léger**.

Un processus peut contenir plusieurs threads.

---

### 17.2 Éléments propres à un thread

Chaque thread possède :

- sa propre pile ;
- son propre contexte ;
- ses registres.

---

### 17.3 Éléments partagés entre threads d’un même processus

Les threads d’un même processus partagent :

- le même espace d’adressage ;
- le même code ;
- les mêmes variables globales ;
- le même tas ;
- les mêmes fichiers ouverts.

---

### 17.4 Différence processus / thread

| Élément | Processus | Thread |
|---|---|---|
| Espace d’adressage | séparé | partagé avec les autres threads |
| Isolation | forte | plus faible |
| Communication | plus coûteuse | directe via mémoire partagée |
| Création | plus coûteuse | plus légère |
| Changement de contexte | plus coûteux | moins coûteux |

---

## 18. Introduction à l’ordonnancement

L’ordonnanceur choisit quel processus prêt doit s’exécuter.

Un bon ordonnanceur cherche à équilibrer :

- performance ;
- réactivité ;
- équité ;
- priorité ;
- utilisation CPU ;
- faible coût de décision ;
- limitation de la famine.

---

## 19. Round Robin

### 19.1 Principe

Round Robin donne à chaque processus un petit quantum de temps.

Quand le quantum est terminé, le processus est replacé en fin de file s’il n’a pas fini.

Schéma :

```text
P1 → P2 → P3 → P1 → P2 → P3 → ...
```

---

### 19.2 Avantages

- Simple à comprendre.
- Équitable si tous les processus ont la même importance.
- Adapté au multitâche général.

---

### 19.3 Inconvénients

- Ne tient pas compte de l’importance des processus.
- Un processus critique peut attendre trop longtemps.
- Le choix du quantum est important :
  - trop petit : trop de changements de contexte ;
  - trop grand : mauvaise réactivité.

---

## 20. Ordonnancement par priorité

### 20.1 Principe

Chaque processus reçoit une priorité.

L’ordonnanceur choisit le processus prêt ayant la plus haute priorité.

Dans le cours :

- un petit nombre peut représenter une priorité élevée ;
- un grand nombre peut représenter une priorité faible.

---

### 20.2 Avantage

Ce mécanisme permet de représenter l’importance relative des processus.

Exemple : un processus critique peut être favorisé par rapport à un processus moins important.

---

### 20.3 Inconvénient : famine

Un processus de faible priorité peut ne jamais être choisi si des processus de priorité plus élevée sont toujours présents.

C’est le problème de **starvation**, ou famine.

---

## 21. Starvation

### 21.1 Définition

La famine apparaît lorsqu’un processus attend indéfiniment le processeur.

Cela peut arriver dans un ordonnanceur à priorité si les processus de haute priorité arrivent continuellement.

---

### 21.2 Solution : vieillissement des priorités

Le système peut augmenter progressivement la priorité des processus qui attendent longtemps.

Principe :

1. chaque processus possède une priorité de base ;
2. à chaque quantum, les processus qui attendent gagnent en priorité ;
3. lorsqu’un processus s’exécute, sa priorité est réinitialisée.

Cela garantit qu’un processus de faible priorité finira par s’exécuter.

---

## 22. Priorité statique et priorité dynamique

### 22.1 Priorité statique

La priorité statique est généralement fixée au lancement du processus.

Elle peut être donnée par l’utilisateur ou par défaut.

---

### 22.2 Priorité dynamique

La priorité dynamique peut changer pendant l’exécution.

Elle permet à l’ordonnanceur d’adapter son comportement.

Exemples :

- augmenter la priorité d’un processus interactif ;
- diminuer la priorité d’un processus très consommateur de CPU ;
- éviter la famine.

---

## 23. Multilevel Queues

### 23.1 Principe

Les processus sont répartis dans plusieurs files selon leur classe de priorité.

Chaque classe possède sa propre file de processus prêts.

L’ordonnanceur choisit d’abord la file de plus haute priorité non vide.

Ensuite, la sélection à l’intérieur de la file peut utiliser une politique propre, souvent Round Robin.

---

### 23.2 Exemple de classes

```text
Temps réel
Système
Interactif
Batch
```

---

### 23.3 Avantages

- Séparation claire des types de processus.
- Possibilité de donner plus de poids aux tâches importantes.
- Chaque classe peut avoir sa propre politique.

---

### 23.4 Inconvénient

La classe d’un processus doit souvent être assignée à l’avance, ce qui n’est pas toujours efficace.

---

## 24. Multilevel Feedback Queues

### 24.1 Principe

Les processus peuvent changer dynamiquement de file selon leur comportement.

Observation de base :

- un processus CPU-bound utilise souvent tout son quantum ;
- un processus I/O-bound rend souvent le CPU avant la fin du quantum.

---

### 24.2 Règles simples

Tous les processus commencent dans la file de plus haute priorité.

Si un processus utilise tout son quantum, on considère qu’il est probablement CPU-bound, donc il descend vers une file de priorité plus faible.

S’il n’utilise pas tout son quantum, on considère qu’il est probablement I/O-bound, donc il reste dans la même file.

---

### 24.3 Avantage

Cette méthode s’adapte au comportement réel du processus.

---

### 24.4 Problème : tricher avec l’ordonnanceur

Un processus intensif en calcul peut volontairement dormir juste avant la fin de son quantum pour rester dans une file de haute priorité.

Pseudo-code :

```c
while (1) {
    faire_du_calcul_presque_tout_le_quantum();
    sleep(jusqu_a_la_fin_du_quantum);
}
```

C’est ce que le cours appelle **gaming the system**.

---

## 25. Ordonnancement multiprocesseur

### 25.1 Contexte

Dans un système multiprocesseur, plusieurs CPU peuvent exécuter des processus en parallèle.

Le problème devient plus complexe :

- quel processus exécuter ?
- sur quel CPU ?
- faut-il migrer un processus ?
- comment équilibrer la charge ?
- comment préserver la localité cache ?

---

### 25.2 Migration de processus

Un processus peut s’exécuter sur un CPU à un quantum, puis sur un autre CPU au quantum suivant.

C’est la **migration de processus**.

---

### 25.3 Coût de la migration

La migration est coûteuse car :

- les caches doivent être rechargés ;
- la mémoire locale peut être moins proche ;
- les données du processus doivent être retrouvées sur le nouveau CPU.

---

### 25.4 Affinité processeur

L’affinité processeur indique sur quels CPU un processus peut s’exécuter.

Deux types :

| Type | Description |
|---|---|
| Hard affinity | contrainte stricte |
| Soft affinity | préférence, mais migration possible |

---

## 26. Ordonnancement multiprocesseur avec un ordonnanceur unique

Une approche naïve consiste à avoir un seul ordonnanceur global qui décide pour tous les CPU.

Avantage :

- décision centralisée simple à comprendre.

Inconvénients :

- goulot d’étranglement ;
- mauvaise scalabilité ;
- contention ;
- peu adapté aux grands systèmes.

---

## 27. Symmetrical Scheduling

Dans l’ordonnancement symétrique, chaque processeur exécute son propre ordonnanceur.

Deux variantes :

- files globales ;
- files par CPU.

---

## 28. Files globales

### 28.1 Principe

Tous les CPU partagent une ou plusieurs files globales de processus prêts.

---

### 28.2 Avantages

- bonne utilisation des CPU ;
- équité globale entre processus.

---

### 28.3 Inconvénients

- non scalable ;
- contention sur la file globale ;
- verrouillage nécessaire ;
- affinité processeur difficile à préserver.

Cette approche était utilisée dans Linux 2.4 et dans xv6.

---

## 29. Files par CPU

### 29.1 Principe

Chaque CPU possède sa propre file de processus prêts.

---

### 29.2 Avantages

- scalable ;
- peu de contention ;
- meilleure localité ;
- plus simple à implémenter.

---

### 29.3 Inconvénient

Il peut y avoir un déséquilibre de charge.

Exemple : un CPU a beaucoup de processus, un autre n’en a presque aucun.

---

## 30. Approche hybride

L’approche hybride combine :

- files locales ;
- mécanismes globaux d’équilibrage.

Elle permet :

- de préserver la localité ;
- d’équilibrer la charge ;
- de réduire la contention.

Linux 2.6 suit une approche de ce type.

---

## 31. Load Balancing

### 31.1 Définition

Le load balancing essaie de répartir le travail équitablement entre tous les processeurs.

---

### 31.2 Push migration

Une tâche spéciale observe périodiquement les charges des CPU.

Si elle détecte un déséquilibre, elle déplace des tâches d’un CPU surchargé vers un CPU moins chargé.

---

### 31.3 Pull migration

Un CPU inactif va chercher une tâche dans la file d’un CPU occupé.

---

## 32. Types de processus sous Linux

Linux distingue plusieurs types de processus.

### 32.1 Processus temps réel

Ils ont des contraintes fortes de délai.

Ils ne doivent pas être bloqués par des tâches de faible priorité.

---

### 32.2 Processus normaux

Deux grandes catégories :

#### Processus interactifs

Ils interagissent avec l’utilisateur.

Exemples :

- terminal ;
- interface graphique ;
- éditeur de texte.

Ils attendent souvent des entrées, mais doivent réagir vite lorsqu’une entrée arrive.

Le délai attendu est typiquement de l’ordre de 50 à 150 ms.

#### Processus batch

Ils n’interagissent pas avec l’utilisateur.

Ils s’exécutent souvent en arrière-plan.

---

## 33. Historique des ordonnanceurs Linux

Pour les processus normaux, Linux a connu plusieurs ordonnanceurs :

| Ordonnanceur | Période |
|---|---|
| `O(n)` scheduler | Linux 2.4 à 2.6 |
| `O(1)` scheduler | Linux 2.6 à 2.6.22 |
| CFS | Linux 2.6.23 et versions suivantes |

---

## 34. Ordonnanceur `O(n)`

### 34.1 Principe

À chaque changement de contexte :

1. parcourir la liste des processus prêts ;
2. calculer les priorités ;
3. choisir le meilleur processus.

---

### 34.2 Complexité

Si `n` est le nombre de processus prêts, le coût est `O(n)`.

---

### 34.3 Inconvénients

- non scalable ;
- coûteux si beaucoup de processus ;
- problème accentué avec des environnements comme Java qui créent beaucoup de tâches ;
- utilisation d’une file globale en SMP, donc faible scalabilité.

---

## 35. Ordonnanceur `O(1)`

### 35.1 Objectif

Choisir le prochain processus en temps constant.

---

### 35.2 Types de processus

L’ordonnanceur `O(1)` distingue :

- processus temps réel ;
- processus normaux.

---

### 35.3 Priorités

| Type | Plage de priorités |
|---|---|
| Temps réel | 0 à 99 |
| Normal | 100 à 139 |

Pour les processus normaux :

- 100 = priorité la plus haute ;
- 139 = priorité la plus basse.

---

### 35.4 Files active et expired

Chaque CPU possède deux ensembles de files :

- active run queues ;
- expired run queues.

Chaque ensemble contient 40 classes de priorité de 100 à 139.

---

### 35.5 Politique

1. choisir la première tâche de la file de plus haute priorité ;
2. l’exécuter ;
3. lorsqu’elle a utilisé son quantum, la placer dans la file expired appropriée ;
4. lorsque les files active sont vides, échanger active et expired.

---

### 35.6 Pourquoi `O(1)` ?

Deux étapes :

1. trouver la première file non vide ;
2. choisir la première tâche de cette file.

La deuxième étape est constante.

Pour la première étape, Linux utilise un bitmap des files non vides et une instruction matérielle comme `find-first-bit-set`.

Donc le choix est fait en temps constant.

---

## 36. Priorités dans l’ordonnanceur `O(1)`

### 36.1 Priorité statique

La priorité de base par défaut est :

```text
120
```

Elle peut être modifiée avec `nice`.

Commande :

```bash
nice -n N ./a.out
```

Avec :

- `N = -20` : priorité la plus élevée ;
- `N = +19` : priorité la plus basse.

---

### 36.2 Priorité dynamique

La priorité dynamique sert à distinguer processus interactifs et batch.

Formule donnée dans le cours :

```text
dynamic_priority = MAX(100, MIN(static_priority - bonus + 5, 139))
```

Le bonus dépend du comportement du processus.

---

### 36.3 Bonus

Le bonus est basé sur le temps moyen de sommeil.

- Un processus I/O-bound dort souvent, donc il reçoit un bonus plus élevé.
- Un processus CPU-bound dort peu, donc il reçoit un bonus faible.

Effet :

- bonus > 5 : priorité améliorée vers 100 ;
- bonus < 5 : priorité dégradée vers 139.

---

## 37. Quantum dans l’ordonnanceur `O(1)`

Le quantum dépend de la priorité.

Formule :

```text
Si priority < 120 :
    time_slice = (140 - priority) * 20 ms
Sinon :
    time_slice = (140 - priority) * 5 ms
```

Exemples :

| Priorité | Nice | Quantum |
|---:|---:|---:|
| 100 | -20 | 800 ms |
| 110 | -10 | 600 ms |
| 120 | 0 | 100 ms |
| 130 | 10 | 50 ms |
| 139 | 19 | 5 ms |

---

## 38. Résumé de l’ordonnanceur `O(1)`

L’ordonnanceur `O(1)` repose sur :

- des files multi-niveaux ;
- 40 classes de priorité ;
- une priorité de base 120 ;
- une priorité dynamique basée sur le temps de sommeil ;
- un quantum dépendant de la priorité ;
- deux ensembles de files : active et expired.

---

## 39. Limites de l’ordonnanceur `O(1)`

Ses limites principales sont :

- heuristiques complexes ;
- difficulté à distinguer proprement interactif et non interactif ;
- dépendance forte entre priorité et quantum ;
- valeurs de quantum non uniformes ;
- comportement parfois difficile à prédire.

---

## 40. CFS : Completely Fair Scheduler

### 40.1 Présentation

CFS est l’ordonnanceur Linux depuis la version 2.6.23.

Il a été introduit par Ingo Molnar et intégré dans le noyau Linux en 2007.

CFS signifie :

```text
Completely Fair Scheduler
```

Son objectif est de donner une part équitable du CPU aux tâches.

---

### 40.2 Idée de fairness idéale

Si `N` processus sont prêts, chacun devrait recevoir environ :

```text
100 / N %
```

du temps processeur.

Exemple :

- s’il y a 4 processus prêts, chacun reçoit environ 25 % du CPU ;
- s’il y a 2 processus prêts, chacun reçoit environ 50 % ;
- s’il n’y en a qu’un, il reçoit 100 %.

---

## 41. Virtual runtime

### 41.1 Définition

Chaque processus exécutable possède un compteur appelé :

```text
vruntime
```

Il représente le temps d’exécution virtuel du processus.

---

### 41.2 Mise à jour

Si un processus a exécuté pendant `t` ms :

```text
vruntime += t
```

En réalité, cette valeur peut être pondérée par la priorité.

Le `vruntime` augmente toujours.

---

## 42. Idée centrale de CFS

À chaque décision d’ordonnancement :

1. choisir la tâche ayant le plus petit `vruntime` ;
2. lui donner le CPU ;
3. mettre à jour son `vruntime` ;
4. la replacer dans la structure des tâches prêtes ;
5. répéter.

Le processus qui a le moins exécuté virtuellement est donc choisi en premier.

---

## 43. Structure de données de CFS : arbre rouge-noir

### 43.1 Rôle

CFS stocke les tâches prêtes dans un **arbre rouge-noir**.

Chaque nœud représente une tâche.

Les tâches sont ordonnées selon leur `vruntime`.

---

### 43.2 Propriété

- À gauche : tâches avec plus petit `vruntime`.
- À droite : tâches avec plus grand `vruntime`.

Le nœud le plus à gauche correspond donc à la tâche qui a le moins exécuté virtuellement.

---

### 43.3 `min_vruntime`

CFS garde en cache le minimum, appelé :

```text
min_vruntime
```

Cela permet de retrouver rapidement la prochaine tâche.

---

## 44. Changement de contexte avec CFS

Lors d’un changement de contexte :

1. CFS prend le nœud le plus à gauche ;
2. ce processus s’exécute ;
3. son `vruntime` augmente ;
4. s’il est encore prêt, il est réinséré dans l’arbre selon son nouveau `vruntime`.

Après exécution, une tâche se déplace donc progressivement vers la droite de l’arbre.

Cela évite la famine.

---

## 45. Pourquoi un arbre rouge-noir ?

Un arbre rouge-noir est auto-équilibré.

Propriétés :

- aucune branche n’est beaucoup plus longue qu’une autre ;
- insertion en `O(log n)` ;
- suppression en `O(log n)` ;
- recherche efficace ;
- bon compromis entre performance et équité.

---

## 46. Priorités et CFS

Avec CFS, la priorité liée à `nice` influence le poids du `vruntime`.

Idée :

```text
vruntime += t * poids_selon_nice
```

Une tâche de faible priorité voit son `vruntime` augmenter plus vite.

Donc elle se retrouve plus rapidement vers la droite de l’arbre et est choisie moins souvent.

Une tâche de haute priorité voit son `vruntime` augmenter plus lentement.

Donc elle reste plus facilement vers la gauche et est choisie plus souvent.

---

## 47. Processus I/O-bound et CPU-bound avec CFS

### 47.1 Processus I/O-bound

Un processus I/O-bound exécute de petites rafales CPU puis dort en attente d’entrée/sortie.

Comme il utilise peu le CPU, son `vruntime` reste faible.

Il reste donc près de la gauche de l’arbre et sera choisi rapidement lorsqu’il redevient prêt.

---

### 47.2 Processus CPU-bound

Un processus CPU-bound consomme beaucoup de CPU.

Son `vruntime` augmente plus vite.

Il se déplace vers la droite de l’arbre, ce qui laisse la place aux autres tâches.

---

### 47.3 Avantage de CFS

CFS favorise naturellement les tâches interactives ou I/O-bound sans avoir besoin d’heuristiques complexes comme l’ordonnanceur `O(1)`.

---

## 48. Nouveau processus dans CFS

Un nouveau processus est ajouté à l’arbre rouge-noir.

Il démarre avec une valeur initiale proche de :

```text
min_vruntime
```

Cela lui permet d’être exécuté rapidement sans perturber fortement l’équité globale.

---

## 49. Comparaison des grands algorithmes

| Algorithme | Idée | Avantage | Inconvénient |
|---|---|---|---|
| Round Robin | tour de rôle avec quantum | simple et équitable | ignore l’importance |
| Priorité | choisir la priorité la plus haute | favorise les tâches importantes | famine possible |
| Multilevel Queues | plusieurs files par classe | organise les tâches | classe fixée à l’avance |
| Multilevel Feedback | changement dynamique de file | s’adapte au comportement | peut être trompé |
| O(n) Linux | parcours complet | simple | non scalable |
| O(1) Linux | files actives/expired | choix constant | heuristiques complexes |
| CFS | plus petit `vruntime` | équité élégante | structure plus sophistiquée |

---

## 50. Points essentiels à retenir

1. Un processus est l’exécution d’un programme.
2. Un processus possède son propre espace d’adressage.
3. `fork` crée un processus fils.
4. Sous Linux, `fork` utilise le copy-on-write.
5. `execve` remplace le programme courant sans changer le PID.
6. `wait` permet au père de récupérer la terminaison d’un fils.
7. Un processus zombi est terminé mais pas encore attendu par son père.
8. Les signaux sont des interruptions logicielles.
9. Les sémaphores servent à synchroniser processus ou threads.
10. Un thread partage l’espace mémoire de son processus.
11. Round Robin donne un quantum à chaque tâche.
12. L’ordonnancement par priorité peut causer la famine.
13. Les files multi-niveaux organisent les tâches par classes.
14. Les files multi-niveaux avec feedback adaptent la priorité selon le comportement.
15. En multiprocesseur, la migration est coûteuse.
16. Les files globales sont équitables mais peu scalables.
17. Les files par CPU sont scalables mais peuvent déséquilibrer la charge.
18. Linux a utilisé `O(n)`, puis `O(1)`, puis CFS.
19. CFS choisit la tâche au plus petit `vruntime`.
20. CFS utilise un arbre rouge-noir.

---

## 51. Questions de révision rapides

### Question 1

Quelle est la différence entre un programme et un processus ?

**Réponse :**  
Un programme est un fichier passif. Un processus est une instance en cours d’exécution de ce programme.

---

### Question 2

Que renvoie `fork` dans le processus fils ?

**Réponse :**  
`fork` renvoie `0` dans le processus fils.

---

### Question 3

Que renvoie `fork` dans le processus père ?

**Réponse :**  
Il renvoie le PID du processus fils.

---

### Question 4

Pourquoi `fork` est-il efficace sous Linux ?

**Réponse :**  
Parce qu’il utilise le copy-on-write : les pages mémoire sont copiées seulement lorsqu’elles sont modifiées.

---

### Question 5

Quelle est la différence entre `fork` et `execve` ?

**Réponse :**  
`fork` crée un nouveau processus. `execve` remplace le programme courant dans le même processus.

---

### Question 6

Qu’est-ce qu’un processus zombi ?

**Réponse :**  
Un processus terminé dont le père n’a pas encore récupéré le statut avec `wait` ou `waitpid`.

---

### Question 7

À quoi sert `waitpid` ?

**Réponse :**  
À attendre un processus fils précis ou un groupe de processus fils selon la valeur de `pid`.

---

### Question 8

Qu’est-ce qu’un signal ?

**Réponse :**  
Une interruption logicielle envoyée à un processus pour lui notifier un événement.

---

### Question 9

Quel signal termine immédiatement un processus ?

**Réponse :**  
`SIGKILL`, numéro 9.

---

### Question 10

Quelle est la différence entre processus et thread ?

**Réponse :**  
Deux processus ont des espaces mémoire séparés. Deux threads d’un même processus partagent le même espace mémoire.

---

### Question 11

Quel est le problème de l’ordonnancement par priorité ?

**Réponse :**  
Il peut provoquer la famine des processus de faible priorité.

---

### Question 12

Comment éviter la famine ?

**Réponse :**  
En augmentant progressivement la priorité des processus qui attendent longtemps.

---

### Question 13

Pourquoi la migration de processus est coûteuse ?

**Réponse :**  
Parce qu’elle peut casser la localité cache et obliger à recharger les données sur un autre CPU.

---

### Question 14

Pourquoi l’ordonnanceur `O(n)` n’est-il pas scalable ?

**Réponse :**  
Parce qu’il parcourt tous les processus prêts à chaque décision.

---

### Question 15

Pourquoi l’ordonnanceur `O(1)` est-il en temps constant ?

**Réponse :**  
Parce qu’il utilise des files de priorités et un bitmap permettant de trouver rapidement une file non vide.

---

### Question 16

Quelle est l’idée principale de CFS ?

**Réponse :**  
Choisir la tâche ayant le plus petit temps d’exécution virtuel (`vruntime`) pour partager équitablement le CPU.

---

### Question 17

Pourquoi CFS utilise-t-il un arbre rouge-noir ?

**Réponse :**  
Parce qu’il permet d’insérer, supprimer et ordonner les tâches efficacement en `O(log n)`.

---

### Question 18

Comment CFS favorise-t-il les processus I/O-bound ?

**Réponse :**  
Ils consomment peu de CPU, donc leur `vruntime` reste faible, ce qui les place près de la gauche de l’arbre.

---

## 52. Mini-fiche ultra courte

```text
Processus = programme en exécution.
fork = création d’un fils.
fork parent → PID fils.
fork fils → 0.
execve = remplace le programme courant.
wait/waitpid = attendre un fils.
zombi = fils terminé non récupéré.
signal = interruption logicielle.
sémaphore = synchronisation.
thread = processus léger partageant la mémoire.
Round Robin = tour de rôle.
Priorité = risque de famine.
MLFQ = files avec priorité dynamique.
SMP = plusieurs CPU.
Migration = coûteuse.
Linux O(n) = parcours liste.
Linux O(1) = files active/expired.
CFS = vruntime + arbre rouge-noir.
```

---

## 53. Formules importantes

### Priorité dynamique de l’ordonnanceur `O(1)`

```text
dynamic_priority = MAX(100, MIN(static_priority - bonus + 5, 139))
```

### Quantum de l’ordonnanceur `O(1)`

```text
Si priority < 120 :
    time_slice = (140 - priority) * 20 ms
Sinon :
    time_slice = (140 - priority) * 5 ms
```

### Fairness idéale

```text
Part CPU par processus = 100 / N %
```

### Mise à jour simplifiée du `vruntime`

```text
vruntime += temps_exécuté
```

Avec pondération par priorité :

```text
vruntime += temps_exécuté * poids_selon_nice
```

---

## 54. Conseils pour l’examen

Pour répondre clairement à une question sur ce cours :

1. Commencer par une définition simple.
2. Donner le rôle du mécanisme.
3. Expliquer le fonctionnement étape par étape.
4. Donner un exemple court.
5. Mentionner les avantages et les limites.

Exemple pour `fork` :

```text
fork est un appel système qui crée un processus fils en dupliquant le processus parent. 
Dans le père, fork renvoie le PID du fils. Dans le fils, fork renvoie 0. 
Sous Linux, la duplication mémoire est optimisée par copy-on-write : les pages ne sont copiées qu’en cas d’écriture.
```

Exemple pour CFS :

```text
CFS est l’ordonnanceur Linux depuis la version 2.6.23. 
Il cherche à partager équitablement le CPU. 
Chaque tâche possède un vruntime. 
L’ordonnanceur choisit la tâche dont le vruntime est le plus faible. 
Les tâches sont stockées dans un arbre rouge-noir.
```