# Exercice 1

## Question 1

```
$ mpirun -n 4 ./main
Rank: 3 / 4 -- Proc: hpc06.c-hpc.pedago.ensiie.fr -- PID: 71409
Rank: 2 / 4 -- Proc: hpc05 -- PID: 132540
Rank: 0 / 4 -- Proc: hpc05 -- PID: 132538
Rank: 1 / 4 -- Proc: hpc05 -- PID: 132539
```

Ce ne sont pas les mêmes PID. En effet, chaque processus est différent (ils peuvent même être sur un noeud différent comme montré ci-dessus pour le processus 3), et se voit donc attribuer un processus différent.

## Question 2

```
$ mpirun -n 4 ./main
Rank: 3 / 4 -- Proc: hpc06.c-hpc.pedago.ensiie.fr -- PID: 72346 -- testVar adress: 0x7ffca5dd5728
Rank: 0 / 4 -- Proc: hpc05 -- PID: 133561 -- testVar adress: 0x7ffebaa4c338
Rank: 1 / 4 -- Proc: hpc05 -- PID: 133562 -- testVar adress: 0x7fff154be4c8
Rank: 2 / 4 -- Proc: hpc05 -- PID: 133563 -- testVar adress: 0x7ffc85896ba8
```

Les adresses sont différentes: chaque processus MPI possède sa propre copie de la variable, indépendante des autres processus. Des appels à `MPI_Send` et `MPI_Recv` doivent être effectués pour les mettre en relation si c'est souhaité.

## Question 3

```
$ mpirun -n 4 ./main
Avant MPI
Avant MPI
Avant MPI
Avant MPI
Rank: 3 / 4 -- Proc: hpc06.c-hpc.pedago.ensiie.fr -- PID: 73360 -- testVar adress: 0x7ffe812e3258
Rank: 0 / 4 -- Proc: hpc05 -- PID: 134847 -- testVar adress: 0x7ffe3f840c88
Rank: 1 / 4 -- Proc: hpc05 -- PID: 134848 -- testVar adress: 0x7fffa873eaf8
Rank: 2 / 4 -- Proc: hpc05 -- PID: 134849 -- testVar adress: 0x7ffc8ce50ac8
```

"Avant MPI" est affiché 4 fois: une fois à chaque lancement de processus.

# Exercice 2

## Question 1

```
$ mpirun -n 2 ./ex2q1
Proc 1 received val = 10
```

## Question 2

```
mpicc -std=c11 ex2q2.c -o main && mpirun -n 2 ./main && rm main
1 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 3.14
```

## Question 3:

```
$ mpicc -std=c11 ex2q3.c -o main && mpirun -n 2 ./main && rm main
Proc 1 received val = 10
Proc 0: 1 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 3.14
```

# Exercice 3

## Piège A

Plusieurs `MPI_Recv` pour un seul `MPI_Send`: faux.

```c
// Erreur
MPI_Recv(&recue, 1, MPI_INT, /*src=*/0,  /*tag=*/1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
// Correction
MPI_Recv(&recue, n, MPI_INT, /*src=*/0,  /*tag=*/1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

## Piège B

On ne peut pas recevoir de soit même.

```c
// Erreur
MPI_Recv(&c, 1, MPI_CHAR, 1, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
// Correction
MPI_Recv(&c, 1, MPI_CHAR, 0, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

## Piège C

Les tags d'envoi/réception doivent correspondre.

```c
// Erreur
MPI_Recv(&c, 1, MPI_CHAR, 0, 2000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
// Correction
MPI_Recv(&c, 1, MPI_CHAR, 0, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

## Piège D

Les types d'envoi/réception doivent correspondre.

```c
// Erreur
double r;
MPI_Recv(&r, 1, MPI_DOUBLE, 0, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
printf("r = %g\n", r);
// Correction
int r;
MPI_Recv(&r, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
printf("r = %d\n", r);
```

# Exercice 4

## Question 1

Si le `MPI_Send` est synchrone, cela va bloquer indéfiniment. S'il est asynchrone, le programme se comportera comme prévu.

## Question 2

Il bloque à `n = 4041`.

## Question 3

- .a : L'envoi écrivant dans `buf_recv`, quand ce buffer sera envoyé il contiendra les données de `buf_send`: il n'y aura pas d'échange de données. On débloque cela via un buffer temporaire si on le souhaite.
- .b : l'envoi devient bloquant quand la taille des buffers à échanger devient plus grande que la taille du buffer donné à MPI pour faire les échanges. On le débloque en s'assurant que ces tailles sont toujours en correspondance.

# Exercice 5

## Question 1

On utilise la méthode `MPI_Scan` (réduction partielle) pour faire une somme de 1 depuis le nombre 0.
Cela amène chaque `me` à la valeur du processus indexée depuis 1. On fait ensuite un `MPI_Allreduce` avec un `MPI_MAX` pour obtenir le `me` maximal et donc `P`, nombre de processus.

Voilà, voilà.

## Question 2

Oups.

## Question 3
