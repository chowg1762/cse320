#include "utfconverter.h"

char* srcFilename;
char* convFilename;
endianness srcEndian;
endianness convEndian;
encoding srcEncoding;
encoding convEncoding;

int main(int argc, char** argv) { 
	/* After calling parse_args(), filename, convEndian, and convEncoding
	 should be set. */
	parse_args(argc, argv);

	int srcFD = open("rsrc/utf16BE-special.txt", O_RDONLY);
	int convFD = open(convFilename, O_WRONLY);
	unsigned int* buf = malloc(sizeof(int));
	*buf = 0;
	Glyph* glyph = malloc(sizeof(Glyph)); 

	//Read BOM
	if (!read_bom(&srcFD)) {
		free(glyph);
		fprintf(stderr, "File has no BOM.\n");
		quit_converter(srcFD, NO_FD);
	}

	// Write BOM to output
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

	// Read source file and create glyphs
	while (read(fd, buf, 1) == 1) {
		memset(glyph, 0, sizeof(Glyph));
		if (srcEncoding == UTF_8) {
			read_utf_8(srcFD, glyph, buf));
		} else if (srcEncoding == UTF_16) {
			read_utf_16(srcFD, glyph, buf);
 		}
		if (srcEncoding != convEncoding) {
			convert_encoding(glyph);
		}
		if (srcEndian != convEndian) {
			swap_endianness(glyph);
		}
		write_glyph(glyph);
		*buf = 0;
	}

	/* Now deal with the rest of the bytes.*/
	while((rv = read(fd, &buf[0], 1)) == 1 &&  
	(rv = read(fd, &buf[1], 1)) == 1) {
		write_glyph(fill_glyph(glyph, buf, srcEndian, &fd));
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
	}
	free(buf);
	free(glyph);
	quit_converter(srcFD, convFD);
	return 0;
}

void convert_encoding(Glyph* glyph) {
	unsigned int unicode = 0, i, j;
	// Find unicode value of UTF 8
	if (srcEncoding == UTF_8) {
		if (glyph->nBytes == 1) {
			unicode = glyph->bytes[0];
		} else {
			int shamt = (glyph->nBytes + 1);
			unicode = (glyph->bytes[0] << shamt) >> shamt;
			for (i = 1; i < glyph->nBytes; ++i) {
				unicode <<= 8;
				unicode += (glyph->bytes[i] << 2) >> 2;
			}
		}
	} 
	// Find unicode value of UTF 16 
	else {
		if (!glyph->surrogate) {
			unicode = glyph->bytes[0] + (glyph->bytes[1] << 8);
		} else {
			int data[4] = {0, 0, 0, 0};
			data
		}
	}
	// Use unicode to make new encoding
	// Unicode -> UTF 8
	if (convEncoding == UTF_8) {

	} 
	// Unicode -> UTF 16
	else {
		// Surrogate pair
		if (unicode > 0x10000) {
			unicode -= 0x10000;
			unsigned int msb = (unicode << 10) + 0xD800;
			unsigned int lsb = (unicode & 0x3FF) + 0xDC00;
			unicode = (msb << 10) + lsb;
			for (i = 0; i < 4; ++i) {
				glyph->bytes[i] = (unicode >> (i * 8)) << ((3 - i) * 8) 
				>> ((3 - i) * 8);
			}
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

Glyph* read_utf_8(int fd, Glyph *glyph, int *buf) {
	glyph->bytes[0] = *buf;
	int i;
	// 1 Byte?
	if (*buf >> 7 == 0) {
		glyph->nBytes = 1;
	}
	// 2 Bytes?
	else if (*buf >> 5 == 0x6) {
		glyph->nBytes = 2;
	} 
	// 3 Bytes?
	else if (*buf >> 4 == 0xD) {
		glyph->nBytes = 3;
	}
	// 4 Bytes?
	else if (*buf >> 3 == 0x1D) {
		glyph->nBytes = 4;
	} else {
		fprintf(stderr, "Encountered an invalid UTF 8 character");
		free(buf);
		free(glyph);
		quit_converter(fd, NO_FD);
	}
	// Read the bytes
	for (i = 0; i < glyph->nBytes; ++i) {
		*buf = 0;
		if (read(fd, buf, 1) && (*buf >> 6 == 2)) {
			glyph->bytes[i + 1] = *buf;
		} else {
			fprintf(stderr, "Encountered an invalid UTF 8 character");
			free(buf);
			free(glyph);
			quit_converter(fd, NO_FD);
		}
	}
	return glyph;
}

Glyph* read_utf_16(int fd, Glyph* glyph, int *buf) {
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
	unsigned int temp = (glyph->[0] + (glyph->bytes[1] << 8));
	// Check if this character is a surrogate pair
	if (temp >= 0xD800 && temp <= 0xDBFF) {
		if (read(fd, glyph->bytes[2], 1) == 1 && 
		read(fd, glyph->bytes[3], 1)) {
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
	}
	return 0;
}

int write_bom(int fd) { 
	int buf[3], nBytes;
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
		{"help", optional_argument, NULL, 'h'},
		{"verbosity_level_1", optional_argument, NULL, 'v'},
		{"verbosity_level_2", optional_argument, NULL, 'z'},
		{"UTF", required_argument, NULL, 'u'},
		{0, 0, 0, 0}
		};
		// TODO - ADD TO ME
	int option_index, c;
	char* endian_convert = NULL;
	

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	if((c = getopt_long(argc, argv, "hu", long_options, &option_index)) 
			!= -1) {
		switch(c) { 
			case 'u':
				endian_convert = optarg;
			default:
				return 0;
		}

	}

	if(optind < argc){
		strcpy(filename, argv[optind]);
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
	for(int i = 0; i < 4; i++){
		printf("%s", USAGE[i]); 
	}
	quit_converter(NO_FD, NO_FD);
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
