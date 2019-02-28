#include <stdbool.h>
#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FLAG_BLOCK_INDEPENDENCE(flags) (flags & 0x20)
#define FLAG_BLOCK_CHECKSUM (flags & 0x10)
#define FLAG_CONTENT_SIZE(flags) (flags & 0x08)
#define FLAG_CONTENT_CHECKSUM (flags & 0x04)
#define FLAG_DICTIONARY_ID(flags) (flags & 0x01)

void decompress_lz4(uint8_t *dst, uint8_t *src){
	// Check FOURCC code.
	assert(memcmp(src, "\x04\x22\x4D\x18", 4) == 0);
	src += 4;
	
	uint8_t flags = src[0];
	uint8_t bd_byte = src[1];
	src += 2;
	
	// Check LZ4 version number is valid.
	assert((flags & 0xC0) == 0x40);
	
	// Reserved bits must always be 0.
	assert((flags & 0x02) == 0 && (bd_byte & 0x00) == 0);
	
	// Read content size.
	uint64_t content_size = 0;
	if(FLAG_CONTENT_SIZE(flags)){
		memcpy(&content_size, src, 8);
		src += 8;
	}
	
	// Read dictionary ID.
	uint32_t dictionary_id = 0;
	if(FLAG_DICTIONARY_ID(flags)){
		memcpy(&dictionary_id, src, 4);
		src += 4;
		
		puts("Dictionary IDs not implemented.");
		abort();
	}
	
	uint8_t header_checksum = src[0];
	src += 1;
	
	uint32_t block_size = 0;
	memcpy(&block_size, src, 4);
	src += 4;
	
	if(block_size & 0x80000000){
		puts("Block is uncompressed and this is NYI.");
		abort();
	}
	
	size_t src_cursor = 0;
	size_t dst_cursor = 0;
	
	while(true){
		uint8_t token = src[src_cursor++];
		
		// Decode the literal run length.
		size_t len = (token >> 4) & 0xF;
		if(len == 15) do len += src[src_cursor]; while(src[src_cursor++] == 255);
		
		// Copy literals.
		for(size_t i = 0; i < len; i++) dst[dst_cursor + i] = src[src_cursor + i];
		src_cursor += len, dst_cursor += len;
		
		// Get the backref offset.
		uint16_t offset = 0;
		memcpy(&offset, src + src_cursor, 2);
		src_cursor += 2;
		
		// Calculate the backref address.
		assert(offset <= dst_cursor);
		uint8_t *backref = dst - offset;
		
		// Done if offset == 0
		if(offset == 0) break;
		
		// Decode backref run length.
		len = (token & 0xF) + 4;
		if(len == 19) do len += src[src_cursor]; while(src[src_cursor++] == 255);
		
		// Copy backref.
		for(size_t i = 0; i < len; i++) dst[dst_cursor + i] = backref[dst_cursor + i];
		dst_cursor += len;
	}
}

uint8_t SRC[1146627];
uint8_t DST[2493109];

int main(int argc, char *argv[]){
	FILE *f = fopen("words.lz4", "r");
	fread(SRC, 1, sizeof(SRC), f);
	
	decompress_lz4(DST, SRC);
	puts(DST);
	
	return EXIT_SUCCESS;
}
