#include <mpi.h>
#include "dispatcher.h"
#include "worker.h"
#include "controlInfo.h"
#include "probConst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>

/** \brief Declaration of function*/
void command_usage(char *cmdName);

/** \brief number of the next worker that will receive work to do*/
int currWorker = 1;

/** \brief workers count*/
int numWorkers;


/**
 * Implements a "circular buffer", that will point the next worker to receive work
 * @return workerId
 */
int getNextWorkerRank() {
    if (currWorker > numWorkers)
        currWorker = 1;
    return currWorker++;
}


/**
 * Dispatcher function
 * Will be called, only by the dispatcher, to implement its life cycle
 * @param filenames name of the files passed by the user
 * @param nFiles num of files passed as argument
 */
void dispatcher(char *filenames[], unsigned int nFiles) {
    // control info structure for sending and receiving messages
    ControlInfo controlInfo;
    // id of the worker that will compute a value
    int workerAssigned;
    // if true, we will send work to the workers
    bool isWorkToBeDone = true;
    // time limits
    double t0, t1;

    // get the starting time
    t0 = ((double) clock ()) / CLOCKS_PER_SEC;

    // Present the filenames
    presentFileNames(filenames, nFiles);


    // while there are results to be computed, send data to the workers
    while (get_data((ControlInfo *) &controlInfo)) {
        // get next worker id
        workerAssigned = getNextWorkerRank();

        // tell worker there is work to be done
        MPI_Send(&isWorkToBeDone, 1, MPI_C_BOOL, workerAssigned, 0, MPI_COMM_WORLD);

        // send message to worker
        MPI_Send(&controlInfo, sizeof(ControlInfo), MPI_BYTE, workerAssigned, 0, MPI_COMM_WORLD);

        // wait for workers response
        MPI_Recv(&controlInfo, sizeof(ControlInfo), MPI_BYTE, workerAssigned, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // save the results in the dispatcher
        write_worker_results((ControlInfo *) &controlInfo);
    }

    // Inform workers there is no more work to be done
    isWorkToBeDone = false;
    for (int i = 1; i <= numWorkers; i++) {
        // tell worker there is work to be done
        MPI_Send(&isWorkToBeDone, 1, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
    }

    // Print the results obtained
    write_results();

    // print for debugging
    //printf("The root process is leaving...\n");

    // print elapsed time
    t1 = ((double) clock ()) / CLOCKS_PER_SEC;
    printf ("\nElapsed time = %.6f s\n\n", t1 - t0);
}


/**
 * Implement the workers life cycle.
 * This entity will receive data from the dispatcher, will compute the results and send them back
 * to the dispatcher.
 * @param rank rank of the worker process
 */
void worker(int rank) {
    // if true, we will send work to the workers
    bool isWorkToBeDone;

    // control info for the worker
    ControlInfo controlInfo;

    // worker lifecycle
    while (true) {
        // check if there is work to be done
        MPI_Recv(&isWorkToBeDone, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (!isWorkToBeDone) {
            //printf("Worker with rank %d is leaving...\n", rank);
            return;
        }

        // wait for work
        MPI_Recv(&controlInfo, sizeof(ControlInfo), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // set counts to zero
        memset(controlInfo.word_lengths, 0, sizeof controlInfo.word_lengths);
        memset(controlInfo.word_vowels, 0, sizeof controlInfo.word_vowels);
        // Process data
        process_data((ControlInfo *) &controlInfo);

        // send results to the root process
        MPI_Send(&controlInfo, sizeof(ControlInfo), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
}


/**
 * \brief Verify if the console command was well executed.
 *
 * @param argc total number of arguments in the command.
 * @param argv pointer to the array that contains the arguments in the command.
 * @return EXIT_SUCCESS if the command was correctly executed, EXIT_FAILURE otherwise.
 */
int process_command(int argc, char *argv[], char **filenames) {
    /* option chosen by the user */
    int opt;

    do {
        switch ((opt = getopt (argc, argv, "h"))) {
            case 'h': /* help mode */
                command_usage(basename (argv[0]));
                return EXIT_FAILURE;
            case '?': /* invalid option */
                fprintf(stderr, "%s: invalid option\n", basename (argv[0]));
                command_usage(basename (argv[0]));
                return EXIT_FAILURE;
            case -1:
                break;
        }
    } while (opt != -1);

    /* if there are no arguments in the command */
    if (argc == 1) {
        fprintf(stderr, "%s: invalid format\n", basename (argv[0]));
        command_usage(basename (argv[0]));
        return EXIT_FAILURE;
    }

    /* saves the filenames in the array */
    for (int o = 1; o < argc; o++)
        filenames[o - 1] = argv[o];

    return EXIT_SUCCESS;
}


/**
 * \brief Print command usage.
 *
 * A message specifying how the program should be called is printed.
 *
 * @param cmdName pointer with the name of the command
 */
void command_usage(char *cmdName) {
    fprintf (stderr, "\nSynopsis: %s [OPTIONS] [filename1 filename2 ...]\n"
                     "  OPTIONS:\n"
                     "  -h      --- print this help\n", cmdName);
}


/**
 * Main method
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {
    int rank;
    // stores the number of processes that were launched by the mpi
    int world_size;

    char **filenames;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // compute number of workers
    numWorkers = world_size - 1;
    if (rank == 0) {
        // allocate memory for the filenames of the files to be processed
        filenames = malloc((argc - 1) * sizeof(char *));

        // process the command and act according to it
        int command_result = process_command(argc, argv, filenames);
        if (command_result != EXIT_SUCCESS)
            return command_result;

        // launch dispatcher
        dispatcher(filenames, argc - 1);
    } else {
        worker(rank);
    }
    MPI_Finalize();
    return 0;
}

