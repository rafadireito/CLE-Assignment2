/**
 *  \file worker.h (header file)
 *
 *  \brief Problem: Frequency of word lengths and the number of vowels
 *
 *  Worker header file
 *
 *  \author Rafael Direito - June 2020
 */

#include <stdbool.h>
#include "controlInfo.h"

#ifndef WORKER
#define WORKER

/** \brief Check if a given character, in hexadecimal, is a vowel. */
extern int check_vowel(const char* character_hex);

/** \brief Check if a given character, in hexadecimal, is one of the characters which are considered to split words. */
extern int is_split_char(const char* character_hex);

/** \brief Process the K tokens retrieved from the current open file. */
extern void process_data(ControlInfo *controlInfo);

#endif
