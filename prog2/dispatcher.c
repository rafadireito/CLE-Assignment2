/**
 *  \file dispatcher.c
 *
 *  \brief Problem: compute the circular cross correlation of signals
 *
 *  Implements all the methods that will be called by the dispatcher
 *
 *  \author Rafael Direito - June 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "controlInfo.h"



/** \brief Struct to hold the file information */
struct FileInfo {
    int numSamplesPerSignal;
    double *signals[2];
    double *expectedResults;
    double *results;
    int currT;
};

/** \brief array that will hold each input file characteristics */
struct FileInfo *filesInfo;
/** \brief  number of file that will be processed */
int numFiles;


/**
 * \brief Process the files
 *
 * Pre-process the files, getting their number of signals, and other values.
 *
 * @param filenames array containing the name of all the files to be processed
 * @param nFiles number of file to be processed
 */
void loadFilesInfo(char *filenames[], unsigned int nFiles) {
    numFiles = nFiles;
    // allocate memory for array that will hold each input file characteristics
    filesInfo = malloc(sizeof(struct FileInfo) * nFiles);
    // check if memory allocation was successful
    if (filesInfo == NULL) {
        fprintf(stderr, "Error allocating memory");
        exit(EXIT_FAILURE);
    }

    // count the signals in each file and save this count to the signalsRead array
    FILE *mFilePtr;
    for (int i = 0; i < nFiles; i++) {
        // open the file that is currently being processed
        mFilePtr = fopen(filenames[i], "rb");
        // check if this operation was successful
        if (mFilePtr == NULL) {
            printf("ERROR: Unable to open the file: %s!\n", filenames[i]);
            exit(EXIT_FAILURE);
        }

        // get the number of signals in each of the files
        if (1 != fread(&filesInfo[i].numSamplesPerSignal, sizeof(int), 1, mFilePtr)) {
            printf("ERROR: Unable to read from file number %d!\n", i);
        }

        // set the current t to be processed to 0
        filesInfo[i].currT = 0;

        // read each signal to the array signals of each structure
        for (int signalIndex = 0; signalIndex < 2; signalIndex++) {
            // Allocate memory and check its allocation
            filesInfo[i].signals[signalIndex] = malloc(sizeof(double) * filesInfo[i].numSamplesPerSignal);
            if (filesInfo[i].signals[signalIndex] == NULL) {
                fprintf(stderr, "Error allocating memory");
                exit(EXIT_FAILURE);
            }

            // read all the values to each signal
            if (filesInfo[i].numSamplesPerSignal !=
                fread(filesInfo[i].signals[signalIndex], sizeof(double), filesInfo[i].numSamplesPerSignal, mFilePtr)) {
                printf("ERROR: Unable to read from file number %d!\n", i);
            }
        }

        // allocate space for the expected results and save them
        filesInfo[i].expectedResults = malloc(sizeof(double) * filesInfo[i].numSamplesPerSignal);
        if (filesInfo[i].expectedResults == NULL) {
            fprintf(stderr, "Error allocating memory");
            exit(EXIT_FAILURE);
        }

        if (filesInfo[i].numSamplesPerSignal !=
            fread(filesInfo[i].expectedResults, sizeof(double), filesInfo[i].numSamplesPerSignal, mFilePtr)) {
            printf("ERROR: Unable to read from file number %d!\n", i);
        }

        // allocate memory to save the results and check if this was successful
        filesInfo[i].results = malloc(sizeof(double) * filesInfo[i].numSamplesPerSignal);
        if (filesInfo[i].results == NULL) {
            fprintf(stderr, "Error allocating memory");
            exit(EXIT_FAILURE);
        }

        // close file
        if (0 != fclose(mFilePtr)) {
            fprintf(stderr, "Error on closing files+");
            exit(EXIT_FAILURE);
        }
    }
}



 /**
  * \brief get data to be processed by the worker
  *  The dispatcher asks for a piece of data to process.
  * The information on this data will be stored in the controlInfo struct ans send to the worker
  * @param controlInfo structure containing all the info needed to get data to process
  * @return true, if there is more data to be processed. false, if not.
  */
bool getPieceOfData(ControlInfo *controlInfo) {
    // indicates the worker if there is more work to be done
    bool existsMoreWork = false;

    controlInfo->fileID = 0;
    controlInfo->nSignals = 0;
    int controlInfoArrayIndex;

    for (int fileIndex = 0; fileIndex < numFiles; fileIndex++) {
        // set the controlInfoArrayIndex to 0
        controlInfoArrayIndex = 0;

        // if all values of t haven't been processed
        if (filesInfo[fileIndex].currT != filesInfo[fileIndex].numSamplesPerSignal) {
            // update data in the control info
            controlInfo->fileID = fileIndex;
            controlInfo->nSignals = filesInfo[fileIndex].numSamplesPerSignal;

            if (filesInfo[fileIndex].numSamplesPerSignal - filesInfo[fileIndex].currT > NUMBER_OF_T_TO_PROCESS) {
                // change ts to be computed
                for (int tToProcess = filesInfo[fileIndex].currT;
                     tToProcess < filesInfo[fileIndex].currT + NUMBER_OF_T_TO_PROCESS; tToProcess++)
                    controlInfo->tValuesToProcess[controlInfoArrayIndex++] = tToProcess;
                // update the current t
                filesInfo[fileIndex].currT += NUMBER_OF_T_TO_PROCESS;
            } else {
                // change ts to be computed
                for (int tToProcess = filesInfo[fileIndex].currT;
                     tToProcess < filesInfo[fileIndex].numSamplesPerSignal; tToProcess++) {
                    controlInfo->tValuesToProcess[controlInfoArrayIndex++] = tToProcess;
                }
                // fill the remaining available positions with -1
                while (controlInfoArrayIndex != NUMBER_OF_T_TO_PROCESS)
                    controlInfo->tValuesToProcess[controlInfoArrayIndex++] = -1;
                // update the current t
                filesInfo[fileIndex].currT = filesInfo[fileIndex].numSamplesPerSignal;
            }

            // Allocate memory to send the signals
            controlInfo->x = malloc(sizeof(double) * filesInfo[fileIndex].numSamplesPerSignal);
            controlInfo->y = malloc(sizeof(double) * filesInfo[fileIndex].numSamplesPerSignal);

            // Check if allocation was successful
            if (controlInfo->x == NULL) {
                fprintf(stderr, "Error allocating memory");
                exit(EXIT_FAILURE);
            }
            if (controlInfo->y == NULL) {
                fprintf(stderr, "Error allocating memory");
                exit(EXIT_FAILURE);
            }

            // Save the signals in the control info
            controlInfo->x = filesInfo[fileIndex].signals[0];
            controlInfo->y = filesInfo[fileIndex].signals[1];

            // inform that there is work to be done
            existsMoreWork = true;
            break;
        }
    }
    return existsMoreWork;
}


/**
 * \brief Stores a computed result in the shared region.
 * @param controlInfo structure containing all the info needed to store a specific value of a signal
 */
void savePartialResults(ControlInfo *controlInfo) {
    // save the results to the files array  results
    for (int tIndex = 0; tIndex < NUMBER_OF_T_TO_PROCESS; tIndex++) {
        // check if we are not dealing with the last portion of the signal
        if (controlInfo->tValuesToProcess[tIndex] != -1) {
            filesInfo[controlInfo->fileID].results[controlInfo->tValuesToProcess[tIndex]] = controlInfo->resultsFromProcessing[tIndex];
        }
    }
}


/**
 * \brief Used to print the result of the computations
 *
 * Will print the number of errors that happened during the computations.
 * Will also print the error rate associated with the computations for each file.
 */
void printResults() {
    // holds the number of different values found
    int different;

    printf("\nResults vs Expected Results:\n");
    // iterate through all the files
    for (int fileIndex = 0; fileIndex < numFiles; fileIndex++) {
        // reset number of different values
        different = 0;

        // compare the the results with the expected ones
        for (int t = 0; t < filesInfo[fileIndex].numSamplesPerSignal; t++) {
            if (filesInfo[fileIndex].results[t] != filesInfo[fileIndex].expectedResults[t])
                different++;
        }

        // Inform the user of the validity of the computations
        printf("For File %d, the results differ from the expected ones by %d values. Error rate =  %.3f.\n",
               fileIndex, different, ((double) different) / ((double) filesInfo[fileIndex].numSamplesPerSignal));
    }
    printf("\n");
}