#include "utfconverter.h"

char* filename;
endianness srcEndian;
endianness convEndian;
encoding srcEncoding;
encoding convEncoding;

int main(int argc, char** argv) { 
	/* After calling parse_args(), filename, convEndian, and convEncoding
	 should be set. */
	parse_args(argc, argv);

	int fd = open("rsrc/utf16BE-special.txt", O_RDONLY); 
	unsigned int buf[2] = {0, 0}; 
	int rv = 0; 

	Glyph* glyph = malloc(sizeof(Glyph)); 

	//Read BOM
	if (!read_bom(&fd)) {
		free(glyph);
		fprintf(stderr, "File has no BOM.\n");
		quit_converter(NO_FD);
	}

	// Write BOM to output
	if (!write_bom()) {
		free(glyph);
		fprintf(stderr, "Error writing BOM.\n");
		quit_converter(NO_FD);
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

	if (srcEncoding == UTF_8) {
		
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
	free(glyph);
	quit_converter(NO_FD);
	return 0;
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

Glyph* fill_glyph(Glyph* glyph, unsigned int data[2], 
endianness end, int* fd) {
	// Store as little endian
	if (end == LITTLE) {
		glyph->bytes[0] = data[0];
		glyph->bytes[1] = data[1];
	} else {
		glyph->bytes[0] = data[1];
		glyph->bytes[1] = data[0];
	}
	unsigned int bits = 0; 
	bits = (glyph->bytes[1] + (glyph->bytes[0] << 8)); // Little Endian
	/* Check high surrogate pair using its special value range.*/
	if (bits >= 0xD800 && bits <= 0xDBFF) { 
		if (read(*fd, &data[0], 1) == 1 && 
			read(*fd, &data[1], 1) == 1) {
			bits = 0; /* bits |= (bytes[FIRST] + (bytes[SECOND] << 8)) */
			if (end == LITTLE) {
				bits = (data[0] + (data[1] << 8));
			} else {
				bits = (data[1] + (data[0] << 8));
			}
			if (bits >= 0xDC00 && bits <= 0xDFFF) { /* Check low surrogate pair.*/
				glyph->surrogate = false; 
			} else {
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = true;
			}
		} else {
			// error reading file
		}
	} else {
		glyph->surrogate = false;
	}
	if (!glyph->surrogate) {
		glyph->bytes[2] = glyph->bytes[3] |= 0;
	} else if (end == LITTLE) {
		glyph->bytes[2] = data[0]; 
		glyph->bytes[3] = data[1];
	} else {
		glyph->bytes[2] = data[1]; 
		glyph->bytes[3] = data[0];
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

int write_bom() {
	int fd = open(filename, O_WRONLY); 
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

void write_glyph(Glyph* glyph) {
	int fd = open("output.txt", O_WRONLY);
	if(glyph->surrogate) {
		write(fd, glyph->bytes, SURROGATE_SIZE); // STDIN_FILENO
	} else {
		write(fd, glyph->bytes, NON_SURROGATE_SIZE);
	}
}

void parse_args(int argc, char** argv) {
	static struct option long_options[] = {
		{"help", optional_argument, NULL, 'h'},
		{"verbosity_level_1", optional_argument, NULL, 'v'},
		{"verbosity_level_2", optional_argument, NULL, "vv"},
		{"UTF", required_argument, NULL, 'u'},

		}
		// TODO - ADD TO ME
	};
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
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				break;
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
		quit_converter(NO_FD);
	}
}

void print_help(void) {
	for(int i = 0; i < 4; i++){
		printf("%s", USAGE[i]); 
	}
	quit_converter(NO_FD);
}

void quit_converter(int fd) {
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	exit(0);
}
