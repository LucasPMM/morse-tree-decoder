#ifndef MORSE_REFERENCE_H
#define MORSE_REFERENCE_H

/* Independent ITU-R M.1677-1 alphanumeric oracle; never derived from data/morse.txt. */
typedef struct {
    char symbol;
    const char *code;
} ReferenceEntry;

static const ReferenceEntry reference[] = {
    {'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},   {'E', "."},     {'F', "..-."},
    {'G', "--."},   {'H', "...."},  {'I', ".."},    {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
    {'M', "--"},    {'N', "-."},    {'O', "---"},   {'P', ".--."},  {'Q', "--.-"},  {'R', ".-."},
    {'S', "..."},   {'T', "-"},     {'U', "..-"},   {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},  {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
    {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}};

#define REFERENCE_COUNT (sizeof(reference) / sizeof(reference[0]))

#endif
