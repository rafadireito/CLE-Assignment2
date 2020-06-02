/**
 *  \file controlInfo.h (header file)
 *
 *  \brief Problem name: Frequency of word lengths and the number of vowels.
 *
 *  Struct the will be used for the dispatcher and the workers to hold the information needed
 *
 *  \author Rafael Direito - June 2020
 */

#include "probConst.h"

#ifndef CONTROLINFO_H_
#define CONTROLINFO_H_

typedef struct {
    int fileIndex;
    int n_chars_read;
    int num_words_read;
    int max_num_vowels;
    int max_word_length;
    unsigned char chars_read[WORD_LENGTH * K];
    int word_lengths[WORD_LENGTH];
    int word_vowels[WORD_LENGTH][WORD_LENGTH];
} ControlInfo;
#endif
