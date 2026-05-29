# Notes de cours — Architecture OS

Ce module couvre l'architecture des systèmes d'exploitation modernes : organisation de la mémoire, appels système, ordonnancement, et sécurité système.

---

## Table des matières

1. [Organisation de la mémoire](#1-organisation-de-la-mémoire)
2. [Gestion de la mémoire virtuelle](#2-gestion-de-la-mémoire-virtuelle)
3. [Allocateurs de tas](#3-allocateurs-de-tas)
4. [Ordonnancement (noyau Linux)](#4-ordonnancement-noyau-linux)
5. [Synchronisation](#5-synchronisation)
6. [Sécurité système](#6-sécurité-système)

---

## 1. Organisation de la mémoire

Un processus Linux dispose d'un espace d'adressage virtuel divisé en segments :

```
Haute adresse
┌─────────────────┐
│   Pile (stack)  │  ↓ croît vers les basses adresses
├─────────────────┤
│   (espace libre)│
├─────────────────┤
│   Tas (heap)    │  ↑ croît vers les hautes adresses
├─────────────────┤
│   BSS (non init)│
├─────────────────┤
│   Data (init)   │
├─────────────────┤
│   Text (code)   │
└─────────────────┘
Basse adresse
```

- **Text** : code machine exécutable (lecture seule)
- **Data** : variables globales initialisées
- **BSS** : variables globales non initialisées (mises à zéro au démarrage)
- **Heap** : allocation dynamique (`malloc`/`free`)
- **Stack** : variables locales, paramètres, adresses de retour

---

## 2. Gestion de la mémoire virtuelle

- La **MMU** (Memory Management Unit) traduit les adresses virtuelles en adresses physiques via les **tables de pages**.
- Un **défaut de page** (page fault) survient quand une page n'est pas présente en RAM → le noyau la charge depuis le disque (swap) ou depuis le binaire.
- `mmap()` permet de mapper des fichiers ou de la mémoire anonyme dans l'espace d'adressage.

Primitives utiles :
```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t len, int prot);
```

---

## 3. Allocateurs de tas

### Fonctionnement de `malloc` (glibc — ptmalloc2)

- Utilise `brk()`/`sbrk()` pour les petites allocations (extension du segment heap).
- Utilise `mmap()` pour les grandes allocations (≥ 128 Ko).
- Gère des **chunks** avec un en-tête contenant la taille et des bits de contrôle.
- **Listes libres** (bins) : tri par taille pour recycler rapidement les blocs libérés.

### Politiques d'allocation

| Politique | Description | Complexité |
|---|---|---|
| First-fit | Premier bloc suffisamment grand | O(n) |
| Best-fit | Bloc libre le plus petit suffisant | O(n) |
| Next-fit | First-fit depuis le dernier emplacement | O(n) amortit |
| Buddy system | Blocs de taille puissance de 2 | O(log n) |

### TP — `hp_allocator`

Implémentation d'un allocateur personnalisé avec :
- Gestion d'un bloc de mémoire brut (`mmap` ou tableau statique)
- Liste chaînée de blocs libres
- Wrapper `LD_PRELOAD` pour remplacer `malloc`/`free` système

Voir [tps/tp_mémoire/](tps/tp_mémoire/) pour les sources et benchmarks.

---

## 4. Ordonnancement (noyau Linux)

### CFS — Completely Fair Scheduler

- Algorithme par défaut depuis Linux 2.6.23.
- Utilise un **arbre rouge-noir** trié par `vruntime` (temps CPU virtuel).
- Le processus avec le plus petit `vruntime` est sélectionné en premier.
- Chaque processus a un **poids** lié à sa priorité (`nice`).

### Primitives du noyau — `completion`

`completion` est une primitive de synchronisation noyau (plus simple qu'un semaphore) :

```c
struct completion c;
init_completion(&c);

/* Producteur (signale) */
complete(&c);

/* Consommateur (attend) */
wait_for_completion(&c);
```

Voir [kernel/sched/completion.c](kernel/sched/completion.c) pour l'implémentation.

---

## 5. Synchronisation

| Primitive | Contexte | Usage |
|---|---|---|
| `mutex` | Processus/threads | Exclusion mutuelle |
| `spinlock` | Noyau (section critique courte) | Attente active |
| `semaphore` | Processus | Comptage de ressources |
| `completion` | Noyau | Signalisation one-shot |
| `rwlock` | Noyau | Lecture simultanée, écriture exclusive |

---

## 6. Sécurité système

Voir [kernel/sécurité informatique/notes.md](kernel/sécurité%20informatique/notes.md).

Thèmes principaux :
- Contrôle d'accès : DAC (Unix permissions), MAC (SELinux, AppArmor)
- Espaces de noms (`namespaces`) et conteneurs
- Capabilities Linux (remplacement du tout-ou-rien root/non-root)
- Appels système sensibles et filtrage (`seccomp`)
- Attaques mémoire : buffer overflow, heap spray, ROP — et contre-mesures (ASLR, NX, stack canaries, PIE)
