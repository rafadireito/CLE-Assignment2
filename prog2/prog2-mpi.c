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

/** \brief filenames of the files to be processed */
char **filenames;

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
    t0 = ((double) clock()) / CLOCKS_PER_SEC;

    // get files info
    loadFilesInfo(filenames, nFiles);

    // while there are results to be computed, send data to the workers
    while (getPieceOfData((ControlInfo *) &controlInfo)) {
        // get next worker id
        workerAssigned = getNextWorkerRank();

        // tell worker there is work to be done
        MPI_Send(&isWorkToBeDone, 1, MPI_C_BOOL, workerAssigned, 0, MPI_COMM_WORLD);

        // send message to worker
        MPI_Send(&controlInfo, sizeof(ControlInfo), MPI_BYTE, workerAssigned, 0, MPI_COMM_WORLD);

        // The structure containers pointers to the arrays x and y. The MPI cant pass this pointers, so
        // we will have to pass them as arrays
        double x_signal[controlInfo.nSignals];
        double y_signal[controlInfo.nSignals];

        for (int i = 0; i < controlInfo.nSignals; i++) {
            x_signal[i] = controlInfo.x[i];
            y_signal[i] = controlInfo.y[i];
        }

        // send the signals
        MPI_Send(x_signal, controlInfo.nSignals, MPI_DOUBLE, workerAssigned, 0, MPI_COMM_WORLD);
        MPI_Send(y_signal, controlInfo.nSignals, MPI_DOUBLE, workerAssigned, 0, MPI_COMM_WORLD);

        // wait for workers response
        MPI_Recv(&controlInfo, sizeof(ControlInfo), MPI_BYTE, workerAssigned, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // save the results in the dispatcher
        savePartialResults((ControlInfo *) &controlInfo);
    }

    // Print the results obtained
    printResults();

    // Inform workers there is no more work to be done
    isWorkToBeDone = false;
    for (int i = 1; i <= numWorkers; i++) {
        // tell worker there is work to be done
        MPI_Send(&isWorkToBeDone, 1, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
    }
    // print for debugging
    printf("The root process is leaving...\n");

    // print elapsed time
    t1 = ((double) clock()) / CLOCKS_PER_SEC;
    printf("\nElapsed time = %.6f s\n\n", t1 - t0);
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

        // Since the signals were pointers, in the dispatcher, the MPI cant pass them
        // We have to pass their arrays individually
        double x_signal[controlInfo.nSignals];
        double y_signal[controlInfo.nSignals];

        // receive x and y signals
        MPI_Recv(x_signal, controlInfo.nSignals, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(y_signal, controlInfo.nSignals, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // replace the pointers in the structure
        controlInfo.x = x_signal;
        controlInfo.y = y_signal;

        // print for debugging
        //printf("Worker with rank %d will compute results.\n", rank);

        // do the work
        processData((ControlInfo *) &controlInfo);

        // send results to the root process
        MPI_Send(&controlInfo, sizeof(ControlInfo), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
}


/**
 * \brief Print command usage.
 * A message specifying how the program should be called is printed.
 * \param cmdName string with the name of the command
 */
static void command_usage(char *cmdName) {
    fprintf(stderr, "\nSynopsis: %s [OPTIONS] [filename1 filename2 ...]\n"
                    "  OPTIONS:\n"
                    "  -h      --- print this help\n",
            cmdName);
}


/**
 * \brief Processes the input command
 * @return success of the validation
 */
static int process_command(int argc, char *argv[]) {
    int opt;
    opterr = 0;
    do {
        switch ((opt = getopt(argc, argv, "h"))) {
            case 'h': /* help mode */
                command_usage(basename(argv[0]));
                return EXIT_SUCCESS;
            case '?': /* invalid option */
                fprintf(stderr, "%s: invalid option\n", basename(argv[0]));
                command_usage(basename(argv[0]));
                return EXIT_FAILURE;
            case -1:
                break;
        }
    } while (opt != -1);

    if (argc == 1) {
        fprintf(stderr, "\n%s: invalid format\n", basename(argv[0]));
        command_usage(basename(argv[0]));
        return EXIT_FAILURE;
    }

    for (int o = 1; o < argc; o++)
        filenames[o - 1] = argv[o];

    return EXIT_SUCCESS;
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

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // compute number of workers
    numWorkers = world_size - 1;

    // if the rank of the process is  0, this will be the dispatcher
    if (rank == 0) {
        // allocate memory for the filenames of the files to be processed
        filenames = malloc((argc - 1) * sizeof(char *));

        // process the command and act according to it
        int command_result = process_command(argc, argv);
        if (command_result != EXIT_SUCCESS)
            return command_result;

        // launch dispatcher
        dispatcher(filenames, argc - 1);


    }
    // if the rank of the process is  different than 0, we have a worker process
    else
        worker(rank);

    MPI_Finalize();
    return 0;
}


