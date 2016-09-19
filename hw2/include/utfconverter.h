#ifndef UTF_CONVERTER
#define UTF_CONVERTER

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>


#define MAX_BYTES 4
#define SURROGATE_SIZE 4
#define NON_SURROGATE_SIZE 2
#define NO_FD -1
#define OFFSET 2 // was 4

#define FIRST  10000000
#define SECOND 20000000
#define THIRD  30000000
#define FOURTH 40000000

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

/** The enum for endianness. */
typedef enum {LITTLE, BIG} endianness;

/** The enum for encoding. */
typedef enum {UTF_8, UTF_16} encoding;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	unsigned char bytes[MAX_BYTES];
	bool surrogate;
} Glyph;

/** The given filename. */
extern char* filename;

/** The usage statement. */
const char* const USAGE[] = { 
	"Usage:  ./utfconverter FILENAME [OPTION]\n\t",
	"./utfconverter -h\t\t\tDisplays this usage statement.\n\t",
	"./utfconverter --help\t\t\tDisplays this usage statement.\n\t"
	"./utfconverter --UTF-16=ENDIANNESS\tEndianness to convert to.\n",
};

/** Which endianness to convert to. */
extern endianness convEndian;

/** Which endianness the source file is in. */
extern endianness srcEndian;

/** Which encoding the source file is in. */
extern encoding srcEncoding;

/** Which encoding to convert to */
extern encoding convEncoding;

/**
* Reads the BOM of the passed file and sets global endianness and encoding
*
* @param fd The file descriptor
* @return Returns whether or not the file has a valid BOM
*/
int read_bom P((int*));

/**
* Writes the BOM of the designated encoding to the output file
*
* @return Returns the success of writing to output file
*/
int write_bom P(());

/**
 * A function that swaps the endianness of the bytes of an encoding from
 * LE to BE and vice versa.
 *
 * @param glyph The pointer to the glyph struct to swap.
 * @return Returns a pointer to the glyph that has been swapped.
 */
Glyph* swap_endianness P((Glyph*));

/**
* A function that converts a UTF-8 glyph to a UTF-16LE or UTF-16BE
* glyph, and returns the result as a pointer to the converted glyph.
*
* @param glyph The UTF-8 glyph to convert.
* @param end   The endianness to convert to (UTF-16LE or UTF-16BE).
* @return The converted glyph
*/
Glyph* convert P((Glyph* glyph, endianness end));

/**
 * Fills in a glyph with the given data in data[2], with the given endianness 
 * by end.
 *
 * @param glyph 	The pointer to the glyph struct to fill in with bytes.
 * @param data[2]	The array of data to fill the glyph struct with.
 * @param end	   	The endianness enum of the glyph.
 * @param fd 		The int pointer to the file descriptor of the input 
 * 			file.
 * @return Returns a pointer to the filled-in glyph.
 */
Glyph* fill_glyph P((Glyph*, unsigned int*, endianness, int*));

/**
 * Writes the given glyph's contents to stdout.
 *
 * @param glyph The pointer to the glyph struct to write to stdout.
 */
void write_glyph P((Glyph*));

/**
 * Calls getopt() and parses arguments.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as an array of string.
 */
void parse_args P((int, char**));

/**
 * Prints the usage statement.
 */
void print_help P((void));

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int));

#endif
