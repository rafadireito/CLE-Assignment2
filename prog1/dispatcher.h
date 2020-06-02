/**
 *  \file dispatcher.h (header file)
 *
 *  \brief Problem name: Frequency of word lengths and the number of vowels
 *
 *  Dispatcher header file
 *
 *  \author Rafael Direito - June 2020
 */

#include <stdbool.h>
#include "controlInfo.h"

#ifndef DISPATCHER
#define DISPATCHER

/** \brief The dispatcher loads the files to be processed */
extern void presentFileNames(char *inputFilenames[], unsigned int nFiles);

/** \brief Check if there are still files to be processed */
extern int file_available();

/** \brief Close a file */
extern void close_file();

/** \brief The dispacther gets a piece of data to be processed by a worker */
extern int get_data(ControlInfo *controlInfo);

/** \brief The dispatcher stores the results received from the workers */
extern void write_worker_results(ControlInfo *controlInfo);

/** \brief Show final results of the processing of the files */
extern int write_results();
#endif
