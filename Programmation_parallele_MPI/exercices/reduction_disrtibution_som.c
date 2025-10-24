#include <stdio.h>
#include <mpi.h>

void reduction_distribution_som(int *sendbuf, int *recvbuf, int recvcount)
{
	int rang, P; 

	MPI_Comm_size(MPI_COMM_WORLD, &P); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rang); 


	int lenth = recvcount*P; 
	int tab[P][lenth];
	MPI_Gather(&sendbuf, P, MPI_INT, tab, lenth,MPI_INT, 0, MPI_COMM_WORLD); 
        

	int tabStock[lenth];       	
	if(rang == 0) 
	{
	 
		for(int i=0; i<lenth; i++)
		{
			for(int j=0; j<P; j++)
			{
				tabStock[i] = tabStock[i] + tab[j][i]; 
			}
		}
	}
	MPI_Scatter(&tabStock,lenth,MPI_INT,&recvbuf,recvcount,MPI_INT, 0, MPI_COMM_WORLD);
	 
}

int main(int argc, char **argv)
{
	int rang, P; 
	
	MPI_Init(&argc, &argv); 
	
	 MPI_Comm_size(MPI_COMM_WORLD, &P); 
         MPI_Comm_rank(MPI_COMM_WORLD, &rang);

	int sendbuf[] = {1,2,3,4,5,6}; 
	int recvbuf[2];   
	
	reduction_distribution_som(sendbuf, recvbuf, 2); 
	 return 0; 
 
}


