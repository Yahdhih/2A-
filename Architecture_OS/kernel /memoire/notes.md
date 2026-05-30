# Notes de cours — Allocation Mémoire

> **Module** : Architecture OS — ARSE · ENSIIE  
> **Version EN** : [notes.en.md](notes.en.md)  
> **Thèmes** : mémoire physique, mémoire virtuelle, malloc/free, pagination, swap, TLB, remplacement de pages, NUMA, allocation mémoire en HPC, Unified Memory GPU.

---

> **Concepts essentiels**
> - Page (logique) → Frame (physique) via table des pages ; TLB = cache de traductions.
> - Défaut de page = page absente de la RAM → chargement depuis swap.
> - Algorithmes de remplacement : FIF (optimal), LRU (efficace), Clock (approximation de LRU), FIFO (simple).
> - NUMA : accéder à la mémoire locale est rapide ; first-touch = la page va sur le nœud du premier thread qui y accède.
> - Unified Memory CUDA : pointeur unique CPU/GPU + migration automatique (Pascal+).
> - Thrashing = trop de swap → working set pour y remédier.

---

## Table des matières

1. [Vue globale du cours](#1-vue-globale-du-cours)
2. [Mémoire physique](#2-mémoire-physique)
3. [Mémoire vue par le programmeur](#3-mémoire-vue-par-le-programmeur)
4. [Variables statiques, automatiques et dynamiques](#4-variables-statiques-automatiques-et-dynamiques)
5. [`malloc` / `free`](#5-malloc--free)
6. [Fragmentation mémoire](#6-fragmentation-mémoire)
7. [Problème des adresses logiques](#7-problème-des-adresses-logiques)
8. [Mémoire virtuelle](#8-mémoire-virtuelle)
9. [Pages, frames et table des pages](#9-pages-frames-et-table-des-pages)
10. [TLB : Translation Lookaside Buffer](#10-tlb--translation-lookaside-buffer)
11. [Swap et chargement paresseux](#11-swap-et-chargement-paresseux)
12. [Défaut de page](#12-défaut-de-page)
13. [Bits de validité et dirty bit](#13-bits-de-validité-et-dirty-bit)
14. [Algorithmes de remplacement de pages](#14-algorithmes-de-remplacement-de-pages)
15. [Gestion globale et locale des frames](#15-gestion-globale-et-locale-des-frames)
16. [Thrashing et Working Set](#16-thrashing-et-working-set)
17. [Tables des pages hiérarchiques](#17-tables-des-pages-hiérarchiques)
18. [`mmap`](#18-mmap)
19. [NUMA et politique First Touch](#19-numa-et-politique-first-touch)
20. [Rôle du système d’exploitation dans la mémoire virtuelle](#20-rôle-du-système-dexploitation-dans-la-mémoire-virtuelle)
21. [Allocation mémoire en espace utilisateur](#21-allocation-mémoire-en-espace-utilisateur)
22. [Allocateurs mémoire et multithreading](#22-allocateurs-mémoire-et-multithreading)
23. [Allocation mémoire en contexte HPC](#23-allocation-mémoire-en-contexte-hpc)
24. [Page fault cost et page zeroing](#24-page-fault-cost-et-page-zeroing)
25. [Allocation physique des pages et cache](#25-allocation-physique-des-pages-et-cache)
26. [Unified Memory sur GPU](#26-unified-memory-sur-gpu)
27. [Évolution Kepler, Pascal, Volta](#27-évolution-kepler-pascal-volta)
28. [Heuristiques du driver Unified Memory](#28-heuristiques-du-driver-unified-memory)
29. [Optimisations utilisateur CUDA](#29-optimisations-utilisateur-cuda)
30. [Applications HPC, Deep Learning et Graph Analytics](#30-applications-hpc-deep-learning-et-graph-analytics)
31. [Résumé ultra-important pour l’examen](#31-résumé-ultra-important-pour-lexamen)
32. [Questions-réponses possibles](#32-questions-réponses-possibles)

---

# 1. Vue globale du cours

Le cours porte sur **l’allocation mémoire** dans un système d’exploitation, depuis les mécanismes bas niveau du noyau jusqu’aux problématiques de performance dans le calcul haute performance et sur GPU.

Le plan général est :

- allocation mémoire en **espace noyau** ;
- allocation mémoire en **espace utilisateur** ;
- allocation mémoire en **contexte HPC** ;
- mémoire unifiée sur **GPU**.

L’idée centrale est la suivante :

> Un programme croit manipuler une mémoire simple, continue et privée. En réalité, le système d’exploitation, le processeur, le noyau, les allocateurs, le disque, le cache, NUMA et parfois le GPU collaborent pour donner cette illusion.

---

# 2. Mémoire physique

## 2.1 Définition

La mémoire physique correspond à la **RAM réelle** de la machine.

Elle peut être vue comme :

- un grand tableau de cellules mémoire ;
- chaque cellule contient un nombre fixe de bits ;
- chaque cellule possède une adresse ;
- l’information contenue dans une cellule est appelée un **mot mémoire**.

Exemple simplifié :

```text
Adresse :   0     1     2     3     4     ...
Contenu :  1010  0011  1110  0001  1001  ...
```

## 2.2 RAM

RAM signifie **Random Access Memory**.

Cela veut dire que le processeur peut accéder directement à n’importe quelle case mémoire sans devoir parcourir toutes les précédentes.

La taille mémoire dépend du type de machine :

- téléphone : quelques Go ;
- ordinateur personnel : plusieurs Go ou dizaines de Go ;
- nœud de supercalculateur : jusqu’à plusieurs centaines de Go ou 1 To ;
- machine HPC : mémoire répartie sur plusieurs nœuds NUMA.

## 2.3 Mémoire physique et NUMA

Dans les machines modernes, la mémoire peut être organisée en **nœuds NUMA**.

NUMA signifie **Non Uniform Memory Access**.

Cela veut dire que :

- chaque processeur ou socket possède une mémoire locale ;
- accéder à la mémoire locale est rapide ;
- accéder à la mémoire attachée à un autre socket est plus lent.

Schéma simplifié :

```text
CPU 0 ---- Mémoire NUMA 0
CPU 1 ---- Mémoire NUMA 1
CPU 2 ---- Mémoire NUMA 2
CPU 3 ---- Mémoire NUMA 3
```

Conséquence : en HPC, il ne suffit pas d’avoir assez de mémoire. Il faut aussi que les données soient proches du processeur qui les utilise.

---

# 3. Mémoire vue par le programmeur

Quand un programmeur écrit un programme C, il ne manipule pas directement la mémoire physique. Il manipule un **espace d’adressage logique**.

Cet espace contient généralement :

```text
+----------------------------+
| Zone texte                 |  Code du programme
+----------------------------+
| Zone donnée                |  Variables globales / statiques
+----------------------------+
| Tas / Heap                 |  malloc / free
|                            |  grandit vers le haut
+----------------------------+
| Zone non utilisée          |
+----------------------------+
| Pile / Stack               |  variables locales, appels de fonctions
|                            |  grandit vers le bas
+----------------------------+
```

## 3.1 Zone texte

La **zone texte** contient le code exécutable du programme.

Exemple :

```c
int main() {
    return 0;
}
```

Le code machine généré pour `main` est placé dans la zone texte.

## 3.2 Zone donnée

La **zone donnée** contient les variables globales ou statiques.

Exemple :

```c
int compteur = 0;

int main() {
    compteur++;
    return 0;
}
```

Ici, `compteur` vit pendant toute l’exécution du programme.

## 3.3 Pile ou stack

La pile contient les variables locales et les informations liées aux appels de fonctions.

Exemple :

```c
void f() {
    int x = 10;
}
```

La variable `x` existe seulement pendant l’exécution de `f`.

Quand la fonction se termine, `x` disparaît automatiquement.

## 3.4 Tas ou heap

Le tas contient les variables allouées dynamiquement.

Exemple :

```c
int *p = malloc(sizeof(int));
*p = 42;
free(p);
```

Ici, la durée de vie de la mémoire pointée par `p` est contrôlée par le programmeur.

---

# 4. Variables statiques, automatiques et dynamiques

## 4.1 Variables statiques

Une variable statique a une durée de vie égale à celle du programme.

Elle peut être :

- globale ;
- déclarée avec le mot-clé `static`.

Exemple :

```c
int g = 5;          // variable globale

void f() {
    static int c = 0;  // variable statique locale
    c++;
}
```

Même si `c` est déclarée dans une fonction, elle conserve sa valeur entre deux appels.

## 4.2 Variables automatiques

Une variable automatique est une variable locale.

Elle est créée quand on entre dans une fonction ou un bloc, puis détruite quand on en sort.

Exemple :

```c
void f() {
    int x = 3;  // variable automatique
}
```

## 4.3 Variables dynamiques

Une variable dynamique est allouée dans le tas.

Elle est créée avec `malloc`, puis libérée avec `free`.

Exemple :

```c
int *tab = malloc(100 * sizeof(int));
free(tab);
```

Elle ne disparaît pas automatiquement : si on oublie `free`, on crée une fuite mémoire.

---

# 5. `malloc` / `free`

## 5.1 Rôle

`malloc` et `free` sont les fonctions principales d’allocation dynamique en C.

```c
#include <stdlib.h>

void *malloc(size_t size);
void free(void *ptr);
```

- `malloc(size)` demande un bloc mémoire de `size` octets ;
- `free(ptr)` libère un bloc précédemment alloué.

## 5.2 Où se trouvent `malloc` et `free` ?

`malloc` et `free` appartiennent à l’**espace utilisateur**.

Le programme appelle `malloc`, mais l’allocateur mémoire de la libc peut ensuite appeler le noyau si nécessaire.

## 5.3 Pourquoi `malloc` ne fait pas toujours un appel système ?

Un appel système est coûteux.

Donc l’allocateur mémoire essaie de :

- demander de gros blocs au noyau ;
- les découper lui-même pour répondre à plusieurs appels `malloc` ;
- éviter d’appeler le noyau à chaque allocation.

## 5.4 Appels systèmes utilisés

Deux mécanismes importants :

| Mécanisme | Rôle |
|---|---|
| `brk` / `sbrk` | agrandir ou réduire le tas |
| `mmap` | demander une zone mémoire indépendante, souvent pour les grosses allocations |

## 5.5 Pourquoi `malloc` est critique pour les performances ?

Dans les programmes C/C++, surtout en HPC, `malloc` peut être très coûteux à cause de :

- nombreux appels concurrents par plusieurs threads ;
- verrous internes de l’allocateur ;
- fragmentation ;
- mauvaise localité NUMA ;
- page faults ;
- initialisation à zéro des pages ;
- interaction avec le cache.

---

# 6. Fragmentation mémoire

## 6.1 Définition

La fragmentation apparaît après une succession d’allocations et de libérations.

Exemple :

```text
Avant :
[ libre libre libre libre libre libre ]

Après plusieurs malloc/free :
[ utilisé ][ libre ][ utilisé ][ libre ][ utilisé ][ libre ]
```

Même si la somme des zones libres est grande, il peut être impossible d’allouer un gros bloc continu.

## 6.2 Fragmentation interne et externe

### Fragmentation interne

La mémoire allouée est plus grande que la mémoire réellement demandée.

Exemple :

```text
Demande : 13 octets
Bloc donné : 16 octets
Perte : 3 octets
```

### Fragmentation externe

La mémoire libre existe, mais elle est découpée en petits morceaux dispersés.

Exemple :

```text
Libre total = 100 Mo
Mais aucun bloc libre continu de 50 Mo
```

## 6.3 Stratégies de placement

Quand un nouveau `malloc` arrive, l’allocateur doit choisir où placer le bloc.

| Stratégie | Principe | Avantage | Inconvénient |
|---|---|---|---|
| First Fit | prendre le premier trou suffisant | rapide | peut fragmenter au début |
| Best Fit | prendre le plus petit trou suffisant | limite le gaspillage immédiat | recherche plus coûteuse, petits trous restants |
| Worst Fit | prendre le plus grand trou | garde de gros espaces | peut gaspiller beaucoup |

---

# 7. Problème des adresses logiques

## 7.1 Le problème

Chaque programme croit avoir son propre espace mémoire.

Exemple :

```text
Processus A utilise l'adresse 0x1000
Processus B utilise aussi l'adresse 0x1000
```

Cela semble impossible si les deux s’exécutent en même temps.

## 7.2 Deux problèmes fondamentaux

### Problème 1 : collision d’adresses

Plusieurs processus peuvent utiliser les mêmes adresses logiques.

Le système doit donc faire en sorte que :

```text
Adresse logique 0x1000 du processus A ≠ adresse logique 0x1000 du processus B
```

### Problème 2 : mémoire logique totale supérieure à la mémoire physique

La somme des mémoires demandées par tous les processus peut dépasser la RAM disponible.

Exemple :

```text
RAM physique : 8 Go
Processus A : 4 Go
Processus B : 4 Go
Processus C : 4 Go
Total demandé : 12 Go
```

Le système doit alors charger seulement une partie des processus en RAM et mettre le reste sur disque.

---

# 8. Mémoire virtuelle

## 8.1 Définition

La mémoire virtuelle est un mécanisme qui sépare :

- l’espace d’adressage logique vu par le programme ;
- l’espace d’adressage physique réel de la RAM.

Elle permet :

- l’isolation entre processus ;
- l’exécution de programmes plus grands que la mémoire physique ;
- le chargement à la demande ;
- la protection mémoire ;
- le partage contrôlé de pages.

## 8.2 Pages et frames

La mémoire est découpée en blocs de même taille.

| Élément | Description |
|---|---|
| Page | bloc de mémoire logique |
| Frame | bloc de mémoire physique |

La taille classique est :

```text
4 Ko = 2^12 octets
```

Il existe aussi des **huge pages**, par exemple :

```text
2 Mo
```

## 8.3 Adresse logique

Une adresse logique est découpée en deux parties :

```text
Adresse logique = numéro de page + déplacement
```

Exemple avec des pages de 4 Ko :

```text
Adresse 64 bits :
[ numéro de page ][ déplacement ]
        52 bits        12 bits
```

Le déplacement indique la position à l’intérieur de la page.

---

# 9. Pages, frames et table des pages

## 9.1 Table des frames

La table des frames est gérée par le système d’exploitation.

Elle contient une entrée par frame physique.

Pour chaque frame, elle indique :

- libre ou occupée ;
- si occupée : par quel processus ;
- quelle page logique est stockée dans cette frame.

## 9.2 Table des pages

Chaque processus possède sa propre table des pages.

Elle fait partie du contexte du processus.

Elle permet de traduire :

```text
page logique → frame physique
```

Exemple :

```text
Processus A
Page 0 → Frame 10
Page 1 → Frame 3
Page 2 → sur disque
Page 3 → Frame 25
```

## 9.3 Traduction d’adresse

Si une adresse logique est :

```text
page = p
offset = d
```

Alors la table des pages donne :

```text
page p → frame f
```

L’adresse physique devient :

```text
adresse physique = frame f + déplacement d
```

---

# 10. TLB : Translation Lookaside Buffer

## 10.1 Problème

Si chaque accès mémoire nécessite de lire la table des pages en RAM, alors chaque accès devient très lent.

Pour accéder à une donnée, il faudrait :

1. lire la table des pages ;
2. trouver la frame ;
3. lire la donnée en mémoire.

Cela peut presque doubler le coût d’un accès mémoire.

## 10.2 Solution : TLB

Le TLB est un petit cache matériel spécialisé dans la traduction d’adresses.

Il stocke les traductions récentes :

```text
Page logique → Frame physique
```

## 10.3 TLB hit

Si la traduction est dans le TLB :

```text
page trouvée dans TLB → traduction rapide
```

C’est un **TLB hit**.

## 10.4 TLB miss

Si la traduction n’est pas dans le TLB :

```text
page absente du TLB → consultation de la table des pages en mémoire
```

C’est un **TLB miss**.

C’est plus lent.

## 10.5 Changement de processus

Quand le système change de processus, le TLB peut contenir des traductions de l’ancien processus.

Il faut donc :

- vider le TLB ;
- ou utiliser des identifiants de contexte selon les architectures.

Dans le cours, on retient surtout que l’exécution d’un processus implique une gestion du TLB.

---

# 11. Swap et chargement paresseux

## 11.1 Problème

Que faire si :

```text
nombre total de pages utilisées > nombre de frames en RAM
```

Réponse : utiliser le disque comme extension de la mémoire.

## 11.2 Swap

Le **swap** est une zone disque utilisée pour stocker temporairement des pages mémoire.

Il peut être :

- une partition spécifique ;
- un fichier de swap.

## 11.3 Chargement paresseux

Le système ne charge pas forcément toutes les pages d’un processus au lancement.

Il charge une page seulement quand elle est utilisée pour la première fois.

C’est la **pagination à la demande**.

Exemple :

```text
Le programme possède 1000 pages logiques.
Au début, seules quelques pages sont réellement chargées.
Les autres restent sur disque.
```

## 11.4 Si mémoire + swap insuffisants

Si la demande totale dépasse :

```text
RAM + swap
```

le système peut tuer un processus consommant beaucoup de mémoire.

Sur Linux, cela peut impliquer le mécanisme OOM Killer.

---

# 12. Défaut de page

## 12.1 Définition

Un défaut de page, ou **page fault**, se produit quand un processus accède à une page qui n’est pas actuellement en mémoire physique.

## 12.2 Cas possibles lors d’une référence mémoire

Quand une page est référencée :

1. la référence est invalide ;
2. la page est en mémoire ;
3. la page est sur disque.

### Cas 1 : référence invalide

Le processus tente d’accéder à une adresse interdite.

Exemple :

```c
int *p = NULL;
*p = 5;
```

Cela peut provoquer une erreur de segmentation.

### Cas 2 : page en mémoire

La traduction est valide, l’exécution continue.

### Cas 3 : page sur disque

Le système déclenche un défaut de page.

Il doit charger la page depuis le disque vers une frame libre.

## 12.3 Étapes d’un défaut de page

1. Le programme accède à une adresse.
2. Le matériel détecte que la page n’est pas en RAM.
3. Une interruption/trap est envoyée au système d’exploitation.
4. Le système localise la page sur disque.
5. Il trouve une frame libre ou choisit une page victime.
6. Il charge la page demandée en mémoire.
7. Il met à jour la table des pages.
8. Il relance l’instruction interrompue.

---

# 13. Bits de validité et dirty bit

## 13.1 Bit de validité

La table des pages contient un bit de validité.

| Bit | Signification |
|---|---|
| 1 | page en mémoire |
| 0 | page absente de la mémoire, souvent sur disque |

Si le bit vaut 0 et que la page est utilisée, cela déclenche un défaut de page.

## 13.2 Dirty bit

Le dirty bit indique si la page a été modifiée.

| Dirty bit | Signification |
|---|---|
| 0 | page non modifiée |
| 1 | page modifiée |

## 13.3 Pourquoi le dirty bit est important ?

Quand on remplace une page :

- si elle n’a pas été modifiée, inutile de la réécrire sur disque ;
- si elle a été modifiée, il faut la sauvegarder.

Donc le dirty bit évite des écritures disque inutiles.

---

# 14. Algorithmes de remplacement de pages

## 14.1 Objectif

Quand il n’y a plus de frame libre, le système doit choisir une page à retirer de la RAM.

Cette page est appelée **page victime**.

Objectif :

```text
minimiser le nombre de défauts de page
```

## 14.2 Algorithmes du cours

Le cours cite :

- Random ;
- FIFO ;
- FIF ;
- LRU ;
- Clock.

## 14.3 Random

Random choisit une page au hasard.

Avantage :

- très simple.

Inconvénient :

- peut remplacer une page très utile.

## 14.4 FIFO

FIFO signifie **First In First Out**.

Principe :

> La page chargée depuis le plus longtemps est remplacée en premier.

On gère les pages comme une file d’attente.

Exemple :

```text
Ordre d’arrivée : A, B, C
Victime FIFO : A
```

Avantage :

- simple à implémenter.

Inconvénient :

- ne tient pas compte de l’utilisation réelle.

Une vieille page peut être très utilisée.

## 14.5 FIF / Optimal

FIF signifie **Furthest In Future**.

Principe :

> Remplacer la page qui sera utilisée le plus tard dans le futur.

C’est l’algorithme optimal en nombre de défauts de page.

Mais il est impossible en pratique, car il faut connaître le futur.

Il sert surtout de référence théorique.

## 14.6 LRU

LRU signifie **Least Recently Used**.

Principe :

> Remplacer la page qui n’a pas été utilisée depuis le plus longtemps.

Idée : si une page n’a pas été utilisée récemment, elle a moins de chances d’être utilisée bientôt.

Avantage :

- souvent performant.

Inconvénient :

- coûteux à implémenter exactement.

## 14.7 Clock

Clock est aussi appelé **algorithme de la deuxième chance**.

Principe :

- chaque page a un bit de référence ;
- quand la page est utilisée, bit = 1 ;
- les pages sont organisées en liste circulaire ;
- si le pointeur rencontre une page avec bit = 0, elle est remplacée ;
- si bit = 1, on remet le bit à 0 et on passe à la page suivante.

Pseudo-code :

```text
tant que vrai :
    si bit_reference(page_courante) == 0 :
        remplacer cette page
        arrêter
    sinon :
        bit_reference(page_courante) = 0
        avancer au prochain élément
```

Clock approxime LRU avec un coût plus faible.

## 14.8 Classement donné dans le cours

Le classement théorique présenté est :

```text
FIF > LRU > Clock > FIFO > Random
```

Mais le résultat réel dépend des programmes.

---

# 15. Gestion globale et locale des frames

## 15.1 Question centrale

Comment partager les frames physiques entre plusieurs processus ?

Deux approches :

- gestion globale ;
- gestion locale.

## 15.2 Gestion globale

Quand un processus demande une frame, le système peut prendre une frame appartenant à un autre processus.

Avantage :

- allocation dynamique ;
- meilleure adaptation à la charge réelle.

Inconvénient :

- un processus peut voler trop de frames aux autres ;
- risque de thrashing.

## 15.3 Gestion locale

Quand un processus doit remplacer une page, il choisit uniquement parmi ses propres frames.

Avantage :

- isolation ;
- nombre de frames par processus plus stable.

Inconvénient :

- moins flexible ;
- un processus peut manquer de mémoire alors que d’autres ont des frames peu utiles.

---

# 16. Thrashing et Working Set

## 16.1 Thrashing

Le thrashing se produit quand le système passe plus de temps à charger/décharger des pages qu’à exécuter réellement les programmes.

Symptôme :

```text
beaucoup de défauts de page
beaucoup d'E/S disque
CPU peu utilisé utilement
système très lent
```

## 16.2 Pourquoi le remplacement global peut provoquer du thrashing ?

Situation typique :

1. un processus provoque des défauts de page ;
2. il attend le disque ;
3. le processeur lance d’autres processus ;
4. ces processus manquent aussi de pages ;
5. le système charge encore plus de processus ;
6. la mémoire devient insuffisante ;
7. les processus passent leur temps à échanger des pages avec le disque.

Résultat : le système s’effondre en performance.

## 16.3 Working Set

Le **working set** d’un processus est l’ensemble des pages utilisées récemment.

Définition simple :

```text
Working set = pages accédées pendant la dernière fenêtre de temps
```

Exemple :

```text
Références récentes : 1, 2, 5, 6, 7
Working set = {1, 2, 5, 6, 7}
```

## 16.4 Algorithme du working set

Si :

```text
somme des working sets > mémoire physique
```

alors le système peut suspendre un processus et décharger ses pages.

Objectif : éviter le thrashing.

## 16.5 Courbe working set / défauts de page

Plus un processus possède de frames, plus son taux de défauts de page diminue.

Mais au-delà d’un certain seuil, ajouter des frames améliore peu les performances.

Idée :

```text
Peu de frames  → beaucoup de page faults
Assez de frames → page faults faibles
Trop de frames → mémoire gaspillée
```

---

# 17. Tables des pages hiérarchiques

## 17.1 Problème de taille

Sur une architecture 64 bits, l’espace logique peut être énorme.

Avec des pages de 4 Ko :

```text
Taille page = 2^12 octets
Nombre possible de pages ≈ 2^52
```

Une table des pages plate serait gigantesque.

## 17.2 Solution

On découpe la table des pages en plusieurs niveaux.

Adresse logique simplifiée :

```text
[ numéro page externe ][ numéro page interne ][ déplacement ]
```

Ainsi, on ne crée que les sous-tables nécessaires.

## 17.3 Intuition

Au lieu d’avoir un énorme tableau complet, on utilise une structure arborescente.

```text
Table niveau 1
   ├── Table niveau 2
   │      ├── entrée page
   │      └── entrée page
   └── Table niveau 2
          └── entrée page
```

Cela économise beaucoup de mémoire.

---

# 18. `mmap`

## 18.1 Prototype

```c
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
```

## 18.2 Rôle

`mmap` projette un fichier ou une zone mémoire dans l’espace d’adressage du processus.

Au lieu de faire :

```c
read(fd, buffer, size);
write(fd, buffer, size);
```

on peut accéder au fichier comme à de la mémoire.

## 18.3 Avantages

- simplifie les entrées/sorties ;
- peut être plus rapide ;
- utilise la pagination à la demande ;
- le fichier n’est pas entièrement chargé en mémoire ;
- permet de recouvrir chargement et calcul.

## 18.4 Idée simple

```text
Une portion du fichier disque est associée à une zone mémoire virtuelle.
Quand le programme lit cette zone, le système charge les pages nécessaires.
```

---

# 19. NUMA et politique First Touch

## 19.1 Pourquoi NUMA est important ?

Dans une machine NUMA, l’accès à la mémoire dépend de la proximité entre :

- le thread qui exécute le code ;
- le nœud mémoire où se trouvent les données.

Deux paramètres sont critiques :

- latence ;
- bande passante.

## 19.2 First Touch

Linux utilise souvent la politique **first touch**.

Principe :

> La page physique est allouée sur le nœud NUMA du thread qui y accède pour la première fois.

Exemple :

```text
Thread sur CPU 0 touche la page en premier
→ page placée sur le nœud NUMA proche de CPU 0
```

## 19.3 Risque

Si un thread initialise toutes les données, mais que d’autres threads les utilisent ensuite, les pages peuvent être mal placées.

Exemple :

```c
// Mauvais en NUMA si un seul thread initialise tout
for (int i = 0; i < N; i++) {
    tab[i] = 0;
}
```

Si ensuite plusieurs threads utilisent `tab`, ils peuvent accéder à de la mémoire distante.

## 19.4 Solution

Initialiser les données en parallèle avec les mêmes threads qui vont les utiliser.

On peut aussi utiliser :

```bash
numactl --interleave=all ./programme
```

ou la bibliothèque `libnuma`.

---

# 20. Rôle du système d’exploitation dans la mémoire virtuelle

Le cours identifie quatre moments où le système d’exploitation intervient.

## 20.1 Création du processus

Le système doit :

- décider de la taille de la table des pages ;
- créer la table des pages ;
- préparer l’espace logique du processus.

## 20.2 Exécution du processus

Lors de l’exécution, le système doit :

- gérer le contexte mémoire ;
- remettre à zéro ou gérer le TLB lors des changements de contexte.

## 20.3 Défaut de page

Lors d’un défaut de page, le système doit :

- déterminer l’adresse virtuelle fautive ;
- localiser la page demandée ;
- choisir une frame libre ou une page victime ;
- charger la page ;
- mettre à jour la table des pages.

## 20.4 Terminaison du processus

Quand le processus se termine, le système doit :

- libérer la table des pages ;
- libérer les frames utilisées ;
- nettoyer les structures associées.

---

# 21. Allocation mémoire en espace utilisateur

## 21.1 Objectif de l’allocateur utilisateur

L’allocateur mémoire en espace utilisateur cherche à réduire le nombre d’appels au noyau.

Idée :

```text
Le programme appelle souvent malloc.
L’allocateur regroupe ces demandes.
Le noyau est appelé moins souvent.
```

## 21.2 Limites

Les allocateurs classiques peuvent être peu efficaces en HPC parce qu’ils ont :

- un support NUMA limité ;
- peu de connaissance sur la localité des données ;
- une gestion multithread parfois coûteuse ;
- des contentions sur des verrous ;
- une vision trop séquentielle.

---

# 22. Allocateurs mémoire et multithreading

## 22.1 Linux sans arena

Approche centralisée :

- un verrou à l’entrée de `malloc` ;
- pas de distinction entre threads ;
- pas de notion NUMA ;
- dépend de la politique first touch ;
- pas de lien avec l’ordonnanceur.

Problème : plusieurs threads peuvent se bloquer sur le même verrou.

## 22.2 Linux avec arena

Les arenas permettent de réduire la contention.

Idée :

```text
Plusieurs zones internes d’allocation
→ plusieurs threads peuvent allouer en parallèle
```

Mais cela ne résout pas entièrement :

- la localité NUMA ;
- la migration des threads ;
- la relation avec l’ordonnanceur.

## 22.3 BSD

Approche plus hiérarchique :

- prise en compte des threads ;
- verrous distribués ;
- mais pas forcément de support NUMA ;
- pas de lien avec l’ordonnanceur.

## 22.4 False sharing

Le false sharing arrive lorsque deux threads modifient des données différentes mais situées sur la même ligne de cache.

Exemple :

```text
Cache line : [ variable A ][ variable B ]
Thread 1 modifie A
Thread 2 modifie B
```

Même si A et B sont différentes, les processeurs se disputent la même ligne de cache.

Effet : baisse importante des performances.

---

# 23. Allocation mémoire en contexte HPC

## 23.1 Pourquoi l’allocation mémoire est spéciale en HPC ?

En HPC, les programmes utilisent :

- beaucoup de mémoire ;
- beaucoup de threads ;
- plusieurs sockets NUMA ;
- des données volumineuses ;
- des accès mémoire très fréquents.

Donc une mauvaise allocation mémoire peut détruire les performances.

## 23.2 Problèmes des allocateurs classiques

Les allocateurs classiques :

- réduisent les appels au système, mais ;
- allouent parfois sans connaissance NUMA ;
- supportent mal les applications fortement multithreadées ;
- provoquent des contentions si plusieurs threads visent la même zone mémoire.

## 23.3 Thread pools

Une solution est d’utiliser des pools par thread.

Chaque thread ou groupe de threads gère localement ses allocations.

Avantages :

- meilleure localité ;
- moins de contention ;
- moins de false sharing ;
- accès mémoire plus prévisibles.

## 23.4 Macro-blocs

Les thread pools échangent avec la source mémoire via de gros blocs, appelés macro-blocs.

Dans le cours, on parle de macro-blocs de taille supérieure à 2 Mo.

Idée :

```text
OS / Memory Source
      ↓ macro-blocs
NUMA pool
      ↓ blocs plus petits
Thread pool
      ↓ malloc utilisateur
Application
```

## 23.5 NUMA pools

Pour éviter de taper directement dans le global pool, l’allocateur MPC introduit des niveaux de pools NUMA.

Objectifs :

- éviter la contention sur une source globale ;
- garder les données proches du bon nœud NUMA ;
- permettre le recyclage de pages dans le même niveau NUMA ;
- mieux gérer les grosses allocations.

## 23.6 Migration des threads

Si un thread migre vers un autre CPU, ses données peuvent rester sur l’ancien nœud NUMA.

Cela crée de mauvais accès mémoire.

Une solution évoquée est de lier davantage :

- l’ordonnanceur de threads ;
- l’allocateur mémoire.

## 23.7 Next touch policy

Le cours évoque une politique de type **next touch**.

Idée :

> La mémoire peut être réorganisée selon les prochains accès, pas seulement selon le premier accès.

C’est utile si les threads migrent ou si les données changent de propriétaire.

## 23.8 Huge segments

Les très grosses allocations peuvent encore passer par l’OS.

Problèmes :

- coût des appels système ;
- coût des défauts de page ;
- contention sur la source mémoire.

Solution MPC :

- la source mémoire devient elle-même un allocateur ;
- elle garde des macro-blocs libres pour les réutiliser ;
- elle peut fusionner des macro-blocs adjacents ;
- elle peut utiliser `mremap` si aucun segment adapté n’est disponible.

Inconvénient :

- consommation mémoire plus élevée ;
- latence si un appel système reste nécessaire.

---

# 24. Page fault cost et page zeroing

## 24.1 Coût d’un défaut de page

Un défaut de page n’est pas gratuit.

Il implique :

- trap vers le noyau ;
- recherche ou allocation d’une frame ;
- mise à jour des structures mémoire ;
- éventuellement lecture disque ;
- remise à zéro de la page ;
- retour au processus.

## 24.2 Page zeroing

Avant de donner une page à un processus, le noyau la remet souvent à zéro.

Pourquoi ?

Pour des raisons de sécurité.

Un processus ne doit pas lire les anciennes données d’un autre processus.

## 24.3 Coût du page zeroing

Le cours indique que, dans certains contextes, une grande partie du temps de défaut de page peut être due à la remise à zéro des pages.

Idée :

```text
Page fault = allocation + protection + initialisation + structures noyau
```

Le zeroing est utile pour la sécurité, mais peut être coûteux en HPC.

## 24.4 Réutilisation de pages sales dans le même processus

Une optimisation étudiée consiste à réutiliser des pages déjà utilisées par le même processus afin d’éviter de les remettre à zéro.

Comme elles restent dans le même processus, le risque de fuite entre processus est réduit.

Mais cela nécessite des modifications noyau et comporte des limites.

---

# 25. Allocation physique des pages et cache

## 25.1 Pourquoi l’emplacement physique compte ?

Même si le programme voit des adresses virtuelles, les caches travaillent avec des effets liés aux adresses physiques.

Une mauvaise distribution des pages physiques peut :

- augmenter les conflits de cache ;
- dégrader la localité ;
- réduire fortement les performances.

## 25.2 Politiques citées

Le cours mentionne :

- Linux : distribution parfois peu contrôlée ;
- Linux Transparent Huge Pages ;
- OpenSolaris : page coloring ;
- FreeBSD : page coloring et superpages.

## 25.3 Page coloring

Le page coloring cherche à choisir les pages physiques pour mieux répartir les données dans le cache.

Objectif : éviter que trop de pages concurrentes tombent dans les mêmes zones du cache.

---

# 26. Unified Memory sur GPU

## 26.1 Problème des architectures hétérogènes

Dans une machine CPU + GPU, on a souvent :

```text
Mémoire CPU
Mémoire GPU 0
Mémoire GPU 1
Mémoire GPU 2
...
```

Traditionnellement, le programmeur devait déplacer explicitement les données.

## 26.2 Gestion explicite classique

Exemple classique CUDA :

```c
void *h_data, *d_data;
h_data = malloc(N);
cudaMalloc(&d_data, N);

cpu_func1(h_data, N);
cudaMemcpy(d_data, h_data, N, cudaMemcpyHostToDevice);
gpu_func<<<...>>>(d_data, N);
cudaMemcpy(h_data, d_data, N, cudaMemcpyDeviceToHost);
cpu_func3(h_data, N);

free(h_data);
cudaFree(d_data);
```

Inconvénient :

- code plus long ;
- erreurs faciles ;
- copies explicites ;
- difficulté avec les structures complexes.

## 26.3 Unified Memory

La mémoire unifiée permet d’utiliser un pointeur unique accessible par le CPU et le GPU.

Exemple :

```c
void *data;
cudaMallocManaged(&data, N);

cpu_func1(data, N);
gpu_func2<<<...>>>(data, N);
cudaDeviceSynchronize();
cpu_func3(data, N);

cudaFree(data);
```

Avantage :

- simplifie la programmation ;
- évite beaucoup de copies explicites ;
- facilite les structures dynamiques ;
- utile pour le prototypage et le debugging.

## 26.4 Deep copy nightmare

Avec des structures comme :

```c
char **data;
```

la gestion explicite est pénible, car il faut copier :

- le tableau de pointeurs ;
- chaque bloc pointé ;
- les pointeurs côté GPU.

Unified Memory évite une grande partie de cette complexité.

## 26.5 Migration à la demande

Quand un processeur accède à une page qui se trouve dans une autre mémoire, un défaut de page peut déclencher une migration.

Exemple :

```text
GPU accède à une page située en mémoire CPU
→ page fault GPU
→ migration CPU vers GPU
→ accès local ensuite
```

## 26.6 Oversubscription

L’oversubscription signifie que le dataset est plus grand que la mémoire physique disponible sur le GPU.

Unified Memory peut déplacer les pages selon les besoins.

Avantage :

```text
Mieux vaut exécuter plus lentement que planter par manque de mémoire.
```

## 26.7 Quand Unified Memory est utile ?

Unified Memory est utile quand :

- on veut prototyper rapidement ;
- on veut simplifier le code ;
- les structures de données sont irrégulières ;
- il est difficile de partitionner le working set ;
- les données sont réutilisées et le coût de migration est amorti ;
- le dataset dépasse la mémoire GPU.

---

# 27. Évolution Kepler, Pascal, Volta

## 27.1 Kepler

Unified Memory apparaît avec CUDA 6.

Limites de Kepler :

- pas de page fault GPU ;
- espace virtuel limité ;
- migration en bloc au lancement du kernel ;
- pas d’oversubscription ;
- pas d’atomiques système.

## 27.2 Pascal

Pascal apporte :

- support des page faults GPU ;
- espace d’adressage virtuel étendu ;
- migration à la demande ;
- oversubscription ;
- atomiques système.

C’est une évolution majeure.

## 27.3 Volta

Volta conserve le modèle de migration à la demande.

Nouvelle fonctionnalité importante : **access counters**.

Les compteurs d’accès permettent au driver d’identifier les pages fréquemment utilisées et de mieux décider quoi migrer.

## 27.4 Volta + Power9 + NVLINK2

Avec NVLINK2, on obtient :

- cohérence de cache CPU-GPU ;
- accès CPU direct à la mémoire GPU ;
- atomiques natives CPU-GPU.

---

# 28. Heuristiques du driver Unified Memory

Le driver Unified Memory fait plusieurs optimisations automatiquement.

## 28.1 Prefetching automatique

Le driver peut migrer proactivement des pages avant qu’elles ne causent trop de défauts.

Attention : ce prefetching driver n’est pas la même chose que `cudaMemPrefetchAsync`.

## 28.2 Anti-thrashing

Si CPU et GPU accèdent fréquemment à la même page, la page peut migrer sans arrêt.

C’est une forme de thrashing.

Le driver essaie de limiter ces migrations répétées.

Sur Pascal, certaines pages peuvent être épinglées côté CPU, ce qui réduit la visibilité sur les accès.

Sur Volta, les access counters aident à mieux placer les pages.

## 28.3 Éviction

Quand la mémoire GPU est pleine, il faut choisir quelles pages sortir.

Le driver maintient une liste de chunks physiques GPU.

Les chunks au début de la liste peuvent être évincés d’abord selon une logique proche de LRU.

---

# 29. Optimisations utilisateur CUDA

Si le programmeur connaît bien son application, il peut donner des indices au driver.

## 29.1 `cudaMemPrefetchAsync`

```c
cudaMemPrefetchAsync(ptr, size, processor, stream);
```

Rôle : demander explicitement la migration des pages vers un processeur.

Exemple :

```c
cudaMemPrefetchAsync(data, N, myGpuId, stream);
mykernel<<<..., stream>>>(data, N);
cudaMemPrefetchAsync(data, N, cudaCpuDeviceId, stream);
```

Objectif : éviter les défauts de page pendant le kernel.

## 29.2 `cudaMemAdvise`

```c
cudaMemAdvise(ptr, size, advice, processor);
```

Rôle : donner un conseil au driver.

Conseils importants :

| Conseil | Idée |
|---|---|
| `SetReadMostly` | données surtout lues |
| `SetPreferredLocation` | emplacement préféré |
| `SetAccessedBy` | processeur susceptible d’accéder aux données |

## 29.3 Read Mostly

Si les données sont surtout lues, le driver peut créer des copies au lieu de déplacer sans cesse la page.

Avantage : CPU et GPU peuvent lire simultanément sans provoquer trop de défauts.

Inconvénient : les écritures deviennent plus coûteuses.

## 29.4 Preferred Location

Permet d’indiquer où la donnée devrait rester de préférence.

Exemple :

```c
cudaMemAdvise(data, N, cudaMemAdviseSetPreferredLocation, cudaCpuDeviceId);
```

Le driver résiste davantage à la migration loin de cet emplacement.

## 29.5 Accessed By

Permet d’indiquer qu’un processeur va accéder aux données.

Sur certains systèmes, cela permet d’établir un mapping direct et d’éviter certains défauts de page.

---

# 30. Applications HPC, Deep Learning et Graph Analytics

## 30.1 HPGMG / AMR

HPGMG est un proxy de calcul scientifique lié aux méthodes multigrilles géométriques.

Dans ce type de code :

- les datasets sont gros ;
- les niveaux AMR sont réutilisés ;
- CPU et GPU peuvent collaborer ;
- la gestion mémoire est critique.

Optimisation mentionnée :

```text
précharger le niveau AMR suivant pendant que le GPU calcule le niveau courant
```

Cela peut se faire avec un stream CUDA non bloquant.

## 30.2 Deep Learning

Dans les réseaux profonds ou les grands batchs, la mémoire GPU peut être insuffisante.

Unified Memory peut permettre d’exécuter des modèles qui dépassent la mémoire GPU, sans réécrire tout le code.

Mais les performances dépendent fortement :

- de la réutilisation des données ;
- de la qualité de la migration ;
- de l’interconnexion CPU-GPU ;
- des hints donnés au driver.

## 30.3 Graph Analytics

Les graphes ont souvent des accès irréguliers.

Unified Memory est intéressant car :

- le working set est difficile à prédire ;
- les structures sont dynamiques ;
- les accès peuvent être partagés entre GPU.

Les atomiques système et NVLINK peuvent améliorer les performances dans certains algorithmes comme BFS.

---

# 31. Résumé ultra-important pour l’examen

## 31.1 Les idées essentielles

1. La mémoire physique est la RAM réelle.
2. Le programme manipule un espace logique, pas directement la RAM.
3. La mémoire virtuelle traduit les pages logiques en frames physiques.
4. La table des pages appartient au contexte du processus.
5. Le TLB accélère les traductions page → frame.
6. Le swap permet de stocker des pages sur disque.
7. Un défaut de page arrive lorsqu’une page demandée n’est pas en RAM.
8. Le bit de validité indique si la page est en mémoire.
9. Le dirty bit indique si la page doit être réécrite sur disque.
10. Les algorithmes de remplacement choisissent une page victime.
11. FIF est optimal mais impraticable.
12. LRU est performant mais coûteux.
13. Clock approxime LRU avec un coût faible.
14. FIFO est simple mais naïf.
15. Le thrashing arrive quand le système passe son temps à swapper.
16. Le working set aide à éviter le thrashing.
17. Les tables hiérarchiques évitent une table des pages énorme.
18. `mmap` projette un fichier en mémoire virtuelle.
19. NUMA rend la localité mémoire critique.
20. First touch place une page sur le nœud du premier thread qui y accède.
21. `malloc` travaille en espace utilisateur mais peut appeler le noyau.
22. Les allocateurs classiques peuvent mal gérer NUMA et multithreading.
23. En HPC, les pools par thread et pools NUMA réduisent la contention.
24. Le page zeroing protège la sécurité mais coûte du temps.
25. Unified Memory simplifie la programmation CPU-GPU.
26. Pascal apporte le page fault GPU et l’oversubscription.
27. Volta ajoute les access counters.
28. Les hints CUDA permettent d’optimiser les migrations.

---

# 32. Questions-réponses possibles

## Q1. C’est quoi la mémoire physique ?

La mémoire physique est la RAM réelle de la machine. Elle est composée de cellules adressables contenant des bits.

## Q2. C’est quoi la mémoire logique ?

La mémoire logique est l’espace d’adressage vu par le programme. Elle donne l’illusion d’une mémoire continue et privée.

## Q3. Pourquoi a-t-on besoin de mémoire virtuelle ?

Pour isoler les processus, permettre à plusieurs programmes d’utiliser les mêmes adresses logiques, charger les pages à la demande et exécuter des programmes plus grands que la mémoire physique.

## Q4. Quelle est la différence entre page et frame ?

Une page est un bloc de mémoire logique. Une frame est un bloc de mémoire physique. La table des pages associe les pages aux frames.

## Q5. C’est quoi la table des pages ?

C’est une structure par processus qui permet de traduire les numéros de pages logiques en numéros de frames physiques.

## Q6. C’est quoi le TLB ?

Le TLB est un cache matériel qui stocke les traductions récentes page → frame pour éviter de consulter la table des pages à chaque accès mémoire.

## Q7. C’est quoi un défaut de page ?

Un défaut de page se produit quand un processus accède à une page qui n’est pas présente en mémoire physique.

## Q8. C’est quoi le swap ?

Le swap est une zone disque utilisée pour stocker temporairement des pages qui ne tiennent pas en mémoire physique.

## Q9. À quoi sert le bit de validité ?

Il indique si une page est actuellement en mémoire. Si le bit vaut 0 et que la page est accédée, un défaut de page est déclenché.

## Q10. À quoi sert le dirty bit ?

Il indique si une page a été modifiée. Si elle est modifiée, elle doit être réécrite sur disque avant d’être remplacée.

## Q11. Quel est le but des algorithmes de remplacement ?

Choisir la page victime à retirer de la mémoire quand aucune frame libre n’est disponible, en minimisant les défauts de page.

## Q12. Pourquoi FIF est optimal mais non utilisé ?

Parce qu’il remplace la page qui sera utilisée le plus tard dans le futur, mais connaître le futur est impossible en pratique.

## Q13. Pourquoi LRU est intéressant ?

LRU remplace la page la moins récemment utilisée. Il se base sur l’idée qu’une page peu utilisée récemment a moins de chance d’être utilisée bientôt.

## Q14. Pourquoi Clock est utilisé ?

Clock approxime LRU avec un coût d’implémentation plus faible grâce à un bit de référence et une liste circulaire.

## Q15. C’est quoi le thrashing ?

C’est une situation où le système passe plus de temps à charger et décharger des pages qu’à exécuter réellement les programmes.

## Q16. C’est quoi le working set ?

C’est l’ensemble des pages utilisées récemment par un processus. Il permet d’estimer combien de frames le processus a besoin.

## Q17. Pourquoi utilise-t-on des tables des pages hiérarchiques ?

Parce qu’une table des pages plate serait trop grande en 64 bits. Les tables hiérarchiques ne créent que les niveaux nécessaires.

## Q18. À quoi sert `mmap` ?

`mmap` permet de projeter un fichier ou une zone mémoire dans l’espace d’adressage d’un processus pour accéder au fichier comme à de la mémoire.

## Q19. C’est quoi NUMA ?

NUMA est une architecture où l’accès mémoire n’a pas le même coût selon la proximité entre CPU et mémoire.

## Q20. C’est quoi first touch ?

First touch est une politique où une page physique est allouée sur le nœud NUMA du thread qui y accède pour la première fois.

## Q21. Pourquoi `malloc` peut être lent en multithread ?

Parce que plusieurs threads peuvent se bloquer sur des verrous internes, provoquer de la contention, du false sharing ou une mauvaise localité NUMA.

## Q22. Pourquoi l’allocation mémoire est critique en HPC ?

Parce que les applications HPC utilisent beaucoup de threads, de mémoire et de données. Une mauvaise localité mémoire peut réduire fortement les performances.

## Q23. C’est quoi Unified Memory ?

Unified Memory est un mécanisme CUDA qui permet d’utiliser un pointeur unique accessible par CPU et GPU, avec migration automatique des pages.

## Q24. Pourquoi Unified Memory simplifie la programmation GPU ?

Parce qu’il évite de gérer explicitement les copies CPU ↔ GPU avec `cudaMemcpy`, surtout pour les structures complexes.

## Q25. Quelle différence entre Kepler, Pascal et Volta pour Unified Memory ?

Kepler fait surtout des migrations en bloc. Pascal ajoute les page faults GPU, la migration à la demande et l’oversubscription. Volta ajoute les access counters pour mieux décider quelles pages migrer.

## Q26. À quoi sert `cudaMemPrefetchAsync` ?

Il permet de migrer des pages à l’avance vers le CPU ou le GPU pour éviter des défauts de page pendant l’exécution.

## Q27. À quoi sert `cudaMemAdvise` ?

Il donne des indications au driver, par exemple que les données sont surtout lues, qu’elles ont une localisation préférée ou qu’un processeur va y accéder.

---

# Fiche très courte à mémoriser

```text
Mémoire physique = RAM réelle.
Mémoire logique = espace vu par le programme.
Page = bloc logique.
Frame = bloc physique.
Table des pages = page → frame.
TLB = cache des traductions.
Swap = pages stockées sur disque.
Page fault = page absente de la RAM.
Valid bit = page présente ou non.
Dirty bit = page modifiée ou non.
FIFO = remplace la plus ancienne.
FIF = optimal théorique.
LRU = remplace la moins récemment utilisée.
Clock = approximation efficace de LRU.
Thrashing = trop de swap, peu de calcul utile.
Working set = pages récemment utilisées.
mmap = fichier projeté en mémoire.
NUMA = accès mémoire non uniforme.
First touch = page placée près du premier thread qui l’utilise.
HPC = localité + multithreading + NUMA très importants.
Unified Memory = pointeur unique CPU/GPU + migration de pages.
```

---

# Schéma mental final

```text
Programme C
   |
   | malloc / free / mmap
   v
Allocateur utilisateur
   |
   | brk / mmap
   v
Noyau Linux
   |
   | tables des pages, frames, swap, page faults
   v
Mémoire physique RAM
   |
   | NUMA, cache, huge pages
   v
Performance réelle
```

Et pour GPU :

```text
CPU + GPU
   |
   | cudaMallocManaged
   v
Unified Memory
   |
   | page fault + migration + prefetch + hints
   v
Mémoire CPU / mémoire GPU
```

---

## Conclusion

Ce cours montre que la mémoire n’est pas seulement un espace de stockage. C’est un élément central de la performance, de la sécurité et de la stabilité du système.

Pour bien comprendre l’allocation mémoire, il faut relier quatre niveaux :

1. **niveau programmeur** : variables, pile, tas, malloc/free ;
2. **niveau système** : mémoire virtuelle, pages, frames, swap, TLB ;
3. **niveau performance** : fragmentation, page faults, NUMA, cache ;
4. **niveau HPC/GPU** : allocateurs spécialisés, pools NUMA, Unified Memory, migrations CPU-GPU.

Le message final du cours est clair :

> Dans les systèmes modernes, la performance mémoire dépend autant de l’endroit où les données sont placées que de la quantité de mémoire disponible.