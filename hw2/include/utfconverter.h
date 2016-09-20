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
	int nBytes;
	bool surrogate;
} Glyph;

/** The given filename. */
extern char* filename;

/** The usage statement. */
const char* const USAGE = 
"Command line uility for converting UTF files to and from UTF-8, UTF-16LE\n\
and UTF-16BE.\n\n\
Usage:\n\
  ./utf [-h|--help] -u OUT_ENC | --UTF=OUT_ENC IN_FILE [OUT_FILE]\n\n\
  Option arguments:\n\
    -h, --help\t    Displays this usage.\n\
    -v, -vv\t    Toggles the verbosity of the program to level 1 or 2.\n\n\
  Mandatory arguments:\n\
    -u OUT_ENC, --UTF=OUT_ENC\tSets the output encoding.\n\
\t\t\t\tValid values for OUT_ENC: 8, 16LE, 16BE\n\n\
  Positional Arguments:\n\
    IN_FILE\t    The file to convert.\n\
    [OUT_FILE]\t    Output file name. If not present, defaults to stdout.\n";

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
* Converts the encoding of a glyph to the other type
*
* @param glyph The pointer to the glyph struct to convert
*/
void convert_encoding P((Glyph*));

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
* Reads the source file as UTF 8 and stores the passes the unicode to fill_glyph
*
* @param fd	The int pointer to the file descriptor of the input
* @return Returns the filled glyph
*/
Glyph* read_utf_8 P((int fd, Glyph* glyph, unsigned int* buf));

/**
* Reads the sfdource file as UTF 16 and stores the passes the unicode to fill_glyph
*
* @param fd	The int pointer to the file descriptor of the input
* @return Returns the success of reading the source file and storing its unicode
*/
Glyph* read_utf_16 P((int fd, Glyph* glyph, unsigned int* buf));

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
 * @param fd The file descriptor for output
 */
void write_glyph P((Glyph*, int));

/**
 * Calls getopt() and parses arguments.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as an array of string.
 * @return Returns the success of parsing the args
 */
int parse_args P((int, char**));

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
void quit_converter P((int, int));

#endif
