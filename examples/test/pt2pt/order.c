#include <stdio.h>
#include "mpi.h"

#if defined(NEEDS_STDLIB_PROTOTYPES)
#include "protofix.h"
#endif


int main( int argc, char *argv[] )
{
    int easy;
    int rank;
    int size;
    int a;
    int b;
    MPI_Request request;
    MPI_Status  status;
    double t1, t0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* This test depends on a working wtime.  Make a simple check */
    t0 = MPI_Wtime();
    if (t0 == 0 && MPI_Wtime() == 0) {
	/* We MAY have a problem.  Check some more */
	for (a=0; a<1000; a++) t0 = MPI_Wtime();
	if (t0 == 0) {
	    fprintf( stderr, 
		     "MPI_WTIME is returning 0; a working value is needed\n\
for this test.\n" );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
    }

    easy = 1;

    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == 0)
    {
	MPI_Irecv(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
	MPI_Recv(&b, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
	MPI_Wait(&request, &status);
	/* Check for correct values: */
	if (a == 1 && b == 2) {
	    printf( " No Errors\n" );
	}
	else {
	    printf("rank = %d, a = %d, b = %d\n", rank, a, b);
	}
    }
    else
    {
	t1 = MPI_Wtime();
	while (MPI_Wtime() - t1 < easy) ;
	a = 1;
	b = 2;
	MPI_Send(&a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&b, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    

    MPI_Finalize();
    return 0;
}
