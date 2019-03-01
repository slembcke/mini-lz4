section "lz4-data", hram

token: db ; Most recently read token.
backref: dw

; NOTES:
; dst/src is always stored in de/hl

section "lz4-code", rom0

; de: dst, hl: src
decompress_lz4::
	; Read token from stream.
	ld a, [hl+]
	ldh [token], a
	
	; Decode literal count into bc starting with upper nibble of token.
	swap a
	and $F
	ld b, 0
	ld c, a
	
	cp 15
	call consume_inc_bytes
	call copy_to_dst
	
	; Check if backref is 0.
	ld a, [hl+]
	ldh [backref+0], a
	or [hl]
	ret z
	ld a, [hl+]
	ldh [backref+1], a
	
	; Decode backref count into bc starting with the lower nibble of token.
	ldh a, [token]
	and $F
	add 4 ; Minimum match length is 4.
	; B is reset by copy_to_dst
	ld c, a
	
	cp 19
	call consume_inc_bytes
	
	; Save src pointer.
	push hl
	
	; backref_src = dst - backref
	ldh a, [backref+0]
	sub e
	cpl
	ld l, a
	ldh a, [backref+1]
	sbc d
	cpl
	ld h, a
	inc hl
	
	; Copy the back ref.
	call copy_to_dst
	
	; Restore src and repeat.
	pop hl
	jp decompress_lz4

consume_inc_bytes:
	ret nz
	
	; 	u8 overflow = *src++;
	ld a, [hl]
	
	; 	literals += overflow;
	add c
	ld c, a
	jr nc, .no_inc_b
	inc b
.no_inc_b:

	ld a, [hl+]
	
	; Check if it was 255
	inc a
	jr consume_inc_bytes

; de: dst, hl: src, bc: size
copy_to_dst:
	; Check exit condition first.
	ld a, b
	or c
	ret z
	
	ld a, [hl+]
	ld [de], a
	inc de
	dec bc
	jr copy_to_dst
