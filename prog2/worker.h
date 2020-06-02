/**
 *  \file worker.h (header file)
 *
 *  \brief Problem: compute the circular cross correlation of signals
 *
 *  Worker header file
 *
 *  \author Rafael Direito - June 2020
 */

#include <stdbool.h>
#include "controlInfo.h"

#ifndef WORKER
#define WORKER

/** \brief Processes the data received from the dispatcher */
extern void processData(ControlInfo *controlInfo);

#endif
