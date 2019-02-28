#include <stdbool.h>
#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FLAG_BLOCK_INDEPENDENCE(flags) (flags & 0x20)
#define FLAG_CONTENT_SIZE(flags) (flags & 0x08)
#define FLAG_DICTIONARY_ID(flags) (flags & 0x01)
	// bool flag_block_independence = flags & 0x20;
	// bool flag_block_checksum = flags & 0x10;
	// bool flag_content_size = flags & 0x08;
	// bool flag_content_checksum = flags & 0x04;
	// bool flag_dictionary_id = flags & 0x01;

void decompress_lz4(uint8_t *dst, uint8_t *src){
	if(memcmp(src, "\x04\x22\x4D\x18", 4) != 0){
		printf("Error: Invalid FOURCC in header.\n");
		return;
	}
	src += 4;
	
	uint8_t flags = src[0];
	uint8_t bd_byte = src[1];
	src += 2;
	
	if((flags & 0xC0) != 0x40){
		printf("Error: Invalid LZ4 version number.\n");
		return;
	}
	
	// Reserved bits must always be 0.
	if((flags & 0x02) || (bd_byte & 0x00)){
		printf("Error: Invalid reserved bits state\n");
		return;
	}
	
	// Read content size.
	uint64_t content_size = 0;
	if(FLAG_CONTENT_SIZE(flags)){
		memcpy(&content_size, src, 8);
		src += 8;
		
		// printf("Content size: %d\n", content_size);
	}
	
	// Read dictionary ID.
	uint32_t dictionary_id = 0;
	if(FLAG_DICTIONARY_ID(flags)){
		memcpy(&dictionary_id, src, 4);
		src += 4;
		
		printf("Dictionary IDs not implemented.\n");
		return;
	}
	
	uint8_t header_checksum = src[0];
	src += 1;
	
	uint32_t block_size = 0;
	memcpy(&block_size, src, 4);
	src += 4;
	
	if(block_size & 0x80000000){
		printf("Block is uncompressed and this is NYI...\n");
		return;
	} else {
		// printf("block size: %d\n", block_size);
	}
	
	while(true){
		uint8_t token = *(src++);
		// printf("token is 0x%02X\n", token);
		
		// Decode the literal run length.
		size_t len = (token >> 4) & 0xF;
		if(len == 15) do len += *src; while(*(src++) == 255);
		// printf("literals: %d\n", len);
		
		// Copy literals.
		for(size_t i = 0; i < len; i++) dst[i] = src[i];
		src += len, dst += len;
		
		// Get the backref offset.
		uint16_t offset = 0;
		memcpy(&offset, src, 2);
		src += 2;
		// printf("backref offset: %d\n", offset);
		
		// Done if offset == 0
		if(offset == 0) break;
		
		// Decode backref run length.
		len = (token & 0xF) + 4;
		if(len == 19) do len += *src; while(*(src++) == 255);
		// printf("backref len: %d\n", len);
		
		// Copy backref.
		for(size_t i = 0; i < len; i++) dst[i] = dst[i - offset];
		dst += len;
	}
}

uint8_t SRC[1146627];
uint8_t DST[2493109];

int main(int argc, char *argv[]){
	FILE *f = fopen("words.lz4", "r");
	fread(SRC, 1, sizeof(SRC), f);
	
	decompress_lz4(DST, SRC);
	printf("%s", DST);
	
	return EXIT_SUCCESS;
}
