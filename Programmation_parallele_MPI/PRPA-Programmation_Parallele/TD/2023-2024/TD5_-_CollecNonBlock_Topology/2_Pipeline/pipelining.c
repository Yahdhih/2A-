#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>


void fill_tabs(float *a, float *b, float *c, int nloc)  
{
    int tictac = 1;

    float deuxtiers=2.0f/3.0f;
    float quatretiers=4.0f/3.0f;
    float inv_randmax=1.0f/RAND_MAX;

    // Remplissage des trois tableaux, de trois manieres differentes 
    // mais sensees donner le meme resultat final.
    int i;
    for( i = 0 ; i < nloc ; i++ )
    {
         a[i] = 1.0f;
         b[i] = 2.0f * rand() * inv_randmax;
         c[i] = (tictac ? deuxtiers : quatretiers);
         tictac=!tictac;
    }
}

void reduce_nocomm(int N, float *a, float *b, float *c) 
{
    float res_loc[3] = {0.f,0.f,0.f};
    int i;

    for( i = 0 ; i < N ; i++ )
    {    
	    res_loc[0] += a[i];
	    res_loc[1] += b[i];
	    res_loc[2] += c[i];
    }
}

/* l'exercice commence ici */
void allreduce_blocking(int N, float *a, float *b, float *c, float *res_glob) 
{
    float res_loc[3] = {0.f,0.f,0.f};
    int i;

    for( i = 0 ; i < N ; i++ )
    {    
	    res_loc[0] += a[i];
	    res_loc[1] += b[i];
	    res_loc[2] += c[i];
    }

    // Zone-2-1-A : Reduction sur les 3 grandeurs directement

   // Fin Zone-2-1-A
}


void allreduce_blocking_v2(int N, float *a, float *b, float *c, float *res_glob) 
{
    float res_loc[3] = {0.f,0.f,0.f};
    int i;

    for( i = 0 ; i < N ; i++ )
    {    
	    res_loc[0] += a[i];
	    res_loc[1] += b[i];
	    res_loc[2] += c[i];
    }

    // Zone-2-1-B : Reduction sur les 3 grandeurs séparement 
    
    // Fin Zone-2-1-B
}



void allreduce_nonblocking(int N, float *a, float *b, float *c, float *res_glob) 
{
    float res_loc[3] = {0.f,0.f,0.f};
    int i;

    for( i = 0 ; i < N ; i++ )
    {    
	    res_loc[0] += a[i];
	    res_loc[1] += b[i];
	    res_loc[2] += c[i];
    }

    // Zone-2-1-C : Reduction non-bloquante sur les 3 grandeurs directement 

    // Fin Zone-2-1-C
}


void allreduce_nonblocking_v2(int N, float *a, float *b, float *c, float *res_glob) 
{
    float res_loc[3] = {0.f,0.f,0.f};
    int i;
    
    for( i = 0 ; i < N ; i++ )
    {    
	    res_loc[0] += a[i];
	    res_loc[1] += b[i];
	    res_loc[2] += c[i];
    }
    
    // Zone-2-1-D : Reduction non-bloquante sur les 3 grandeurs séparement
    
    // Fin Zone-2-1-D

}

int main(int argc, char **argv) 
{
    // DEBUT DE LA PHASE PARALLELE
    MPI_Init(&argc, &argv);
    
    int Nglob, Nloc;
    float *a, *b, *c;
    float res1[3], res2[3], res3[3], res4[3], err;

    // RECUPERATION DU RANG
    int rang;
    MPI_Comm_rank(MPI_COMM_WORLD, &(rang));

    // RECUPERATION DU NOMBRE DE PROCESSUS MPI
    int nproc;
    MPI_Comm_size(MPI_COMM_WORLD, &(nproc));

    Nloc = atoi(argv[1]);;
    Nglob = nproc * Nloc;

    a = (float*)malloc(Nloc*sizeof(float));
    b = (float*)malloc(Nloc*sizeof(float));
    c = (float*)malloc(Nloc*sizeof(float));

    fill_tabs(a, b, c, Nloc);

    //MPI_Barrier(MPI_COMM_WORLD);

    int i=0;
    int itermax=1000;

    double t01=0.;
    double t02=0.;
    double time0=0.;
    for (i=0; i<itermax;i++)
    {
        t01 = MPI_Wtime();
        reduce_nocomm(Nloc, a, b, c);
        t02 = MPI_Wtime();
        time0 += (t02 - t01);
    }

    double t11=0.;
    double t12=0.;
    double time1=0.;
    for (i=0; i<itermax;i++)
    {
        t11 = MPI_Wtime();
        allreduce_blocking(Nloc, a, b, c, res1);
        t12 = MPI_Wtime();
        time1 += (t12 - t11);
    }
    
    double t21=0.;
    double t22=0.;
    double time2=0.;
    for (i=0; i<itermax;i++)
    {
        t21 = MPI_Wtime();
        allreduce_blocking_v2(Nloc, a, b, c,res2);
        t22 = MPI_Wtime();
        time2 += (t22 - t21);
    }

    double t31=0.;
    double t32=0.;
    double time3=0.;
    for (i=0; i<itermax;i++)
    {
        t31 = MPI_Wtime();
        allreduce_nonblocking(Nloc, a, b, c, res3);
        t32 = MPI_Wtime();
        time3 += (t32 - t31);
    }

    double t41=0.;
    double t42=0.;
    double time4=0.;
    for (i=0; i<itermax;i++)
    {
        t41 = MPI_Wtime();
        allreduce_nonblocking_v2(Nloc, a, b, c, res4);
        t42 = MPI_Wtime();
        time4 += (t42 - t41);
    }

    if (rang == 0)
    {
        printf("compute duration : %.12e \n", time0);
        printf("reduction allg 3 = %.12e ; %.12e ; %.12e ; duration %.12e \n", res1[0],res1[1],res1[2],time1);
        printf("reduction allg 1 = %.12e ; %.12e ; %.12e ; duration %.12e \n", res2[0],res2[1],res2[2],time2);
        printf("reduction Iallg 3= %.12e ; %.12e ; %.12e ; duration %.12e \n", res3[0],res3[1],res3[2],time3);
        printf("reduction Iallg 1= %.12e ; %.12e ; %.12e ; duration %.12e \n", res4[0],res4[1],res4[2],time4);
        fflush(stdout);
    }

    free(a);
    free(b);
    free(c);

    MPI_Finalize();

    return 0;
}

