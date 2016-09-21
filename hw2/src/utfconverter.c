#include "utfconverter.h"
#include <time.h>
#include <sys/time.h>

char* srcFilename;
char* convFilename;
endianness srcEndian;
endianness convEndian;
encoding srcEncoding;
encoding convEncoding;

int verbosityLevel;
float readingTimes[3];
float convertingTimes[3];
float writingTimes[3];
int glyphCount;
int asciiCount;
int surrogateCount;

int main() { /** int argc, char** argv */
	/* After calling parse_args(), filename, convEndian, and convEncoding
	 should be set. */
	/** parse_args(argc, argv); */
	convFilename = "output.txt";
	convEndian = BIG;
	convEncoding = UTF_16;
	int srcFD = open("rsrc/utf8-special.txt", O_RDONLY);
	int convFD = open(convFilename, O_WRONLY);
	unsigned int* buf = malloc(sizeof(int));
	*buf = 0;
	Glyph* glyph = malloc(sizeof(Glyph));

	memset(readingTimes, 0, sizeof(readingTimes));
	memset(convertingTimes, 0, sizeof(convertingTimes));
	memset(writingTimes, 0, sizeof(writingTimes));
	glyphCount = 0;
    asciiCount = 0;
	surrogateCount = 0;
	struct timeval timeBefore, timeAfter;

	/** Read BOM */
	if (!read_bom(&srcFD)) {
		free(glyph);
		fprintf(stderr, "File has no BOM.\n");
		quit_converter(srcFD, NO_FD);
	}

	/** Write BOM to output */
	if (!write_bom(convFD)) {
		free(glyph);
		fprintf(stderr, "Error writing BOM.\n");
		quit_converter(srcFD, convFD);
	}
	
	void* memset_return = memset(glyph, 0, sizeof(Glyph));
	/* Memory write failed, recover from it: */
	if(memset_return == NULL) {
		/* tweak write permission on heap memory. */
		asm("movl $8, %esi\n\t"
			"movl $.LC0, %edi\n\t"
			"movl $0, %eax");
		/* Now make the request again. */
		memset(glyph, 0, sizeof(Glyph));
	}

	/* Read source file and create glyphs */
	while (read(srcFD, buf, 1) == 1) {
		memset(glyph, 0, sizeof(Glyph));

		/* Read */
		gettimeofday(&timeBefore, NULL);
		if (srcEncoding == UTF_8) {
			read_utf_8(srcFD, glyph, buf);
		} else {
			read_utf_16(srcFD, glyph, buf);
 		}
		gettimeofday(&timeAfter, NULL);
		readingTimes[REAL] +=  (timeAfter.tv_sec - timeBefore.tv_sec) +
		(timeAfter.tv_usec - timeBefore.tv_usec);

		/** Convert */
		gettimeofday(&timeBefore, NULL);
		if (convEncoding != srcEncoding) {
			convert_encoding(glyph);
		}

		if (convEndian != LITTLE) {
			swap_endianness(glyph);
		}
		convertingTimes[REAL] +=  (timeAfter.tv_sec - timeBefore.tv_sec) +
		(timeAfter.tv_usec - timeBefore.tv_usec);

		/** Write */
		gettimeofday(&timeBefore, NULL);
		write_glyph(glyph, convFD);
		writingTimes[REAL] +=  (timeAfter.tv_sec - timeBefore.tv_sec) +
		(timeAfter.tv_usec - timeBefore.tv_usec);

		++glyphCount;
		if (glyph->surrogate) {
			++surrogateCount;
		} else {
			if (glyph->bytes[1] == 0 && glyph->bytes[2] == 0 
			&& glyph->bytes[3] == 0) {
				++asciiCount;
			}
		}
		*buf = 0;
	}


	print_verbosity(srcFD);

	/* Now deal with the rest of the bytes. 
	while((rv = read(fd, &buf[0], 1)) == 1 &&  
	(rv = read(fd, &buf[1], 1)) == 1) {
		write_glyph(fill_glyph(glyph, buf, srcEndian, &fd));
		void* memset_return = memset(glyph, 0, sizeof(Glyph)); 
	     Memory write failed, recover from it: 
	    if(memset_return == NULL) {
		     tweak write permission on heap memory. 
	       	asm("movl $8, %esi\n\t"
		    "movl $.LC0, %edi\n\t"
	        "movl $0, %eax");
	         Now make the request again. 
		        memset(glyph, 0, sizeof(Glyph));
	    }
	} */
	free(buf);
	free(glyph);
	quit_converter(srcFD, convFD);
	return 0;
}

void convert_encoding(Glyph* glyph) {
	unsigned int unicode = 0;
	int i, mask;
	/** Find unicode value of UTF 8 */
	if (srcEncoding == UTF_8) {
		if (glyph->nBytes == 1) {
			unicode = glyph->bytes[0];
		} else {
			mask = 0;
			for (i = 0; i < 7 - glyph->nBytes; ++i) {
				mask <<= 1;
				++mask;
			}
			unicode = glyph->bytes[0] & mask;
			mask = 0x3F;
			for (i = 1; i < glyph->nBytes; ++i) {
				unicode <<= 6;
				unicode += glyph->bytes[i] & mask;
			}
		}
	} 
	/** Find unicode value of UTF 16 */ 
	else {
		if (!glyph->surrogate) {
			unicode = glyph->bytes[0] + (glyph->bytes[1] << 8);
		} else {
			/** TODO - Extra credit */
		}
	}
	memset(glyph, 0, 4);
	/** Use unicode to make new encoding
	* Unicode -> UTF 8 */
	if (convEncoding == UTF_8) {
		/** TODO - Extra credit */
	} 
	/** Unicode -> UTF 16 */
	else {
		/** Surrogate pair */
		if (unicode > 0x10000) {
			unicode -= 0x10000;
			unsigned int msb = (unicode >> 10) + 0xD800;
			unsigned int lsb = (unicode & 0x3FF) + 0xDC00;
			unicode = (msb << 16) + lsb;
			mask = 0xFF;
			for (i = 0; i < 4; ++i) {
				glyph->bytes[(i < 2)? i + 2 : i - 2] = 
				(unicode & mask) >> (i * 8);
				mask <<= 8;
			}
			glyph->surrogate = true;
		} else {
			mask = 0xFF;
			glyph->bytes[0] = unicode & mask;
			mask = 0xFF00;
			glyph->bytes[1] = (unicode & mask) >> 8;
			glyph->bytes[2] = glyph->bytes[3] = 0;
		}
	}
}

Glyph* swap_endianness(Glyph* glyph) {
	/* Use XOR to be more efficient with how we swap values. */
	glyph->bytes[0] ^= glyph->bytes[1];
	glyph->bytes[1] ^= glyph->bytes[0];
	glyph->bytes[0] ^= glyph->bytes[1];
	if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
		glyph->bytes[2] ^= glyph->bytes[3];
		glyph->bytes[3] ^= glyph->bytes[2];
		glyph->bytes[2] ^= glyph->bytes[3];
	}
	return glyph;
}

Glyph* read_utf_8(int fd, Glyph *glyph, unsigned int *buf) {
	glyph->bytes[0] = *buf;
	int i;
	/** 1 Byte? */
	if (*buf >> 7 == 0) {
		glyph->nBytes = 1;
	}
	/** 2 Bytes? */
	else if (*buf >> 5 == 0x6) {
		glyph->nBytes = 2;
	} 
	/** 3 Bytes? */
	else if (*buf >> 4 == 0xE) {
		glyph->nBytes = 3;
	}
	/** 4 Bytes? */
	else if (*buf >> 3 == 0x1E) {
		glyph->nBytes = 4;
	} else {
		fprintf(stderr, "Encountered an invalid UTF 8 character.\n");
		free(buf);
		free(glyph);
		quit_converter(fd, NO_FD);
	}
	/** Read the bytes */
	for (i = 1; i < glyph->nBytes; ++i) {
		*buf = 0;
		if (read(fd, buf, 1) && (*buf >> 6 == 2)) {
			glyph->bytes[i] = *buf;
		} else {
			fprintf(stderr, "Encountered an invalid UTF 8 character.\n");
			free(buf);
			free(glyph);
			quit_converter(fd, NO_FD);
		}
	}
	return glyph;
}

Glyph* read_utf_16(int fd, Glyph* glyph, unsigned int *buf) {
	glyph->bytes[0] = *buf;
	if (read(fd, buf, 1) != 1) {
		fprintf(stderr, "Error reading file.");
		free(buf);
		free(glyph);
		quit_converter(fd, NO_FD);
	}
	if (srcEndian == LITTLE) {
		glyph->bytes[1] = *buf;
	} else {
		glyph->bytes[1] = glyph->bytes[0];
		glyph->bytes[0] = *buf;
	}
	unsigned int temp = (glyph->bytes[0] + (glyph->bytes[1] << 8));
	/** Check if this character is a surrogate pair */
	if (temp >= 0xD800 && temp <= 0xDBFF) {
		if (read(fd, &glyph->bytes[2], 1) == 1 && 
		read(fd, &glyph->bytes[3], 1)) {
			if (srcEndian == LITTLE) {
				temp = glyph->bytes[2] + (glyph->bytes[3] << 8);
			} else {
				temp = glyph->bytes[3] + (glyph->bytes[2] << 8);
			}
			if (temp >= 0xDC00 && temp <= 0xDFFF) {
				glyph->surrogate = true;
			} else {
				lseek(fd, -OFFSET, SEEK_CUR);
				glyph->surrogate =false;
			}
		} else {
			fprintf(stderr, "Error reading file.");
			free(buf);
			free(glyph);
			quit_converter(fd, NO_FD);
		}
	} else {
		glyph->surrogate = false;
	}
	if (!glyph->surrogate) {
		glyph->bytes[2] = glyph->bytes[3] = 0;
	} else if (srcEndian == BIG) {
		*buf = glyph->bytes[2];
		glyph->bytes[2] = glyph->bytes[3];
		glyph->bytes[3] = *buf;
	}
	return glyph;
}

int read_bom(int* fd) {
	unsigned int buf[2] = {0, 0}; 
	int rv = 0;
	if((rv = read(*fd, &buf[0], 1)) == 1 && 
			(rv = read(*fd, &buf[1], 1)) == 1) { 
		if(buf[0] == 0xff && buf[1] == 0xfe) {
			/*file is little endian*/
			srcEndian = LITTLE; 
			srcEncoding = UTF_16;
		} else if(buf[0] == 0xfe && buf[1] == 0xff) {
			/*file is big endian*/
			srcEndian = BIG;
			srcEncoding = UTF_16;
		} else if (buf[0] == 0xef && buf[1] == 0xbb &&
		(rv = read(*fd, &buf[0], 1) == 1) && buf[0] == 0xbf) {
			srcEncoding = UTF_8;
		} else {
			return 0;
		}
		return 1;
	} else {
		fprintf(stderr, "Error reading BOM of source file.\n");
	}
	return 0;
}

int write_bom(int fd) { 
	int nBytes;
	unsigned char buf[3];
	if (convEncoding == UTF_8) {
		nBytes = 3;
		buf[0] = 0xef;
		buf[1] = 0xbb;
		buf[2] = 0xbf;
	} else {
		nBytes = 2;
		if (convEndian == LITTLE) {
			buf[0] = 0xff;
			buf[1] = 0xfe;
		} else {
			buf[0] = 0xfe;
			buf[1] = 0xff;
		}
	}
	if (write(fd, buf, nBytes) == -1) {
		return 0;
	}
	return 1;
}

void write_glyph(Glyph* glyph, int fd) {
	if (convEncoding == UTF_8) {
		write(fd, glyph->bytes, glyph->nBytes);
	} else {
		if(glyph->surrogate) {
			write(fd, glyph->bytes, SURROGATE_SIZE); // STDIN_FILENO
		} else {
			write(fd, glyph->bytes, NON_SURROGATE_SIZE);
		}
	}
}

int parse_args(int argc, char** argv) {
	static struct option long_options[] = {
		{"help", optional_argument, 0, 'h'},
		{"UTF=", required_argument, 0, 'u'},
		{0, 0, 0, 0}
	};
		/** TODO - ADD TO ME */
	int option_index, c;
	char* endian_convert = NULL;
	
	while (c = getopt(argc, argv, "hv:", ) != -1) {
		switch(c) {
			case 'v':
				if (verbosityLevel == LEVEL_0) {
					verbosityLevel = LEVEL_1;
				} else {
					verbosityLevel = LEVEL_2;
				}
				break;
			case 'h':
				print_help();
			default:
				print_help();
		}
	}

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	while ((c = getopt_long(argc, argv, "hu:", long_options, &option_index)) 
			!= -1) {
		switch(c) { 
			case 'u':
				endian_convert = optarg;
				break;
			case 'h':
				print_help();
			default:
				return 0;
		}

	}

	if(optind < argc){
		strcpy(srcFilename, argv[optind]);
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
	}

	if(endian_convert == NULL) {
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
	}

	if(strcmp(endian_convert, "LE")) { 
		convEndian = LITTLE;
	} else if(strcmp(endian_convert, "BE")) {
		convEndian = BIG;
	} else {
		return 0;
	}
	return 1;
}

void print_help(void) {
	printf("%s", USAGE); 
	quit_converter(NO_FD, NO_FD);
}

void print_verbosity(int fd) {
	fd++;
	char hostname[101],;
	hostname[100] = '\0';
	gethostname(&hostname, sizeof(hostname));
	
	char pwd[301];
	pwd[300] = '\0';
	getcwd(pwd, sizeof(pwd));
}

void quit_converter(int srcFD, int convFD) {
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if (srcFD != NO_FD)
		close(srcFD);
	if (convFD != NO_FD)
		close(convFD);
	exit(0);
}