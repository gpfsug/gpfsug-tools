/* 
 * Name: mmmkfile.c
 * Description: Preallocates a file of k/kb/MB/GB/TB on a GPFS filesystem
 *              Imitates the behaviour of the standard UNIX mkfile utility
 *              Majority of code was shamelessly cribbed from IBMs preallocate.c example on:
 *              https://publib.boulder.ibm.com/infocenter/clresctr/vxrx/index.jsp?topic=%2Fcom.ibm.cluster.gpfs.v3r4.gpfs300.doc%2Fbl1adm_preallc.html
 * 
 * Author: Jez Tucker
 * Date: 24 Feb 2012
 * Version : 0.1
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <gpfs.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUG 1
#define TRUE 1
#define FALSE 0

int opt, optind;
int fileHandle;
unsigned long long bytesToAllocate;				// size of the file in bytes
unsigned long long startOffset;
unsigned int numStreams;
unsigned long long chunkSize;
//char fileName[UCHAR_MAX];
char *fileName;

struct thread_info
{
	pthread_t thread_id;
	int thread_num;
	long long start_byte;
	long long num_bytes;
};

void
print_help() 
{
    fprintf(stdout, "help file\n");

/*    b // default
    k
    m
    g
    t
    
    n 'Create an empty filename. The size is noted, but  disk
           blocks  are  not  allocated  until  data is written to
           them. Files created with this option cannot be swapped
           over local UFS mounts.'
    v 'Verbose.  Report the names and sizes of created files'

*/

}

/* Preallocate the file
   This functon is wrapped in case we want to provide more 
   feedback / checking
*/
int
preallocateFile(int fileHandle, long long startOffset, long long bytesToAllocate) 
{
    int rc = gpfs_prealloc(fileHandle, startOffset, bytesToAllocate);
    if (rc < 0)
    {
        fprintf(stderr, "Error %d: preallocation at %lld for %lld in %s\n",
             errno, startOffset, bytesToAllocate, fileName);
		return (-1);
    }
	
	return (TRUE);
}


static void *
process_request(void *arg)
{
	unsigned long long end_byte, bytes_written;
    struct thread_info *tinfo = (struct thread_info *) arg;

	end_byte = tinfo->start_byte + tinfo->num_bytes;

	fprintf(stdout, "tn: %d\tStart: %d\tEnd: %d\n", tinfo->thread_num, tinfo->start_byte, end_byte);
	
	// start timer

	// open the file at startBlock
	// zeroFill the file until startBlock + fillSize
	// stop timer

	// print stats
	//fprintf(stdout, "Thread %d wrote %d bytes in %d seconds (%d MB/sec)", zerofFillRequestProcess, , , );

	return (NULL);

}



extern int 
main(int argc, char *argv[])
{
	int s;
	void *res;


	/************** PROCESS ARGS START ***************/
	int argCount = 0;
	if (DEBUG) 
	{
    	for (argCount = 0; argCount < argc; argCount++)
		{
	  		printf("argv[%d] = %s\n", argCount, argv[argCount]);
		}
	}

    // Check the args
    while ((opt = getopt(argc, argv, "b:k:m:g:t:s:c:nvh")) != EOF) 
    {
		switch (opt) {
	    	case 'h' :
				print_help();
				exit(EXIT_FAILURE);
				break;
	    	case 't' : // terabytes
				if (optarg) {
		    		bytesToAllocate = atoll(optarg)*1024*1024*1024*1024;
            	} else {
					fprintf(stdout, "no arg");
				}
				break;
	    	case 'g' : // gigabytes
				bytesToAllocate = atoll(optarg)*1024*1024*1024;
				break;
	    	case 'm' : // megabytes
				bytesToAllocate = atoll(optarg)*1024*1024;
				break;
	    	case 'k' : // kilobytes
				bytesToAllocate = atoll(optarg)*1024;
				break;
            case 's': // number of zerofill 'streams'
                numStreams = atoll(optarg);
				break;
            case 'c' : // block size of streams
                chunkSize = atoll(optarg);
                break;
	    	case 'b':  // bytes 
				// fall through to default
	    	default:
				bytesToAllocate = atoll(optarg); // bytes
			break;
		}
    }

    // Check a filename was passed
    if (optind >= argc)
    {
		fprintf(stderr, "Expected filename after options\n");
		exit(EXIT_FAILURE);
    } 
    else
    {
		//memset(fileName, 0, sizeof(fileName));
		fileName = argv[optind];	
    }

    /*************** PROCESS ARGS END **************/
    /* Open the file handle */
    fileHandle = open(fileName, O_RDWR|O_CREAT, 0644);
    if (fileHandle < 0)
    {
        perror(fileName);;
        exit(EXIT_FAILURE);
    }  


	/************ PREALLOCATE FILE START ***********/
	if (! preallocateFile(fileHandle, 0, bytesToAllocate)) {

		fprintf(stderr, "Could not preallocate file %s. Exiting.\n", fileName);
		exit (EXIT_FAILURE);
	}

	/********** PREALLOCATE FILE END *********/


	
	// DEFINE A NUMBER OF THREADS
    int rc = 0;
	int tnum = 10;
	int wCount = 0;
	int num_threads = 10;

	thread_info *tinfo;
	pthread_t attr;

    /* Allocate memory for the pthreads */
	tinfo = (thread_info *)malloc(sizeof(struct thread_info));
	if (tinfo == NULL) 
	{
		printf("tinfo err\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "total bytes: %llu\n", bytesToAllocate);
	unsigned long long next_byte = 1;

	/* Create one thread for each block of the file */
	for (tnum=1; tnum <= num_threads; tnum++) 
	{
		/* Fill in the thread_info struct to pass to the thread */
		tinfo[tnum].thread_num = tnum;
		tinfo[tnum].start_byte = next_byte;
		tinfo[tnum].num_bytes = (bytesToAllocate / num_threads) - 1;

		next_byte = tinfo[tnum].start_byte + tinfo[tnum].num_bytes + 1;

		/* Create the thread */
		rc = pthread_create(&tinfo[tnum].thread_id, NULL, &process_request, &tinfo[tnum]);
		if (rc) {
			exit(EXIT_FAILURE);
		}
	}

	/* Now join with each thread, and display its returned value */

/*	for (tnum = 0; tnum < num_threads; tnum++)
	{
		s = pthread_join(tinfo[tnum].thread_id, &res);
        if (s != 0)
		{
			printf("pthread_join err\n");
		}
        
		printf("Joined with thread %d; returned value was %s\n", tinfo[tnum].thread_num, (char *) res); */
        //free(res);      /* Free memory allocated by thread */
//    }

    /* Exit nicely */
	exit(EXIT_SUCCESS);
}
