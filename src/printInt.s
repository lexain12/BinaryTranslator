section .text

global _start
;=======================================
; Entry: rdi - pointer on int
; Exit:  none, prints int to buffer
; Uses:  r11 - intermediate buffer for puting int there, r12 - buffer for reversing number there
;=======================================
_start:
	xor rbx, rbx
    xor rcx, rcx
	mov rax, rdi
	mov rdi, 10
	cmp eax, 0              ; if negative
	jge .Loop
	not eax
	inc rax
	mov cl, 0x01            ; flag for negative

.Loop:
	xor rdx, rdx
	div rdi
	add rdx, 0x30
	mov byte [r11 + rbx], dl
	inc rbx
	cmp rax, 0x00
	jne .Loop

	cmp cl, 0x01 				; check if number is negative
	jne .NotNeg
	mov byte [r11 + rbx], '-'   ;
	inc rbx

.NotNeg:
	mov rdi, r12
	lea rsi, [r11 + rbx - 1]
	mov rdx, rbx
	call MemcpyR
    mov rdi, 1
    mov eax, 1
    mov rsi, r12
    mov rdx, 5
    syscall
    ret

MemcpyR:        ; reverses the number from r11 buff to r12 buff

	push rdi
.Loop1:
	mov al, byte [rsi]
	mov byte [rdi], al

	dec rsi
	inc rdi
	dec rdx
	cmp rdx, 0x00
	jne .Loop1
	pop rax
	ret

