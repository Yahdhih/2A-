# Programmation Parallèle — MPI

Notes de cours, travaux dirigés et exercices pratiques sur la programmation parallèle avec l'interface MPI (Message Passing Interface).

## Contenu

```
Programmation_parallele_MPI/
├── exercices/                  # Exercices MPI codés en C
├── td1_mpi_collective/         # TD sur les communications collectives
├── PRPA-Programmation_Parallele/  # Archives des cours et TDs (2018-2025)
│   ├── Cours/                  # Cours par année académique
│   ├── TD/                     # Sujets de TD par année
│   └── Partiel/                # Sujets de partiels (2018-2025)
├── notes.md                    # Notes de cours
├── notes1.md                   # Suite des notes
└── notes1_1.md                 # Approfondissements
```

## Concepts clés

- **Communications point à point** : `MPI_Send`, `MPI_Recv`, `MPI_Isend`, `MPI_Irecv`
- **Communications collectives** : `MPI_Bcast`, `MPI_Scatter`, `MPI_Gather`, `MPI_Reduce`, `MPI_Allreduce`
- **Scan et préfixe** : `MPI_Scan`, `MPI_Exscan`
- **Types de données dérivés** : `MPI_Type_vector`, `MPI_Type_struct`
- **Topologies** : `MPI_Cart_create`, `MPI_Cart_shift`
- **Communications non bloquantes** : `MPI_Wait`, `MPI_Test`

## Exercices

Voir [exercices/README.md](exercices/README.md) pour le détail des exercices disponibles.

### Compilation et exécution rapide

```bash
# Compiler un exercice MPI
mpicc -o scan_som scan_som.c

# Exécuter sur 4 processus
mpirun -np 4 ./scan_som
```

## Archives des cours (PRPA)

Le répertoire [PRPA-Programmation_Parallele/](PRPA-Programmation_Parallele/) contient les ressources pédagogiques des années 2018 à 2025 :
- Supports de cours
- Sujets de TD avec corrections
- Sujets de partiels
