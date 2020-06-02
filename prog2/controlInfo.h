/**
 *  \file controlInfo.h (header file)
 *
 *  \brief Problem: compute the circular cross correlation of signals
 *
 *  Struct the will be used for the worker to hold the information needed
 *
 *  \author Rafael Direito - June 2020
 */

#include "probConst.h"

#ifndef CONTROLINFO
#define CONTROLINFO

/**
 * Structure that eah worker will hold.
 * The workers will communicate with the shared region using this struct, that will hold all the values needed
 * for them to execute the computations.
 */
typedef struct {
    // id of the file being processed
    unsigned int fileID;
    // number of signals existing in the file that is being processed
    unsigned int nSignals;
    // will hold the t values that a worker will have to compute
    int tValuesToProcess[NUMBER_OF_T_TO_PROCESS];
    // will hold the results obtained by a worker, during one iteration
    double resultsFromProcessing[NUMBER_OF_T_TO_PROCESS];
    // will hold the values of the signal x
    double *x;
    // will hold the values of the signal y
    double *y;
} ControlInfo;

#endif