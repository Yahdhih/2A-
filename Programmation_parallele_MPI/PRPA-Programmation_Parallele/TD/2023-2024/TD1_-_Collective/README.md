# Prise en main

## Se connecter sur le cluster
```
prompt> ssh -Y hpc.pedago.ensiie.fr -l prenom.nom
```

## `module` : préparation environnement
```
hpc01> module load mpi/mpich-3.2-x86_64
```

## `mpicc` : compilation 
`mpicc` se comporte comme un compilateur classique
```
hpc01> mpicc monprog.c -o monprog.exe
```

## `sinfo` : connaître l'ensemble des nœuds disponibles
```
hpc01> sinfo
PARTITION AVAIL  TIMELIMIT  NODES  STATE NODELIST
inter        up   infinite      2  drain hpc[02-03]
calcul*      up   infinite      1  drain hpc13
calcul*      up   infinite      9   idle hpc[04-12]
```

## `squeue` : vérifier l’allocation des ressources (nœuds en cours d’utilisation)
```
hpc01> squeue
      JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)
      12882    calcul     bash  dureaud  R       0:04      2 hpc[04-05]
```

## Exécuter le programme MPI en contrôlant l’allocation de la ressource 
Ici 4 cœurs (`-n 4`) répartis sur 2 nœuds (`-N 2`) :
```
hpc01> srun -n 4 -N 2  ./monprog.exe
hpc01> srun -n 2      ./monprog.exe
```

