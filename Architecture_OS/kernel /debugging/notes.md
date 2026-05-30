# Notes de cours — Debugging Système et Noyau

> **Module** : Architecture OS — ARSE · ENSIIE  
> **Auteur du support** : Aurélien Cedeyn  
> **Année** : 2021–2022  
> **Version EN** : [notes.en.md](notes.en.md)  
> **Objectif** : comprendre le debugging en espace utilisateur et en espace noyau.

---

> **Outils à choisir selon le problème**
>
> | Problème | Outil |
> |---|---|
> | Voir les processus | `ps aux`, `top` |
> | Tracer les syscalls | `strace` |
> | Tracer les appels libc | `ltrace` |
> | Programme qui plante | `gdb`, core file |
> | Logs noyau | `dmesg -T` |
> | Tracer fonctions noyau | `perf`, `ftrace`, BPF |
> | Kernel panic | `kdump`, `crash` |

---

## Table des matières

1. [Idée générale du debugging](#1-idée-générale-du-debugging)
2. [Les trois univers d'un système](#2-les-trois-univers-dun-système)
3. [Notions d'assembleur utiles au debugging](#3-notions-dassembleur-utiles-au-debugging)
4. [De la source au binaire](#4-de-la-source-au-binaire)
5. [Exécution d'un programme en mémoire](#5-exécution-dun-programme-en-mémoire)
6. [La pile d'exécution](#6-la-pile-dexécution)
7. [Debugging des processus utilisateur](#7-debugging-des-processus-utilisateur)
8. [Lire et comprendre `/proc`](#8-lire-et-comprendre-proc)
9. [`strace` : tracer les appels systèmes](#9-strace--tracer-les-appels-systèmes)
10. [`ltrace` : tracer les appels de bibliothèques](#10-ltrace--tracer-les-appels-de-bibliothèques)
11. [Sources, symboles et `cscope`](#11-sources-symboles-et-cscope)
12. [`gdb` : debugger un processus utilisateur](#12-gdb--debugger-un-processus-utilisateur)
13. [Core files et analyse post-mortem](#13-core-files-et-analyse-post-mortem)
14. [Au-delà de l'espace utilisateur : le noyau](#14-au-delà-de-lespace-utilisateur--le-noyau)
15. [`task_struct` et `mm_struct`](#15-task_struct-et-mm_struct)
16. [Les appels systèmes dans le noyau](#16-les-appels-systèmes-dans-le-noyau)
17. [Cycle réel d'exécution : `clone` -> `execve` -> `exit`](#17-cycle-réel-dexécution--clone---execve---exit)
18. [Logs noyau avec `dmesg`](#18-logs-noyau-avec-dmesg)
19. [`debugfs`, `dynamic_debug` et tracing noyau](#19-debugfs-dynamic_debug-et-tracing-noyau)
20. [`perf` : tracing et statistiques](#20-perf--tracing-et-statistiques)
21. [`crash` : debugger le noyau](#21-crash--debugger-le-noyau)
22. [Kdump et Kexec](#22-kdump-et-kexec)
23. [Autres outils : BPF/BCC et SystemTap](#23-autres-outils--bpfbcc-et-systemtap)
24. [Méthodologie pratique de debugging](#24-méthodologie-pratique-de-debugging)
25. [Commandes essentielles à retenir](#25-commandes-essentielles-à-retenir)
26. [Questions d'examen possibles](#26-questions-dexamen-possibles)
27. [Résumé ultra-court](#27-résumé-ultra-court)

---

## 1. Idée générale du debugging

Le **debugging** consiste à rechercher, comprendre et corriger des erreurs ou des comportements anormaux dans un système informatique.

On debugge lorsqu'un élément ne se comporte pas comme prévu :

- une application produit un résultat incorrect ;
- les performances sont mauvaises ;
- un problème de sécurité apparaît ;
- un processus plante ;
- le système entier devient instable ;
- le noyau génère une erreur ou un kernel panic.

Le debugging ne consiste pas seulement à lancer un outil. Il faut :

1. observer le symptôme ;
2. formuler des hypothèses ;
3. choisir le bon outil ;
4. collecter des traces ;
5. relier les traces au fonctionnement interne du système ;
6. confirmer ou infirmer l'hypothèse.

### Les grands types d'analyse

Le cours distingue plusieurs manières d'analyser un problème :

- lire et comprendre les logs ;
- tracer un processus ;
- récupérer la pile d'exécution ;
- analyser la mémoire d'un processus ;
- inspecter le comportement du noyau ;
- faire une analyse post-mortem après un crash.

---

## 2. Les trois univers d'un système

Un système d'exploitation peut être vu comme trois couches :

```text
+-----------------------------+
| User Space                  |
| - terminal                  |
| - window manager            |
| - systemd / init            |
| - programmes utilisateur    |
| - libc                      |
+-----------------------------+
| Kernel Space                |
| - appels systèmes           |
| - services noyau            |
| - modules                   |
| - drivers                   |
+-----------------------------+
| Hardware                    |
| - CPU                       |
| - mémoire                   |
| - périphériques             |
+-----------------------------+
```

### Espace utilisateur

L'espace utilisateur contient les programmes lancés par l'utilisateur : shell, éditeurs, navigateurs, programmes C, services, etc.

Un programme utilisateur **n'accède pas directement au matériel**. Il passe par le noyau.

### Espace noyau

L'espace noyau contient le noyau Linux, les services internes, les modules et les drivers.

Le noyau :

- gère les processus ;
- gère la mémoire ;
- gère les fichiers ;
- gère le réseau ;
- communique avec le matériel ;
- expose des fonctionnalités via les appels systèmes.

### Matériel

Le matériel correspond au CPU, à la mémoire, aux disques, aux cartes réseau et aux périphériques.

### Idée clé

L'espace utilisateur accède au noyau via des **syscalls**. Le noyau accède au matériel via ses **drivers**.

---

## 3. Notions d'assembleur utiles au debugging

Pour comprendre `gdb`, une pile d'exécution ou un appel système, il faut connaître quelques registres et instructions assembleur.

### Registres x86_64 importants

| Registre | Rôle simplifié |
|---|---|
| `%rax` | accumulateur ; utilisé pour les calculs et pour stocker le numéro de syscall |
| `%rbx` | registre auxiliaire ; utile pour calculs et adresses |
| `%rcx` | compteur ; souvent utilisé dans les boucles |
| `%rdx` | registre de données ; utilisé pour passer ou stocker des données |
| `%rdi` | premier argument d'une fonction ou d'un syscall en x86_64 Linux |
| `%rsi` | deuxième argument d'une fonction ou d'un syscall |
| `%rbp` | base pointer ; base du cadre de pile courant |
| `%rsp` | stack pointer ; sommet courant de la pile |
| `%rip` | instruction pointer ; adresse de la prochaine instruction |

### Sous-registres

Un registre 64 bits peut être vu en plusieurs tailles :

```text
rax : 64 bits
 eax : 32 bits
  ax : 16 bits
  ah/al : 8 bits
```

### Instruction `mov`

```asm
mov src, dest
```

Elle copie la source vers la destination.

Exemple :

```asm
mov $60, %rax
mov $2, %rdi
syscall
```

Ici :

- `60` est placé dans `%rax` : numéro du syscall `exit` en x86_64 ;
- `2` est placé dans `%rdi` : code de retour transmis à `exit` ;
- `syscall` déclenche l'appel système.

### Instruction `syscall`

`syscall` déclenche une interruption logicielle contrôlée permettant de passer de l'espace utilisateur vers l'espace noyau.

---

## 4. De la source au binaire

Un programme passe par plusieurs étapes avant d'être exécuté.

### Exemple en C

```c
void main(void) {
    exit(2);
}
```

### Exemple équivalent en assembleur x86_64

```asm
.text
.globl _start
_start:
    mov $60, %rax
    mov $2, %rdi
    syscall
```

### Compilation en objet

```bash
as add.s -o add.o
```

Le compilateur ou l'assembleur produit un **fichier objet** contenant du code machine mais pas encore un exécutable complet.

### Édition de liens

```bash
ld add.o -o add
```

Le linker transforme l'objet en fichier exécutable.

### Lire un binaire

Deux outils importants :

```bash
objdump -dsh add.o
xxd add.o
```

- `objdump -dsh` affiche les sections, le contenu binaire et le désassemblage.
- `xxd` affiche les octets bruts du fichier.

### Notion importante : une instruction est un ensemble d'octets

Une instruction assembleur n'est pas magique. Pour le processeur, elle correspond à des octets.

Exemple x86_64 :

```asm
48 c7 c0 3c 00 00 00    mov $0x3c,%rax
48 c7 c7 02 00 00 00    mov $0x2,%rdi
0f 05                   syscall
```

---

## 5. Exécution d'un programme en mémoire

Lorsqu'un binaire est lancé, il est chargé en mémoire sous forme de segments.

```text
+-----------------------------+
| stack                       |
| pile d'appels               |
+-----------------------------+
|                             |
| espace libre                |
|                             |
+-----------------------------+
| heap                        |
| allocations dynamiques      |
+-----------------------------+
| bss                         |
| variables globales non init |
+-----------------------------+
| data                        |
| variables globales init     |
+-----------------------------+
| text                        |
| code exécutable             |
+-----------------------------+
```

### Segment `text`

Contient le code exécutable du programme.

### Segment `data`

Contient les variables globales initialisées.

Exemple :

```c
int x = 5;
```

### Segment `bss`

Contient les variables globales non initialisées.

Exemple :

```c
int y;
```

### Heap

Le tas contient les allocations dynamiques.

Exemple :

```c
int *p = malloc(sizeof(int));
```

### Stack

La pile contient :

- les appels de fonctions ;
- les variables locales ;
- les anciennes valeurs de registres ;
- les adresses de retour.

---

## 6. La pile d'exécution

La pile est essentielle pour comprendre un crash ou une backtrace.

### Registres importants

| Registre | Signification |
|---|---|
| `%rbp` | base du cadre de pile courant |
| `%rsp` | sommet courant de la pile |
| `%rip` | adresse de la prochaine instruction |

### Instructions importantes

#### `push`

Pousse une valeur sur la pile.

```asm
push %rbp
```

#### `pop`

Récupère la dernière valeur de la pile.

```asm
pop %rbp
```

#### `call`

Appelle une fonction. L'instruction fait deux choses :

1. sauvegarde l'adresse de retour sur la pile ;
2. place dans `%rip` l'adresse de la fonction appelée.

#### `ret`

Retourne de la fonction appelée. C'est conceptuellement :

```asm
pop %rip
```

### Exemple conceptuel

Code C :

```c
void main(void) {
    test();
}

void test(void) {
    return in_test();
}

void in_test(void) {
    return;
}
```

Ordre logique :

```text
main -> test -> in_test -> retour in_test -> retour test -> retour main
```

À chaque appel :

- l'adresse de retour est poussée sur la pile ;
- `%rbp` est sauvegardé ;
- `%rsp` évolue ;
- un nouveau cadre de pile est créé.

À chaque retour :

- `%rbp` est restauré ;
- `ret` récupère l'adresse de retour ;
- l'exécution reprend dans la fonction appelante.

### Pourquoi c'est important ?

Quand un programme plante, `gdb` peut afficher la pile d'appels. Cette pile permet de savoir **quelle fonction a appelé quelle fonction** jusqu'au crash.

---

## 7. Debugging des processus utilisateur

Le cours présente quatre familles d'outils pour l'espace utilisateur :

- `ps` et `top` pour observer l'état du système ;
- `strace` pour voir les appels systèmes ;
- `ltrace` pour voir les appels de bibliothèques ;
- `gdb` pour inspecter finement l'exécution.

---

## 8. Lire et comprendre `/proc`

`/proc` est un pseudo-système de fichiers qui expose des informations sur le noyau et les processus.

Exemple :

```bash
cat /proc/self/stat
ls /proc/self/
```

On y trouve notamment :

```text
cmdline
cwd
environ
fd
fdinfo
io
limits
maps
mem
mountinfo
net
sched
smaps
stack
stat
statm
status
syscall
task
wchan
```

### `ps`

```bash
ps aux f
```

Champs importants :

| Champ | Signification |
|---|---|
| PID | identifiant du processus |
| %CPU | consommation CPU |
| %MEM | consommation mémoire |
| VSZ | taille virtuelle utilisable |
| RSS | mémoire réellement utilisée |
| TTY | terminal attaché |
| STAT | état du processus |
| START | date de lancement |
| TIME | temps CPU consommé |
| COMMAND | commande exécutée |

### `top`

```bash
top
```

`top` donne une vision continue de :

- l'uptime ;
- la charge moyenne ;
- le nombre de tâches ;
- l'utilisation CPU ;
- l'utilisation mémoire ;
- les processus les plus actifs.

### Idée clé

`ps` et `top` lisent principalement des informations issues de `/proc`.

---

## 9. `strace` : tracer les appels systèmes

`strace` permet d'observer les appels systèmes effectués par un programme.

Un appel système est une fonction exposée par le noyau à l'espace utilisateur.

### Exemple

```bash
strace -e open,getdents ls
```

Exemple de sortie :

```text
open("/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
open(".", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC) = 3
getdents(3, /* 2 entries */, 32768) = 48
getdents(3, /* 0 entries */, 32768) = 0
+++ exited with 0 +++
```

### Interprétation

Ici, `ls` :

1. ouvre des bibliothèques ;
2. ouvre le répertoire courant ;
3. lit les entrées du répertoire avec `getdents` ;
4. se termine correctement.

### Options utiles

| Option | Rôle |
|---|---|
| `-e` | filtrer les syscalls observés |
| `-tt` | afficher les timestamps |
| `-c` | produire un résumé statistique |
| `-p PID` | s'attacher à un processus existant |
| `-f` | suivre les processus fils |

### Exemples

Filtrer :

```bash
strace -e open,getdents ls
```

Voir les temps :

```bash
strace -tt ls
```

Résumé statistique :

```bash
strace -c ls
```

S'attacher à un processus :

```bash
strace -p <PID>
```

Suivre un shell qui crée un fils :

```bash
strace -f -p <PID>
```

### À retenir

`strace` sert à comprendre comment un processus utilisateur interagit avec le noyau.

---

## 10. `ltrace` : tracer les appels de bibliothèques

`ltrace` observe les appels aux fonctions de bibliothèques dynamiques.

Contrairement à `strace`, il ne montre pas directement les syscalls.

### Exemple

```bash
ltrace -e opendir -e readdir ls
```

Sortie possible :

```text
ls->opendir(".") = 0xf6bca0
ls->readdir(0xf6bca0) = 0xf6bcd0
ls->readdir(0xf6bca0) = 0xf6bce8
ls->readdir(0xf6bca0) = 0
+++ exited (status 0) +++
```

### Options utiles

| Option | Rôle |
|---|---|
| `-e` | sélectionner les fonctions à observer |
| `-tt` | afficher les temps |
| `-c` | compter les appels |
| `-p PID` | s'attacher à un processus existant |

### Différence entre `strace` et `ltrace`

| Outil | Observe |
|---|---|
| `strace` | appels systèmes vers le noyau |
| `ltrace` | appels aux bibliothèques dynamiques |

Exemple :

- `opendir()` est une fonction de bibliothèque ;
- `open()` est un syscall sous-jacent possible.

---

## 11. Sources, symboles et `cscope`

Pour debugger efficacement, il faut idéalement disposer de :

- la version exacte du logiciel ;
- les sources correspondantes ;
- les symboles de debug.

### Pourquoi les symboles sont importants ?

Sans symboles, on voit surtout des adresses :

```text
0x00007ffff780b830 in ?? ()
```

Avec les symboles, on peut voir :

```text
main()
execute_command()
wait_for()
```

### `cscope`

`cscope` permet de naviguer dans un grand projet C, comme le noyau Linux.

Commandes :

```bash
cscope -d -R
cscope -b
```

Pour le noyau Linux :

```bash
make cscope
```

`cscope` permet de chercher :

- une définition globale ;
- une fonction ;
- les fonctions appelées par une fonction ;
- les fonctions qui appellent une fonction ;
- un symbole ;
- un fichier ;
- les fichiers incluant un header.

---

## 12. `gdb` : debugger un processus utilisateur

`gdb` est l'outil principal pour debugger un programme en espace utilisateur.

Il permet :

- d'exécuter un programme pas à pas ;
- de poser des breakpoints ;
- de lire les variables ;
- de lire les registres ;
- d'afficher la pile d'appels ;
- de désassembler une fonction ;
- de modifier des variables ;
- d'analyser un core file.

### Lancer `gdb`

Mode classique :

```bash
gdb <executable>
```

Avec core file :

```bash
gdb <executable> <corefile>
```

S'attacher à un processus :

```bash
gdb -p <PID>
```

### Commandes principales

| Commande | Rôle |
|---|---|
| `break <ligne|fonction>` | placer un point d'arrêt |
| `run <args>` | lancer le programme |
| `where` ou `bt` | afficher la pile d'appels |
| `continue` | continuer l'exécution |
| `set var=value` | modifier une variable |
| `thread apply all where` | afficher la pile de tous les threads |
| `up` / `down` | se déplacer dans la pile |
| `info registers` | afficher les registres |
| `info source` | afficher les infos source |
| `disassemble <fonction>` | afficher l'assembleur d'une fonction |

### Exemple minimal

```gdb
(gdb) break opendir
(gdb) run /
(gdb) where
```

### Lire les registres

```gdb
(gdb) info registers
```

Registres à surveiller :

- `rip` : instruction courante ;
- `rsp` : sommet de pile ;
- `rbp` : base du cadre de pile ;
- `rax` : souvent valeur de retour ;
- `rdi`, `rsi`, `rdx` : arguments de fonction/syscall.

### Désassembler

```gdb
(gdb) disassemble func
```

Permet de relier le comportement haut niveau au code machine réellement exécuté.

---

## 13. Core files et analyse post-mortem

Un **core file** contient une image de la mémoire d'un processus à un instant donné, souvent après un crash.

### Génération automatique

Activer les core files :

```bash
ulimit -c unlimited
```

Voir le pattern de génération :

```bash
cat /proc/sys/kernel/core_pattern
```

### Génération à la demande

```bash
gcore <PID>
```

### Analyse avec `gdb`

```bash
gdb ./programme core
```

Puis :

```gdb
(gdb) where
(gdb) info registers
(gdb) frame 0
(gdb) print variable
```

### Intérêt

L'analyse post-mortem permet de comprendre un crash même lorsque le processus n'est plus en cours d'exécution.

---

## 14. Au-delà de l'espace utilisateur : le noyau

Pour comprendre ce qui se passe côté noyau, il faut changer de niveau.

On ne regarde plus seulement un programme, mais :

- les `task_struct` ;
- les syscalls ;
- les logs noyau ;
- le tracing noyau ;
- les structures internes ;
- les dumps mémoire.

---

## 15. `task_struct` et `mm_struct`

### `task_struct`

La `task_struct` est la structure noyau qui représente un processus ou une tâche.

Elle contient notamment :

- l'état du processus ;
- le pointeur vers sa pile noyau ;
- des compteurs d'usage ;
- le CPU courant ;
- un pointeur vers `mm_struct` ;
- les informations d'ordonnancement ;
- les liens vers les autres tâches.

### `mm_struct`

La `mm_struct` décrit l'espace mémoire du processus.

Elle contient notamment :

- la liste des zones mémoire (`vm_area_struct`) ;
- les bornes du code : `start_code`, `end_code` ;
- les bornes des données : `start_data`, `end_data` ;
- le tas : `start_brk`, `brk` ;
- la pile : `start_stack` ;
- la base des zones `mmap`.

### Lien avec la mémoire utilisateur

```text
task_struct
    |
    +--> mm_struct
             |
             +--> text
             +--> data
             +--> bss
             +--> heap
             +--> mmap
             +--> stack
```

### Idée clé

Le processus vu depuis l'utilisateur devient, côté noyau, une `task_struct` accompagnée de structures mémoire, fichiers, signaux, état d'ordonnancement, etc.

---

## 16. Les appels systèmes dans le noyau

### Où sont définis les syscalls ?

Les numéros de syscalls dépendent de l'architecture.

Exemples :

```text
arch/x86/entry/syscalls/syscall_32.tbl
arch/x86/entry/syscalls/syscall_64.tbl
arch/arm/tools/syscall.tbl
```

Dans `syscall_64.tbl`, on trouve par exemple :

```text
0   common  read    sys_read
1   common  write   sys_write
2   common  open    sys_open
3   common  close   sys_close
60  common  exit    sys_exit
```

### Implémentation avec `SYSCALL_DEFINE`

Le code déclenché par un syscall est déclaré avec une macro :

```c
SYSCALL_DEFINE1(exit, int, error_code)
{
    do_exit((error_code & 0xff) << 8);
}
```

Le chiffre indique le nombre de paramètres.

| Macro | Nombre de paramètres |
|---|---|
| `SYSCALL_DEFINE0` | 0 |
| `SYSCALL_DEFINE1` | 1 |
| `SYSCALL_DEFINE2` | 2 |
| `SYSCALL_DEFINE3` | 3 |
| `SYSCALL_DEFINE5` | 5 |

### Exemple : `exit`

Le programme assembleur :

```asm
mov $60, %rax
mov $2, %rdi
syscall
```

signifie :

- syscall numéro 60 : `exit` ;
- paramètre 1 : code de retour `2` ;
- passage en mode noyau via `syscall`.

---

## 17. Cycle réel d'exécution : `clone` -> `execve` -> `exit`

Quand on lance un programme depuis un shell, il ne se passe pas seulement `execve`.

### Trace simplifiée

```text
shell
  |
  |-- clone()   -> création d'un processus fils
        |
        |-- execve("./add") -> remplacement du code par le binaire demandé
              |
              |-- exit(2)   -> fin du programme
```

### `clone`

`clone` crée une nouvelle tâche.

Côté noyau :

```c
SYSCALL_DEFINE5(clone, ...)
{
    return _do_fork(...);
}
```

Puis :

```c
p = copy_process(...);
p = dup_task_struct(current, node);
```

Idée :

- le noyau duplique la `task_struct` courante ;
- initialise la nouvelle tâche ;
- l'insère dans les structures du noyau.

### `execve`

`execve` remplace l'image mémoire du processus fils par celle du programme demandé.

```c
SYSCALL_DEFINE3(execve,
    const char __user *, filename,
    const char __user *const __user *, argv,
    const char __user *const __user *, envp)
{
    return do_execve(getname(filename), argv, envp);
}
```

Rôle de `execve` :

- ouvrir le binaire ;
- préparer les credentials ;
- créer et remplir la `mm_struct` ;
- charger les segments du binaire ;
- préparer les arguments et l'environnement ;
- lancer l'exécution.

### `exit`

`exit` termine le processus.

Fonctions appelées typiquement dans `do_exit` :

```c
exit_mm();
exit_sem(tsk);
exit_shm(tsk);
exit_files(tsk);
exit_fs(tsk);
exit_thread(tsk);
exit_notify(tsk, group_dead);
do_task_dead();
```

Donc `exit` nettoie :

- mémoire ;
- sémaphores ;
- mémoire partagée ;
- fichiers ouverts ;
- système de fichiers ;
- thread ;
- notification au parent ;
- tâche morte.

### Résumé essentiel

```text
clone   -> crée une task_struct fille
execve  -> remplace le contenu mémoire par le programme demandé
exit    -> nettoie et termine la tâche
```

---

## 18. Logs noyau avec `dmesg`

`dmesg` permet d'accéder aux messages du noyau.

```bash
dmesg
```

Il donne les premières informations pour comprendre :

- un problème driver ;
- une erreur matérielle ;
- un crash noyau ;
- un module qui échoue ;
- un warning ;
- un kernel panic.

En interne, les messages viennent d'un ring buffer noyau, notamment exposé via :

```text
/proc/kmsg
```

### Utilisations utiles

```bash
dmesg | tail
sudo dmesg -w
sudo dmesg -T
```

| Commande | Utilité |
|---|---|
| `dmesg | tail` | voir les derniers messages |
| `dmesg -w` | suivre les messages en direct |
| `dmesg -T` | afficher des dates lisibles |

---

## 19. `debugfs`, `dynamic_debug` et tracing noyau

### `debugfs`

`debugfs` est un pseudo-système de fichiers permettant d'accéder à des fonctions de debug du noyau.

Montage :

```bash
sudo mkdir -p /mnt/debug
sudo mount -t debugfs none /mnt/debug
```

Chemin courant habituel :

```bash
ls /sys/kernel/debug/
```

### `dynamic_debug`

`dynamic_debug` permet d'activer ou désactiver dynamiquement certains messages de debug du noyau.

Fichier de contrôle :

```text
/sys/kernel/debug/dynamic_debug/control
```

Lister les messages activés :

```bash
awk '$3 != "=_"' /sys/kernel/debug/dynamic_debug/control
```

Intérêt : activer des messages précis sans recompiler tout le noyau.

### Tracing noyau avec ftrace

Le tracing noyau permet de suivre les fonctions activées pendant une période.

Fichiers importants :

| Fichier | Rôle |
|---|---|
| `set_ftrace_pid` | choisir le processus à observer |
| `current_tracer` | choisir le type de tracer |
| `tracing_on` | activer/désactiver le tracing |
| `trace` | lire le résultat |

Chemin typique :

```text
/sys/kernel/debug/tracing/
```

Exemple de lecture :

```bash
cat /sys/kernel/debug/tracing/trace
```

---

## 20. `perf` : tracing et statistiques

`perf` simplifie l'utilisation du tracing et des compteurs de performance.

### Commandes principales

| Commande | Rôle |
|---|---|
| `perf list` | liste les événements disponibles |
| `perf record` | enregistre une trace |
| `perf report` | affiche une trace enregistrée |
| `perf stat` | affiche des statistiques CPU |
| `perf probe` | crée des sondes personnalisées |
| `perf script` | affiche les événements enregistrés |

### Exemples

Lister les événements :

```bash
perf list
```

Mesurer les statistiques système :

```bash
sudo perf stat -a sleep 1
```

Tracer un événement ext4 :

```bash
sudo perf record -e ext4:ext4_free_inode -a
sudo perf report
```

Créer une sonde :

```bash
sudo perf probe -F
sudo perf probe -V fonction
sudo perf probe "fonction param"
sudo perf script
```

### Idée clé

`perf` permet d'observer le noyau sans écrire directement dans les fichiers de `debugfs`.

---

## 21. `crash` : debugger le noyau

`crash` est une version améliorée de `gdb` spécialisée pour l'analyse du noyau Linux.

Il permet :

- l'analyse live d'un système ;
- l'analyse post-mortem d'un dump noyau ;
- l'inspection des tâches ;
- l'affichage des piles noyau ;
- l'analyse des structures internes ;
- la lecture brute de mémoire.

### Principe

`crash` utilise :

- un noyau avec symboles de debug ;
- `/proc/kcore` pour une analyse live ;
- un dump mémoire pour une analyse post-mortem.

### Lancement

Analyse live :

```bash
sudo crash
```

Analyse post-mortem :

```bash
crash <vmlinux-avec-symboles> <vmcore>
```

### Pré-requis

Il faut installer les symboles de debug du noyau.

Exemples :

```text
linux-image-XXX-generic-dbgsym     Ubuntu/Debian
kernel-debuginfo                   RedHat/Fedora/CentOS
```

---

## 22. Kdump et Kexec

Le problème après un crash noyau est de récupérer la mémoire du noyau qui vient de planter.

### Kexec

`kexec` permet de charger un noyau depuis un noyau déjà en cours d'exécution, sans passer par un reboot complet matériel.

### Kdump

`kdump` utilise `kexec` pour précharger un noyau minimal de capture.

Quand le noyau principal plante :

1. le noyau de capture démarre ;
2. il récupère la mémoire du noyau planté ;
3. il produit un dump exploitable avec `crash`.

### Réservation mémoire

Au boot, il faut réserver de la mémoire pour le noyau de capture :

```text
crashkernel=auto
```

### Déclenchement d'un panic

Le comportement du kernel panic se configure via :

```text
/proc/sys/kernel/panic*
```

Déclenchements possibles :

- panic autonome ;
- magic SysRq ;
- NMI externe.

---

## 23. Autres outils : BPF/BCC et SystemTap

### BPF / BCC

Les outils BCC basés sur BPF permettent de tracer efficacement des fonctions noyau.

Exemple mentionné :

```bash
/usr/local/share/bcc/tools/gethostlatency
```

BPF est très utile pour :

- observer des appels noyau ;
- mesurer la latence ;
- suivre les accès fichiers ;
- suivre le réseau ;
- inspecter un système en production avec peu d'overhead.

### SystemTap

SystemTap permet d'insérer du code de tracing à différents endroits du noyau.

Exemple de script conceptuel :

```systemtap
global reads

probe vfs.read {
    reads[execname()] <<< count
}
```

SystemTap peut servir à :

- visualiser des variables ;
- inspecter des structures ;
- tracer des fonctions ;
- modifier temporairement le comportement observé.

---

## 24. Méthodologie pratique de debugging

### Cas 1 : programme utilisateur lent

1. Observer avec `top`.
2. Vérifier CPU, mémoire, état du processus.
3. Utiliser `strace -c` pour voir les syscalls dominants.
4. Utiliser `ltrace -c` si le problème semble dans une bibliothèque.
5. Utiliser `perf stat` ou `perf record` si le problème est performance bas niveau.
6. Utiliser `gdb -p PID` si le processus est bloqué.

### Cas 2 : programme qui plante

1. Activer les core files :

```bash
ulimit -c unlimited
```

2. Relancer le programme.
3. Analyser le core :

```bash
gdb ./programme core
```

4. Lire :

```gdb
where
info registers
frame 0
print variable
```

### Cas 3 : syscall inattendu

1. Lancer :

```bash
strace -f ./programme
```

2. Filtrer :

```bash
strace -e open,read,write,execve ./programme
```

3. Vérifier les erreurs :

```text
ENOENT, EACCES, EAGAIN, EPERM, EFAULT
```

### Cas 4 : problème noyau

1. Lire les logs :

```bash
dmesg -T | tail -100
```

2. Vérifier les modules et messages.
3. Activer `dynamic_debug` si nécessaire.
4. Utiliser `perf` ou `ftrace`.
5. En cas de crash, analyser le dump avec `crash`.

### Cas 5 : système gelé ou kernel panic

1. Configurer kdump.
2. Récupérer un `vmcore`.
3. Lancer :

```bash
crash vmlinux vmcore
```

4. Examiner :

```crash
ps
bt
set <PID>
bt
log
kmem
```

---

## 25. Commandes essentielles à retenir

### Observation utilisateur

```bash
ps aux f
top
cat /proc/self/stat
ls /proc/<PID>/
cat /proc/<PID>/status
cat /proc/<PID>/maps
```

### Tracing utilisateur/noyau

```bash
strace ./programme
strace -f ./programme
strace -e open,read,write ./programme
strace -c ./programme
strace -p <PID>
```

### Tracing bibliothèques

```bash
ltrace ./programme
ltrace -e malloc,free ./programme
ltrace -p <PID>
```

### GDB

```bash
gdb ./programme
gdb -p <PID>
gdb ./programme core
```

Commandes GDB :

```gdb
break main
run
where
bt
thread apply all bt
info registers
info source
disassemble main
continue
up
down
print variable
set variable=value
```

### Core files

```bash
ulimit -c unlimited
cat /proc/sys/kernel/core_pattern
gcore <PID>
```

### Logs noyau

```bash
dmesg
dmesg -T
dmesg -w
```

### debugfs / tracing

```bash
sudo mount -t debugfs none /sys/kernel/debug
cat /sys/kernel/debug/dynamic_debug/control
cat /sys/kernel/debug/tracing/trace
```

### perf

```bash
perf list
sudo perf stat -a sleep 1
sudo perf record -a
sudo perf report
sudo perf probe -F
sudo perf script
```

### crash

```bash
sudo crash
crash vmlinux vmcore
```

Commandes crash :

```crash
ps
set <PID>
bt
bt -f
dis <fonction>
whatis <symbole>
print <symbole>
struct <structure>
help <commande>
foreach
mod
kmem
rd
```

---

## 26. Questions d'examen possibles

### Question 1 - Qu'est-ce que le debugging ?

Le debugging est l'ensemble des méthodes permettant de rechercher, comprendre et corriger des erreurs ou comportements anormaux dans une application, un système ou le noyau.

### Question 2 - Quels sont les trois univers d'un système d'exploitation ?

Les trois univers sont :

1. l'espace utilisateur ;
2. l'espace noyau ;
3. le matériel.

L'espace utilisateur passe par des syscalls pour demander des services au noyau. Le noyau communique avec le matériel via ses drivers.

### Question 3 - À quoi sert `%rip` ?

`%rip` est l'instruction pointer. Il contient l'adresse de la prochaine instruction à exécuter.

### Question 4 - À quoi servent `%rsp` et `%rbp` ?

`%rsp` pointe vers le sommet courant de la pile. `%rbp` pointe vers la base du cadre de pile courant.

### Question 5 - Que fait `call` ?

`call` sauvegarde l'adresse de retour sur la pile puis place dans `%rip` l'adresse de la fonction appelée.

### Question 6 - Que fait `ret` ?

`ret` récupère l'adresse de retour depuis la pile et la replace dans `%rip`.

### Question 7 - Quelle est la différence entre `strace` et `ltrace` ?

`strace` trace les appels systèmes, donc les interactions avec le noyau. `ltrace` trace les appels aux bibliothèques dynamiques.

### Question 8 - Pourquoi `/proc` est important ?

`/proc` expose des informations noyau et processus. Des outils comme `ps` et `top` s'appuient sur `/proc` pour afficher l'état des processus.

### Question 9 - À quoi sert `gdb` ?

`gdb` sert à debugger un programme utilisateur : breakpoints, exécution pas à pas, pile d'appels, registres, variables, désassemblage, analyse de core files.

### Question 10 - Qu'est-ce qu'un core file ?

Un core file est une image mémoire d'un processus à un instant donné, souvent après un crash. Il permet une analyse post-mortem avec `gdb`.

### Question 11 - Qu'est-ce qu'une `task_struct` ?

La `task_struct` est la structure noyau représentant une tâche ou un processus. Elle contient son état, son contexte, ses informations mémoire, ses fichiers, ses signaux et ses informations d'ordonnancement.

### Question 12 - Qu'est-ce qu'une `mm_struct` ?

La `mm_struct` décrit l'espace mémoire d'un processus : segments code, data, bss, heap, stack, zones mmap, etc.

### Question 13 - Comment sont déclarés les syscalls dans le noyau Linux ?

Ils sont déclarés avec des macros `SYSCALL_DEFINEn`, où `n` représente le nombre de paramètres du syscall.

### Question 14 - Que se passe-t-il quand on lance un programme depuis un shell ?

Le shell appelle `clone` pour créer un processus fils. Le fils appelle `execve` pour charger le nouveau programme. Quand le programme finit, il appelle `exit`, puis le noyau nettoie la tâche.

### Question 15 - À quoi sert `dmesg` ?

`dmesg` affiche les messages du noyau stockés dans un ring buffer. Il sert à diagnostiquer les problèmes noyau, drivers, matériel et boot.

### Question 16 - À quoi sert `debugfs` ?

`debugfs` est un pseudo-système de fichiers qui expose des fonctions de debug du noyau.

### Question 17 - À quoi sert `perf` ?

`perf` permet de mesurer des performances, enregistrer des traces, lister des événements, créer des sondes et analyser l'activité CPU/noyau.

### Question 18 - À quoi sert `crash` ?

`crash` permet d'analyser le noyau Linux en live ou post-mortem à partir d'un dump mémoire, avec les symboles de debug du noyau.

### Question 19 - Pourquoi faut-il les symboles de debug ?

Ils permettent de remplacer les adresses brutes par des noms de fonctions, variables, structures et lignes de code, ce qui rend l'analyse compréhensible.

### Question 20 - Quel est le rôle de Kdump ?

Kdump permet de récupérer un dump mémoire après un kernel panic en démarrant un noyau de capture préchargé via kexec.

---

## 27. Résumé ultra-court

```text
Debugging = observer + comprendre + vérifier + corriger.

User space : programmes utilisateur.
Kernel space : noyau, drivers, syscalls.
Hardware : CPU, mémoire, périphériques.

Programme : source -> objet -> exécutable -> mémoire.
Mémoire : text, data, bss, heap, stack.
Pile : rbp, rsp, rip, push, pop, call, ret.

ps/top lisent /proc.
strace trace les syscalls.
ltrace trace les bibliothèques.
gdb inspecte le processus.
core file permet l'analyse post-mortem.

Côté noyau :
task_struct = processus vu par le noyau.
mm_struct = mémoire du processus.
clone crée une tâche.
execve charge un binaire.
exit nettoie la tâche.

dmesg lit les logs noyau.
debugfs expose les mécanismes de debug.
perf facilite le tracing et les mesures.
crash analyse le noyau en live ou post-mortem.
BPF/BCC et SystemTap permettent le tracing avancé.
```

---

## Mini-fiche de révision finale

### Outil à choisir selon le problème

| Problème | Outil conseillé |
|---|---|
| Voir les processus actifs | `ps`, `top` |
| Comprendre les syscalls | `strace` |
| Comprendre les appels libc/librairies | `ltrace` |
| Programme qui plante | `gdb`, core file |
| Programme bloqué | `gdb -p`, `strace -p` |
| Problème de logs noyau | `dmesg` |
| Activer logs noyau détaillés | `dynamic_debug` |
| Tracer fonctions noyau | `ftrace`, `perf`, BPF |
| Analyser kernel panic | `kdump`, `crash` |
| Naviguer dans gros code C | `cscope` |

### Formule mentale

```text
Symptôme utilisateur
    -> processus ? ps/top/proc
    -> syscall ? strace
    -> bibliothèque ? ltrace
    -> mémoire/pile ? gdb
    -> noyau ? dmesg/debugfs/perf
    -> crash noyau ? kdump/crash
```
