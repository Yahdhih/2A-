#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

struct complex_s
{
    double real;
    double imag;
};
typedef struct complex_s complex_t;

/* Permet de tester notre programme
   Affecte des valeurs différentes en fonction du rang
 */
void init_tab_compl(complex_t *tab, int n)
{
    int rang, i, deb;

    MPI_Comm_rank(MPI_COMM_WORLD, &rang);

    deb = rang+10;
    for(i = 0 ; i < n ; i++)
    {
	tab[i].real = cos(deb   + ((double)(i+1))/(double)n);
	tab[i].imag = sin(deb+1 + ((double)(i+1))/(double)n);
    }
}

/* Calcule la norme d'un nombre complexe
 */
double norm_compl_ri(double real, double imag)
{
    return sqrt(real*real + imag*imag);
}

double norm_compl(complex_t *cpl)
{
    return norm_compl_ri(cpl->real, cpl->imag);
}

void output_tab(complex_t *tab, int n)
{
    int rang, i;

    MPI_Comm_rank(MPI_COMM_WORLD, &rang);

    char nom_pts[64];
    sprintf(nom_pts, "pts_P%05d", rang);
    FILE *fd = fopen(nom_pts, "w");

    for(i = 0 ; i < n ; i++)
    {
	fprintf(fd, "%.6e %.6e\n", tab[i].real, tab[i].imag);
    }

    fclose(fd);
}

/* Calcule la norme max parmis les nombres complexes du tableau distribuée
 */
void calc_norm_max(
	complex_t *tab, int n, 
	MPI_Op op_norm_max,
        MPI_Datatype mpi_cpl,	
	complex_t *cpl_max)
{
    /*
       A ECRIRE
     */
}


int main(int argc, char **argv)
{
    int rang;
    int n; /* nb d'éléments par processus */

    MPI_Op op_norm_max;
    MPI_Datatype mpi_cpl;
    complex_t cpl_max;
    complex_t *tab;

    FILE *fd;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);

    if (argc == 2)
    {
	n = atoi(argv[1]);
    }
    else
    {
	printf("Il faut exactement 1 argument : le nb d'éléments par processus\n");
	abort();
    }

    /* Allocation + remplissage tableau
       */
    tab = (complex_t*)malloc(n*sizeof(complex_t));

    init_tab_compl(tab, n);

    output_tab(tab, n);


    /* A COMPLETER => creer mpi_cpl */
    /* A COMPLETER => creer op_norm_max */


    calc_norm_max(tab, n, op_norm_max, mpi_cpl, &cpl_max);
    printf("P%d, norm = %.6e\n", rang, norm_compl(&cpl_max));

    fd = fopen("pts_max", "w");
    fprintf(fd, "%.6e %.6e\n", cpl_max.real, cpl_max.imag);
    fclose(fd);

    /* A COMPLETER => liberation operateur, type derivé */

    free(tab);

    MPI_Finalize();

    return 0;
}

