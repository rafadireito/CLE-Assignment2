/**
 *  \file worker.c
 *
 *  \brief Problem: Frequency of word lengths and the number of vowels
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
#include <string.h>

/**
 * \brief Check if a given character, in hexadecimal, is a vowel.
 *
 * @param character_hex pointer that contains the code that define the character in hexadecimal.
 * @return 1 if the character it's a vowel, 0 otherwise.
 */
int check_vowel(const char* character_hex) {
    char *vowels_hex[] = {"61", "65", "69", "6f", "75", "a0", "a1", "a2", "a3", "a8", "a9", "aa", "ac", "ad", "b2",
                          "b3", "b4", "b5", "b9", "ba", "80", "81", "82", "83", "88", "89", "8a", "8c", "8d", "92",
                          "93", "94", "95", "99", "9a", "bc"};

    for (int i = 0; i < sizeof(vowels_hex) / sizeof(vowels_hex[0]); i++)
        if (strcmp(character_hex, vowels_hex[i]) == 0)
            return 1;

    return 0;
}

/**
 * \brief Check if a given character, in hexadecimal, is one of the characters which are considered to split words.
 *
 * @param character_hex pointer that contains the code that define the character in hexadecimal.
 * @return 1 if the character it's a split character, 0 otherwise.
 */
int is_split_char(const char* character_hex) {
    char *split_tokens_hex[] = {"20", "9", "a", "2d", "22", "9c", "9d", "5b", "5d", "7b", "7d", "28", "29", "2e", "2c",
                                "3a", "3b", "3f", "21", "93", "a6", "c2", "ab", "bb", "60", "94"};

    for (int i = 0; i < sizeof(split_tokens_hex) / sizeof(split_tokens_hex[0]); i++)
        if (strcmp(character_hex, split_tokens_hex[i]) == 0)
            return 1;

    return 0;
}

/**
 * \brief Process the K tokens retrieved from the current open file.
 *
 * Construct words with the caracters of the tokens, count every vowel found in them, as well as their lengths.
 *
 * @param controlInfo contains all the info needed to compute the results expected from a worker
 */
void process_data(ControlInfo *controlInfo) {
    char hex[2];
    unsigned char chr;
    int vowel_potential = 0;
    int quotation_potential = 0;
    int word_length = 0;
    int num_vowels = 0;

    // Start variables
    controlInfo->max_word_length = 0;
    controlInfo->max_num_vowels = 0;
    controlInfo->num_words_read = 0;

    for (int i = 0; i < controlInfo->n_chars_read; i++) {
        // Get the next character in the file.
        chr = controlInfo->chars_read[i];
        // Converts the character to hexadecimal.
        sprintf(hex, "%x", tolower(chr));

        // If the hexadecimal of the character is c3, then it can be a vowel.
        if (strcmp(hex, "c3") == 0)
            vowel_potential = 1;
            // If the hexadecimal of the character is e2, then it can be single quotation marks, which merges words.
        else if (strcmp(hex, "e2") == 0)
            quotation_potential = 1;

        // If the hexadecimal isn't a vowel or a single quotation mark, and is one of the characters that splits
        // words, then we have a new word.
        if (!vowel_potential && !quotation_potential && is_split_char(hex)) {
            if (word_length > 0) {
                if (word_length > controlInfo->max_word_length)
                    controlInfo->max_word_length = word_length;

                if (num_vowels > controlInfo->max_num_vowels)
                    controlInfo->max_num_vowels = num_vowels;

                controlInfo->word_lengths[word_length - 1] += 1;
                controlInfo->word_vowels[num_vowels][word_length - 1] += 1;
                controlInfo->num_words_read +=1 ;

                num_vowels = 0;
                word_length = 0;
            }
        } else {
            if (strcmp(hex, "c3") != 0 && strcmp(hex, "e2") != 0) {
                if (!quotation_potential && strcmp(hex, "27") != 0 && strcmp(hex, "98") != 0) {
                    if ((vowel_potential && strcmp(hex, "99") == 0) || strcmp(hex, "99") != 0) {
                        word_length += 1;
                        num_vowels += check_vowel(hex);
                    }
                }
                if (strcmp(hex, "80") != 0)
                    quotation_potential = 0;
                vowel_potential = 0;
            }
        }
    }
}