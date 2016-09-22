#include "utfconverter.h"

#define calc_time_diff ((float)afterUsage.ru_stime.tv_usec - (float)beforeUsage.ru_stime.tv_usec)

char* srcFilename;
char* convFilename;
endianness srcEndian;
endianness convEndian;
encoding srcEncoding;
encoding convEncoding;

int verbosityLevel;
double readingTimes[3];
double convertingTimes[3];
double writingTimes[3];
int glyphCount;
int asciiCount;
int surrogateCount;

int main(int argc, char** argv) { /**  */
	/* After calling parse_args(), filename, convEndian, and convEncoding
	 should be set. */
	struct timeval timeBefore, timeAfter;
	struct rusage beforeUsage, afterUsage, startUsage;
	getrusage(RUSAGE_SELF, &startUsage);

 	parse_args(argc, argv); 
	convFilename = "output.txt";
	convEndian = BIG;
	convEncoding = UTF_16;
	verbosityLevel = LEVEL_2;
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

	/** Read BOM */
	getrusage(RUSAGE_SELF, &beforeUsage);
	gettimeofday(&timeBefore, NULL);
	
	if (!read_bom(&srcFD)) {
		free(glyph);
		fprintf(stderr, "File has no BOM.\n");
		quit_converter(srcFD, NO_FD, EXIT_FAILURE);
	}

	getrusage(RUSAGE_SELF, &afterUsage);
	readingTimes[SYS] += calc_time_diff;
	readingTimes[USER] += calc_time_diff;
	gettimeofday(&timeAfter, NULL);
	readingTimes[REAL] += calc_time_diff;

	/** Write BOM to output */
	getrusage(RUSAGE_SELF, &beforeUsage);
	gettimeofday(&timeBefore, NULL);
	
	if (!write_bom(convFD)) {
		free(glyph);
		fprintf(stderr, "Error writing BOM.\n");
		quit_converter(srcFD, convFD, EXIT_FAILURE);
	}

	getrusage(RUSAGE_SELF, &afterUsage);
	writingTimes[SYS] += calc_time_diff;
	writingTimes[USER] += calc_time_diff;
	gettimeofday(&timeAfter, NULL);
	writingTimes[REAL] += calc_time_diff;
	
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
		getrusage(RUSAGE_SELF, &beforeUsage);
		gettimeofday(&timeBefore, NULL);

		if (srcEncoding == UTF_8) {
			read_utf_8(srcFD, glyph, buf);
		} else {
			read_utf_16(srcFD, glyph, buf);
 		}

		getrusage(RUSAGE_SELF, &afterUsage);
		readingTimes[SYS] += calc_time_diff;
		readingTimes[USER] += calc_time_diff;;
		gettimeofday(&timeAfter, NULL);
		readingTimes[REAL] += calc_time_diff;;
		
		/** Convert */
		getrusage(RUSAGE_SELF, &beforeUsage);
		gettimeofday(&timeBefore, NULL);

		if (convEncoding != srcEncoding) {
			convert_encoding(glyph);
		}

		if (convEndian != LITTLE) {
			swap_endianness(glyph);
		}

		getrusage(RUSAGE_SELF, &afterUsage);
		convertingTimes[SYS] += calc_time_diff;
		convertingTimes[USER] += calc_time_diff;
		gettimeofday(&timeAfter, NULL);
		convertingTimes[REAL] += calc_time_diff;

		/** Write */
		gettimeofday(&timeBefore, NULL);

		write_glyph(glyph, convFD);

		getrusage(RUSAGE_SELF, &afterUsage);
		writingTimes[SYS] += calc_time_diff;
		writingTimes[USER] += calc_time_diff;
		gettimeofday(&timeAfter, NULL);
		writingTimes[REAL] += calc_time_diff;
		
		/** Counts */
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


	print_verbosity(srcFD, startUsage);

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
	quit_converter(srcFD, convFD, EXIT_SUCCESS);
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
		quit_converter(fd, NO_FD, EXIT_FAILURE);
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
			quit_converter(fd, NO_FD, EXIT_FAILURE);
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
		quit_converter(fd, NO_FD, EXIT_FAILURE);
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
			quit_converter(fd, NO_FD, EXIT_FAILURE);
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
	convEncoding = 2;
	convEndian = 2;
	static struct option long_options[] = {
		{"help", optional_argument, 0, 'h'},
		{"UTF=", required_argument, 0, 'u'},
		{NULL, optional_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
		/** TODO - ADD TO ME */
	int option_index;
	char c;

	while ((c = getopt_long(argc, argv, "hu:v", long_options, &option_index)) 
			!= -1) {
		switch(c) { 
			case 'u':
				if (optarg != NULL) {
					if (strcmp(optarg, "16LE") == 0) {
						convEncoding = UTF_16;
						convEndian = LITTLE;
					} else if (strcmp(optarg, "16BE") == 0) {
						convEncoding = UTF_16;
						convEndian = BIG;
					} else if (strcmp(optarg, "8") == 0) {
						convEncoding = UTF_8;
						convEndian = LITTLE;
					} else {
						print_help(EXIT_FAILURE);
					}
				}
				break;
			case 'h':
				print_help(EXIT_SUCCESS);
			case 'v':
				if (verbosityLevel == LEVEL_0) {
					verbosityLevel = LEVEL_1;
				} else {
					verbosityLevel = LEVEL_2;
				}
				break;
			case '?':
				print_help(EXIT_FAILURE);
		}
	}

	if(optind < argc) {
		//if ()
		strcpy(srcFilename, argv[optind]);
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help(EXIT_FAILURE);
	}

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */

	if(convEncoding == 2) {
		fprintf(stderr, "Converson mode not given.\n");
		print_help(EXIT_FAILURE);
	}
	return 1;
}

void print_help(int exit_status) {
	printf("%s", USAGE); 
	quit_converter(NO_FD, NO_FD, exit_status);
}

void print_verbosity(int fd, struct rusage usage) {
	if (verbosityLevel == LEVEL_0) {
		return;
	}
	/** Level 1 + 2 */
	int fileSize = lseek(fd, 0, SEEK_END) / 1000;
	fprintf(stderr, "Input file size: %d kb\n", fileSize);

	

	fprintf(stderr, "Input file encoding: ");
	if (srcEncoding == UTF_8) {
			fprintf(stderr, "UTF-8\n");
	} else {
		if (srcEndian == LITTLE) {
			fprintf(stderr, "UTF-16LE");
		else {
			fprintf(stderr, "UTF-16BE");
		}
	}

	fprintf(stderr, "Reading:\tReal: %f\n\tUser: %f\n\tSys: %f\n\n", 
	readingTimes[REAL], readingTimes[USER], readingTimes[SYS]);
	fprintf(stderr, "Writing:\tReal: %f\n\tUser: %f\n\tSys: %f\n\n", 
	writingTimes[REAL], writingTimes[USER], writingTimes[SYS]);
	fprintf(stderr, "Reading:\tReal: %f\n\tUser: %f\n\tSys: %f\n\n", 
	readingTimes[REAL], readingTimes[USER], readingTimes[SYS]);
	fd++;
	char hostname[101];
	hostname[100] = '\0';
	gethostname(hostname, sizeof(hostname));
	
	char pwd[301];
	pwd[300] = '\0';
	getcwd(pwd, sizeof(pwd));
	getrusage(RUSAGE_SELF, &usage);
}

void quit_converter(int srcFD, int convFD, int exit_status) {
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if (srcFD != NO_FD)
		close(srcFD);
	if (convFD != NO_FD)
		close(convFD);
	exit(exit_status);
}