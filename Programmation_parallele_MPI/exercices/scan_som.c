#include <stdio.h>
#include <mpi.h>

int scan_som(int in)
{
	int rang, P, out; 
	MPI_Status sta; 

	MPI_Comm_size(MPI_COMM_WORLD, &P); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rang); 

	if(rang != 0)
	{
		MPI_Recv(&out, 1, MPI_INT, rang-1, 1000, MPI_COMM_WORLD, &sta); 
	}
	else
	{
		out = 0; 
	}
	
	out = out +  in ; 
	
	if(rang != P-1)
	{
		MPI_Send(&out, 1, MPI_INT, rang+1, 1000, MPI_COMM_WORLD);
	}
	
	return out; 
} 

int main(int argc, char **argv)
{
	int rang, P, res; 
	
	MPI_Init(&argc,&argv); 	
	
	MPI_Comm_size(MPI_COMM_WORLD, &P); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	
	res = scan_som(rang); 
	
	printf("P%d, resultat scan = %d\n", rang, res); 

	MPI_Finalize(); 
	return 0; 
}
