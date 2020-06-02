/**
 *  \file dispatcher.h (header file)
 *
 *  \brief Problem: compute the circular cross correlation of signals
 *
 *  Dispatcher header file
 *
 *  \author Rafael Direito - June 2020
 */

#include <stdbool.h>
#include "controlInfo.h"

#ifndef DISPATCHER
#define DISPATCHER

/** \brief Loads the files passed as argument */
extern void loadFilesInfo(char *filenames[], unsigned int nFiles);

/** \brief used by the dispatcher to send a piece of data to process, to the worker*/
extern bool getPieceOfData(ControlInfo *controlInfo);

/** \brief Save computation results*/
extern void savePartialResults(ControlInfo *controlInfo);

/** \brief Print final results*/
extern void printResults();

#endif
