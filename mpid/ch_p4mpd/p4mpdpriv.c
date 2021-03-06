/* 
 * This file contains code private to the p4 implementation of the ADI device
 * Primarily, this contains the code to setup the initial environment
 * and terminate the program
 */
#include "mpid.h"
#include "mpiddev.h"
#include "mpimem.h"

int __P4FROM, __P4LEN, __P4TYPE, __P4GLOBALTYPE;

#ifdef FOO
static char **P4Args = 0;
static char *P4Argstr = 0;
#endif

/*
 * This routine must be careful NOT to update argv[0], the name of the
 * program.  It does handle propagating the args to all of the processes.
 */
void MPID_P4_Init( argc, argv )
int *argc;
char ***argv;
{
#ifdef FOO    
    int narg,nlen,i,*arglen;
    char *p,*argstr;
#endif
    p4_initenv(argc,*argv);
    p4_post_init();		/* do any special post_init stuff */
    MPID_MyWorldRank = p4_get_my_id();
    MPID_MyWorldSize = p4_num_total_slaves()+1;
    p4_dprintfl( 20, "MPID_P4_Init: MPID_MyWorldRank = %d\n", MPID_MyWorldRank );
    p4_dprintfl( 20, "MPID_P4_Init: MPID_MyWorldSize = %d\n", MPID_MyWorldSize );

/*-------------------------------------------------------------------------------*/
/* the rest of this code is unnecessary since the MPD's handle command-line args */
/*-------------------------------------------------------------------------------*/

#ifdef FOO
    printf("FOO defined!\n" );
    __P4GLOBALTYPE = 1010101010;
    fprintf(stderr, "worldrank=%d, worldsize=%d\n", MPID_MyWorldRank, MPID_MyWorldSize);
    if (MPID_MyWorldRank == 0) {
	p4_broadcastx( __P4GLOBALTYPE,argc,sizeof(int),P4INT);
    }
    else {
	PIbrecv(__P4GLOBALTYPE,argc,sizeof(int),P4INT);
    }
    narg   = *(argc);
    arglen = (int *)MALLOC( narg * sizeof(int) );
    if (narg>0 && !arglen) { 
	p4_error( "Could not allocate memory for commandline arglen",narg);}
    if (PImytid==0) {
	for (i=0; i<narg; i++) 
	    arglen[i] = strlen((*(argv))[i]) + 1;
    }
    if (MPID_MyWorldRank == 0) 
	p4_broadcastx( __P4GLOBALTYPE,arglen,sizeof(int)*narg,P4INT);
    else {
	PIbrecv(__P4GLOBALTYPE,arglen,sizeof(int)*narg,P4INT);
    }
    nlen = 0;
    for (i=0; i<narg; i++) 
	nlen += arglen[i];
    argstr = (char *)MALLOC( nlen );
    if (nlen>0 && !argstr) { 
	p4_error( "Could not allocate memory for commandline args",nlen);}
    P4Argstr = argstr;

    if (PImytid==0) {
	p = argstr;
	for (i=0; i<narg; i++) {
	    strcpy( p, (*argv)[i] );
	    p  += arglen[i];
	}
    }
    if (MPID_MyWorldRank == 0) 
	p4_broadcastx( __P4GLOBALTYPE,argstr,nlen,P4NOX);
    else {
	PIbrecv(__P4GLOBALTYPE,argstr,nlen,P4NOX);
        }
    if (PImytid!=0) {
	/* save the program name */
	char *argv0;
	/* Note that in some cases, argv or *argv may be null */
	if (argv && *argv) argv0 = (*argv)[0];
	else               argv0 = 0;
	/* Replace argv with a new array of arguments */
	*(argv) = (char **) MALLOC( (nlen + 1) * sizeof(char *) );
	if (nlen > 0 && !*(argv)) { 
	    p4_error( "Could not allocate memory for commandline argv",nlen);}
	/* Save this so that it can be freed on exit */
	P4Args = *argv;
	
	p = argstr;
	/* (*(argv))[0] = argstr; */
	(*(argv))[0] = argv0;
	/* Skip over the program name */
	p += arglen[0];
	for (i=1; i<narg; i++) {
	    (*(argv))[i] = p;
	    p += arglen[i];
	}
	/* Some systems expect a null terminated argument string */
	(*(argv))[narg] = 0;
    }
    else {
	FREE(argstr);
	P4Argstr = 0;
    }
    FREE(arglen);
#endif /* FOO */
}

void MPID_P4_End()
{
#ifdef FOO
    /* String containing the values */
    if (P4Argstr) {
	FREE( P4Argstr );
    }
    if (P4Args) {
	/* P4Args is the argv vector */
	FREE( P4Args );
    }
#endif
    p4_wait_for_end();
}

