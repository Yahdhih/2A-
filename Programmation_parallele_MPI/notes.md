# Notes de cours — Programmation Parallèle MPI

Ce module introduit la programmation parallèle par passage de messages avec MPI (Message Passing Interface), standard pour les clusters HPC.

---

## Table des matières

1. [Modèle de programmation MPI](#1-modèle-de-programmation-mpi)
2. [Communications point à point](#2-communications-point-à-point)
3. [Communications collectives](#3-communications-collectives)
4. [Communications non bloquantes](#4-communications-non-bloquantes)
5. [Types de données dérivés](#5-types-de-données-dérivés)
6. [Topologies cartésiennes](#6-topologies-cartésiennes)
7. [Conseils de performance](#7-conseils-de-performance)

---

## 1. Modèle de programmation MPI

MPI suit le modèle **SPMD** (Single Program, Multiple Data) : tous les processus exécutent le même programme, mais opèrent sur des données différentes.

```c
#include <mpi.h>

int main(int argc, char **argv) {
    int rang, taille;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);   /* identifiant du processus */
    MPI_Comm_size(MPI_COMM_WORLD, &taille); /* nombre total de processus */

    /* ... travail parallèle ... */

    MPI_Finalize();
    return 0;
}
```

Compilation et exécution :
```bash
mpicc -o prog prog.c
mpirun -np 4 ./prog
```

---

## 2. Communications point à point

| Fonction | Description |
|---|---|
| `MPI_Send` | Envoi bloquant |
| `MPI_Recv` | Réception bloquante |
| `MPI_Sendrecv` | Envoi + réception simultanés (évite les deadlocks) |

```c
/* Envoi */
MPI_Send(&data, count, MPI_INT, dest, tag, MPI_COMM_WORLD);

/* Réception */
MPI_Status status;
MPI_Recv(&data, count, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
```

**Attention aux deadlocks** : si tous les processus font `MPI_Send` avant `MPI_Recv`, le programme se bloque. Utiliser `MPI_Sendrecv` ou les communications non bloquantes.

---

## 3. Communications collectives

Toutes les fonctions collectives sont **bloquantes** et impliquent **tous** les processus du communicateur.

### Diffusion — `MPI_Bcast`
```c
MPI_Bcast(&data, count, MPI_INT, root, MPI_COMM_WORLD);
```

### Réduction — `MPI_Reduce` / `MPI_Allreduce`
```c
/* Résultat sur root seulement */
MPI_Reduce(&local, &global, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);

/* Résultat sur tous les processus */
MPI_Allreduce(&local, &global, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
```

Opérations disponibles : `MPI_SUM`, `MPI_PROD`, `MPI_MAX`, `MPI_MIN`, `MPI_LAND`, `MPI_LOR`, etc.

### Distribution / Collecte — `MPI_Scatter` / `MPI_Gather`
```c
MPI_Scatter(sendbuf, sendcount, MPI_INT,
            recvbuf, recvcount, MPI_INT, root, MPI_COMM_WORLD);

MPI_Gather(sendbuf, sendcount, MPI_INT,
           recvbuf, recvcount, MPI_INT, root, MPI_COMM_WORLD);
```

### Scan préfixe — `MPI_Scan` / `MPI_Exscan`
```c
MPI_Scan(&local, &result, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
/* Processus i reçoit sum(rang 0..i) */
```

### Barrière de synchronisation
```c
MPI_Barrier(MPI_COMM_WORLD);
```

---

## 4. Communications non bloquantes

Permettent de recouvrir calcul et communication.

```c
MPI_Request req;
MPI_Isend(&data, count, MPI_INT, dest, tag, MPI_COMM_WORLD, &req);
/* ... calcul en parallèle ... */
MPI_Wait(&req, MPI_STATUS_IGNORE);   /* attendre la fin de l'envoi */
```

Fonctions non bloquantes : `MPI_Isend`, `MPI_Irecv`, `MPI_Ibcast`, `MPI_Ireduce`, etc.

Test sans blocage :
```c
int flag;
MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
```

---

## 5. Types de données dérivés

MPI permet de définir des types non contigus pour éviter les copies.

```c
MPI_Datatype coltype;
/* Vecteur : count blocs de blocklength éléments, séparés par stride */
MPI_Type_vector(count, blocklength, stride, MPI_DOUBLE, &coltype);
MPI_Type_commit(&coltype);

MPI_Send(matrix, 1, coltype, dest, tag, MPI_COMM_WORLD);

MPI_Type_free(&coltype);
```

---

## 6. Topologies cartésiennes

Utile pour les calculs sur grilles (stencils, automates cellulaires).

```c
int dims[2] = {4, 4};     /* grille 4×4 */
int periods[2] = {0, 0};  /* pas de frontières périodiques */
MPI_Comm cart_comm;

MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);

int coords[2];
MPI_Cart_coords(cart_comm, rang, 2, coords);

int left, right;
MPI_Cart_shift(cart_comm, 1, 1, &left, &right); /* voisins en dimension 1 */
```

---

## 7. Conseils de performance

- Préférer `MPI_Allreduce` à `MPI_Reduce` + `MPI_Bcast` (plus efficace sur les implémentations modernes).
- Minimiser le nombre de messages (agréger les données avant d'envoyer).
- Recouvrir les communications avec du calcul grâce aux appels non bloquants.
- Utiliser des types dérivés pour éviter les copies mémoire intermédiaires.
- Profiler avec des outils comme **mpiP**, **Tau**, ou **Score-P**.

---

## Ressources complémentaires

- Notes détaillées : [notes1.md](notes1.md), [notes1_1.md](notes1_1.md)
- Exercices : [exercices/README.md](exercices/README.md)
- Archives de cours : [PRPA-Programmation_Parallele/](PRPA-Programmation_Parallele/)
