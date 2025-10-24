#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char **argv) 
{
    MPI_Init(&argc, &argv);

    int ndims = 2;
    int mesh_size[2] = {100,100};
    int nproc_per_dim[2] = {0, 0};
    int periods_per_dim[2] = {0, 0};
    int rang,nproc;

    MPI_Comm_rank(MPI_COMM_WORLD, &(rang));
    MPI_Comm_size(MPI_COMM_WORLD, &(nproc));

    int err = 0;


/*
    // DEBUT Zone-4-1-A
    // Créer la topologie cartesienne adaptée au nombre de processus à l'aide de MPI_Dims_create et MPI_Cart_create

    if (rang == 0)
    {
        printf("\n dims : %d, nbproc X : %d, nbproc Y : %d, nb_procs_tot (objectif) : %d (%d) \n", \
                ndims,nproc_per_dim[0],nproc_per_dim[1],nproc_per_dim[0]*nproc_per_dim[1],nproc);
        fflush(stdout);
    }
    // FIN Zone-4-1-A
*/

/*
    // DEBUT Zone-4-1-B
    // Une barriere MPI ici permettrait de sérarer les affichages

    int proc_coords[2];
    // Récupérer les coordonnées du rang dans la topologie

    int Q[2], R[2], nloc[0], ideb[0], ifin[0];

    int idim = 0;

    for (idim = 0; idim < ndims; idim++)
    {
    
        Q[idim] = mesh_size[idim] / nproc_per_dim[idim];
        R[idim] = mesh_size[idim] % nproc_per_dim[idim];

        if (proc_coords[idim] < R[idim]) 
        {
            nloc[idim] = Q[idim]+1;
            ideb[idim] = proc_coords[idim] * (Q[idim]+1);
            ifin[idim] = ideb[idim] + nloc[idim];
        } 
        else 
        {
            nloc[idim] = Q[idim];
            ideb[idim] = R[idim] * (Q[idim]+1) + (proc_coords[idim] - R[idim]) * Q[idim];
            ifin[idim] = ideb[idim] + nloc[idim];
        }

    }

    int ntot_per_proc = nloc[0] * nloc[1];

    printf("%d %d %d %d\n", rang, proc_coords[0], proc_coords[1], ntot_per_proc);
    fflush(stdout);

    // Une barriere MPI ici permettrait de sérarer les affichages
    
    int nglob = 0;
    // Faire une réduction pour que le processus 0 récupère le nombre total de mailles

    if (rang == 0)
    {
        printf("\n Nombre total de maille (objectif) : %d (%d) \n",nglob,mesh_size[0]*mesh_size[1]);
        fflush(stdout);
    }
    // FIN Zone-4-1-B
*/

/*
    // DEBUT Zone-4-2-A
    // Faire un allgather pour récupérer le nombre de mailles dans tous les autres les autres processus

    if (rang == 0)
    {
        int ir;
        for (ir = 0; ir < nproc; ir++)
        { 
            printf("( %d ; %d )\n", ir, nb_cells_by_proc[ir]);
        }
    }
    // FIN Zone-4-2-A
*/

/*
    // DEBUT Zone-4-2-B
    // Faire un allgather pour récupérer le nombre de mailles des processus voisins
    printf("RANG %d : ( W %d ; E %d ; N %d ; S %d)\n", \
            rang, nb_cells_in_neighbor[0], nb_cells_in_neighbor[1], \
            nb_cells_in_neighbor[2], nb_cells_in_neighbor[3]);
    // FIN Zone-4-2-B
*/
    

    MPI_Finalize();

    return 0;
}

