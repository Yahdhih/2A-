# Exercices MPI

Exercices pratiques de programmation parallèle avec MPI, écrits en C.

## Fichiers

| Fichier | Description |
|---|---|
| [scan_som.c](scan_som.c) | Scan préfixe (somme inclusive) implémenté manuellement avec `MPI_Send`/`MPI_Recv` |
| [reduction_disrtibution_som.c](reduction_disrtibution_som.c) | Réduction globale (somme) puis redistribution du résultat à tous les processus |

## Description des exercices

### `scan_som.c` — Scan préfixe manuel

Implémente `MPI_Scan` (somme préfixe inclusive) **sans utiliser** la primitive MPI correspondante.  
Chaque processus `i` reçoit la somme des valeurs des processus `0` à `i`.

Algorithme utilisé : chaîne de messages séquentiels (processus `i` reçoit depuis `i-1`, ajoute sa valeur, envoie à `i+1`).

```bash
mpicc -o scan_som scan_som.c
mpirun -np 4 ./scan_som
# Sortie attendue :
# P0, resultat scan = 0
# P1, resultat scan = 1
# P2, resultat scan = 3
# P3, resultat scan = 6
```

### `reduction_disrtibution_som.c` — Réduction + distribution

Calcule la somme globale de toutes les valeurs locales (`MPI_Reduce`) puis redistribue le résultat à tous les processus (`MPI_Bcast` ou `MPI_Allreduce`).

```bash
mpicc -o reduction reduction_disrtibution_som.c
mpirun -np 4 ./reduction
```

## Prérequis

- OpenMPI ou MPICH installé
- Compilateur C (`gcc` ou `cc`)

```bash
# Installation OpenMPI (Debian/Ubuntu)
sudo apt install openmpi-bin libopenmpi-dev

# Installation OpenMPI (macOS via Homebrew)
brew install open-mpi
```
