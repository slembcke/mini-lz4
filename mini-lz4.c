#include <stdbool.h>
#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FLAG_BLOCK_INDEPENDENCE(flags) (flags & 0x20)
#define FLAG_BLOCK_CHECKSUM(flags) (flags & 0x10)
#define FLAG_CONTENT_SIZE(flags) (flags & 0x08)
#define FLAG_CONTENT_CHECKSUM(flags) (flags & 0x04)
#define FLAG_DICTIONARY_ID(flags) (flags & 0x01)

// Decompress lz4 data.
// dst: Pointer to the destination buffer.
// dst_size: Size of the destination buffer.
// src: Pointer to the lz4 data.
// src_size: Size of the lz4 data.
// Returns a pointer to the decompressed data.
// If dst is NULL it will be allocated with malloc() if the stream contains the content size.
// Does *NOT* provide any error handling and aborts on an error.
uint8_t *decompress_lz4(uint8_t *dst, size_t dst_size, uint8_t *src, size_t src_size){
	// TODO src_size is not used.
	assert(src);
	
	// Check FOURCC code.
	assert(memcmp(src, "\x04\x22\x4D\x18", 4) == 0);
	src += 4;
	
	uint8_t flags = src[0];
	uint8_t bd_byte = src[1];
	src += 2;
	
	// Check LZ4 version number is valid.
	assert((flags & 0xC0) == 0x40);
	
	// Reserved bits must always be 0.
	// TODO uhh db_byte mask is 0?
	assert((flags & 0x02) == 0 && (bd_byte & 0x00) == 0);
	
	// Read content size.
	uint64_t content_size = dst_size;
	if(FLAG_CONTENT_SIZE(flags)){
		memcpy(&content_size, src, 8);
		src += 8;
		
		assert(dst_size == 0 || content_size <= dst_size);
	}
	assert(content_size > 0);
	
	// Allocate destination memory if needed.
	if(dst == NULL) dst = malloc(content_size);
	
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
		if(len == 15) do {
			assert(src_cursor <= block_size);
			len += src[src_cursor];
		} while(src[src_cursor++] == 255);
		
		// Copy literals.
		assert(dst_cursor + len <= content_size);
		memcpy(dst + dst_cursor, src + src_cursor, len);
		src_cursor += len, dst_cursor += len;
		
		// Check if we are at the end of the stream.
		if(src_cursor == block_size) break;
		
		// Get the backref offset.
		uint16_t offset = 0;
		assert(src_cursor + 2 <= block_size);
		memcpy(&offset, src + src_cursor, 2);
		src_cursor += 2;
		
		// Calculate the backref address.
		assert(offset <= dst_cursor);
		uint8_t *backref = dst - offset;
		
		// Decode backref run length.
		len = (token & 0xF) + 4;
		if(len == 19) do {
			assert(src_cursor <= block_size);
			len += src[src_cursor];
		} while(src[src_cursor++] == 255);
		
		// Copy backref.
		assert(dst_cursor + len <= content_size);
		for(size_t i = 0; i < len; i++) dst[dst_cursor + i] = backref[dst_cursor + i];
		dst_cursor += len;
	}
	
	return dst;
}
