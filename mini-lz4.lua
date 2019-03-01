-- local dbg = dofile("../debugger.lua/debugger.lua")
-- dbg.auto_where = 2

local FLAG_BLOCK_INDEPENDENCE = 0x20
local FLAG_BLOCK_CHECKSUM = 0x10
local FLAG_CONTENT_SIZE = 0x08
local FLAG_CONTENT_CHECKSUM = 0x04
local FLAG_DICTIONARY_ID = 0x01

function decompress_lz4(src)
	local src_cursor = 1
	
	local function read(format)
		local value;
		value, src_cursor = string.unpack(format, src, src_cursor)
		return value
	end
	
	-- Check the FOURCC.
	assert(read("<I4") == 0x184D2204, "LZ4 Error: Invalid FOURCC.")
	
	-- Read flags and bd_byte.
	local flags = read("B")
	local bd_byte = read("B")
	
	-- Check LZ4 version number is valid.
	assert((flags & 0xC0) == 0x40, "LZ4 Error: nvalid version number")
	
	-- Check that the reserved format bits are 0.
	assert((flags & 0x02) == 0 and (bd_byte & 0x00) == 0)
	
	assert(flags & FLAG_CONTENT_SIZE ~= 0, "LZ4 Error: file must have content size embedded.")
	local content_size = read("<I8")
	
	assert(flags & FLAG_DICTIONARY_ID == 0, "LZ4 Error: Dictionaries not supported.")
	-- local dictionary_id = read("<I4")
	
	local header_checksum = read("B")
	
	local block_size = read("<I4")
	assert(block_size & 0x80000000 == 0, "LZ4 Error: Uncompressed blocks not supported.")
	
	local block_end = src_cursor + block_size
	
	local function extend_len(len)
		repeat
			local value = read("B")
			len = len + value
		until value < 255
		return len
	end
	
	local chunks, buffer = {}, ""
	while(true) do
		local offset, len
		local token = read("B")
		
		-- Decode literal run length.
		len = token >> 4
		if len == 15 then len = extend_len(len) end
		
		-- Copy literals.
		buffer = buffer..src:sub(src_cursor, src_cursor + len - 1)
		src_cursor = src_cursor + len
		
		-- Check if we are at the end of the stream.
		if src_cursor == block_end then break end
		-- dbg(src_cursor < 1000)
		
		-- Get the backref offset.
		offset = read("<I2")
		assert(offset < #buffer)
		
		-- Decode the backref run length.
		len = (token & 0xF) + 4
		if len == 19 then len = extend_len(len) end
		
		-- Copy backref.
		while len > 0 do
			local sub_len = math.min(len, offset)
			buffer = buffer..buffer:sub(-offset, sub_len - offset - 1)
			len = len - sub_len
		end
		-- for i = 0, len - 1 do buffer = buffer..buffer:sub(offset, offset) end
		
		if #buffer > 90000 then
			table.insert(chunks, buffer:sub(1, -65537))
			buffer = buffer:sub(-65536, -1)
		end
	end
	
	table.insert(chunks, buffer)
	return table.concat(chunks)
end

local file = io.open("words.lz4", "rb")
local src = file:read("a")
file:close()

local foo = decompress_lz4(src)
print(foo)

return
