/**
 *  \file dispatcher.c
 *
 *  \brief Problem name: Frequency of word lengths and the number of vowels.
 *
 *  Implements all the methods that will be called by the dispatcher
 *
 *  \author Rafael Direito - June 2020
 */

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "probConst.h"
#include "controlInfo.h"

/** \brief pointer which contains the file information. */
FILE *fp;

/** \brief pointer that contains the all the filenames retrieved from the command arguments. */
char **filenames;

/** \brief total number of filenames retrieved. */
int num_files;

/** \brief pointer that saves the total number of words found in each file. */
int *gbl_total_num_words;

/** \brief pointer that saves the highest numbers of vowels found in a word in each file. */
int *gbl_max_num_vowels;

/** \brief pointer that saves the sizes of the longest word found in each file. */
int *gbl_max_word_length;

/** \brief pointer that saves the lengths of words found in each file. */
int (*gbl_word_lengths)[WORD_LENGTH];

/** \brief pointer that saves the number of vowels associated to the length of each word in each file. */
int (*gbl_word_vowels)[WORD_LENGTH][WORD_LENGTH];

/** \brief index which represents the current opened file. */
int files_idx = -1;

/** \brief flag that indicates if the current file was open. */
int file_opened = 0;

/** \brief flag that indicates if the current file was closed. */
int file_closed = 1;




/**
 * Used by the dispatcher to present the filenames to be processed
 * @param inputFilenames name sof the files to be processed
 * @param nFiles number of files to be processed
 */
void presentFileNames(char *inputFilenames[], unsigned int nFiles){
    // Store filenames
    filenames = inputFilenames;

    num_files = nFiles;

    // Allocate spaces
    gbl_total_num_words = malloc(nFiles);
    gbl_max_num_vowels = malloc(nFiles);
    gbl_max_word_length = malloc(nFiles);
    gbl_word_lengths = malloc(sizeof(int[WORD_LENGTH]) * (nFiles));
    gbl_word_vowels = malloc(sizeof(int[WORD_LENGTH][WORD_LENGTH]) * (nFiles));

    for (int i = 0; i<nFiles; i++) {
        gbl_total_num_words[i] = 0;
        gbl_max_num_vowels[i] = 0;
        gbl_max_word_length[i] = 0;
    }
}

/**
 * \brief Verify if there's a file remaining to be opened and, if exists, open it.
 * *
 * @return 1 if a file is opened, 0 otherwise.
 */
int file_available() {
    int file_available = 1;
    files_idx++;

    /* if there's still files to be read and there's no file opened, open the current file. */
    if (files_idx < num_files) {
        file_opened = 1;
        file_closed = 0;
        fp = fopen(filenames[files_idx], "r");

        if (fp == NULL) {
            printf("ERROR: Unable to open the file: %s\n", filenames[files_idx]);
            file_available = 0;
            file_opened = 0;
        }
        memset(gbl_word_lengths[files_idx], 0, sizeof gbl_word_lengths[files_idx]);
        memset(gbl_word_vowels[files_idx], 0, sizeof gbl_word_vowels[files_idx]);
    }
    else
        file_available = 0;
    return file_available;
}

/**
 * \brief Close the current opened file and indicate that the next file can be opened.
 */
void close_file() {
    if (!file_closed) {
        fclose(fp);
        file_opened = 0;
        file_closed = 1;
    }
}

/**
 * \brief Retrieve K tokens from the current open file.
 *
 * Operation carried out by the dispatcher, to send the work to the workers
 * @param controlInfo structure containing all the info needed to get data to process
 * @return 1 if there's still data to read from the file, 0 otherwise.
 */
int get_data(ControlInfo *controlInfo) {

    int num_chars_read = 0;
    int num_tokens_read = 0;
    unsigned char chr;
    int data_avail = 1;

    if (!file_opened)
        data_avail = file_available();

    // save file index to the crontrol structure
    controlInfo->fileIndex = files_idx;

    if (data_avail) {
        /* read the next character and saves it until K tokens are constructed or the EOF character is found. */
        while (num_tokens_read != K && !feof(fp)) {
            chr = fgetc(fp);
            controlInfo->chars_read[num_chars_read] = chr;
            num_chars_read += 1;

            if (chr == ' ')
                num_tokens_read += 1;
        }

    }
    controlInfo->n_chars_read = num_chars_read;

    /* if it reached the end of the file. */
    if (num_tokens_read == 0)
        close_file();

    return data_avail;
}

/**
 * \brief Write the obtained results, by teh dispatcher, in the variables.
 *
 * Operation carried out by the dispatcher.
 *
 * @param controlInfo structure containing all the info needed
 */
void write_worker_results(ControlInfo *controlInfo) {
    gbl_total_num_words[controlInfo->fileIndex] += controlInfo->num_words_read;

    if (controlInfo->max_num_vowels > gbl_max_num_vowels[files_idx])
        gbl_max_num_vowels[controlInfo->fileIndex] = controlInfo->max_num_vowels;

    if (controlInfo->max_word_length > gbl_max_word_length[controlInfo->fileIndex])
        gbl_max_word_length[controlInfo->fileIndex] = controlInfo->max_word_length ;

    for (int i = 0; i < sizeof(gbl_word_lengths[controlInfo->fileIndex]) / sizeof(gbl_word_lengths[controlInfo->fileIndex][0]); i++)
        gbl_word_lengths[controlInfo->fileIndex][i] += controlInfo->word_lengths[i];

    for (int i = 0; i < sizeof(gbl_word_vowels[controlInfo->fileIndex]) / sizeof(gbl_word_vowels[controlInfo->fileIndex][0]); i++)
        for (int j = 0; j < sizeof(gbl_word_vowels[controlInfo->fileIndex][0]) / sizeof(gbl_word_vowels[controlInfo->fileIndex][0][0]); j++)
            gbl_word_vowels[controlInfo->fileIndex][i][j] += controlInfo->word_vowels[i][j];
}

/**
 * \brief Print the obtained results on the console.
 *
 * Operation carried out by the dispatcher.
 *
 * @return EXIT_SUCCESS if it can print and save in disk, EXIT_FAILURE otherwise.
 */
int write_results() {
    for (int fi = 0; fi < num_files; fi++) {
        printf("\nResults for file: %s\n\n", filenames[fi]);
        printf("Total number of words = %d;\n\n", gbl_total_num_words[fi]);

        printf("%2s", " ");

        for (int i = 0; i < gbl_max_word_length[fi]; i++) {
            printf("%6d", i+1);
        }

        printf( "\n");
        printf("%2s", " ");

        for (int i = 0; i < gbl_max_word_length[fi]; i++)
            printf("%6d", gbl_word_lengths[fi][i]);

        printf("\n");
        printf("%2s", "");

        for (int i = 0; i < gbl_max_word_length[fi]; i++)
            printf("%6.2f", (double) gbl_word_lengths[fi][i] / gbl_total_num_words[fi] * 100);

        printf("\n");

        for (int i = 0; i <= gbl_max_word_length[fi]; i++) {
            printf("%2d", i);
            if (i > 1) {
                printf("%*s", 6 * (i - 1), "");
            }
            for (int k = i - 1; k < gbl_max_word_length[fi]; k++) {
                if (k == -1)
                    k = 0;
                if (gbl_word_lengths[k] != 0 && gbl_word_vowels[fi][i][k] != 0) {
                    printf("%6.1f", (double) gbl_word_vowels[fi][i][k] / gbl_word_lengths[fi][k] * 100);
                }
                else {
                    printf("%6.1f", 0.0);
                }
            }
            printf("\n");
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
