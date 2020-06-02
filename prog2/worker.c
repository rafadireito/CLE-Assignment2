/**
 *  \file worker.c
 *
 *  \brief Problem: compute the circular cross correlation of signals
 *
 *  Implements all the methods that will be called by the worker processes
 *
 *  \author Rafael Direito - June 2020
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>
#include "probConst.h"
#include "controlInfo.h"


/**
 * Contains the logic to process the data sent by the dispatcher
 * @param controlInfo
 */
void processData(ControlInfo *controlInfo) {
    int t;
    double crossCorrelation;
    // process the t values the worker was asked
    for (int tIndex = 0; tIndex < NUMBER_OF_T_TO_PROCESS; tIndex++) {
        // check if we are not dealing with the last portion of the signal
        if ((t = controlInfo->tValuesToProcess[tIndex]) != -1) {
            // compute the circular cross correlation
            crossCorrelation = 0;
            for (int k = 0; k < controlInfo->nSignals; k++) {
                crossCorrelation += (controlInfo->x[k] * controlInfo->y[(t + k) % controlInfo->nSignals]);
            }
            // save the result to the control info array
            controlInfo->resultsFromProcessing[tIndex] = crossCorrelation;

        }
    }
}