; NASM assembly language code for PAQ7.
; (C) 2005, Matt Mahoney.
; This is free software under GPL, http://www.gnu.org/licenses/gpl.txt
;
;   MINGW g++:     nasm paq7asm.asm -f win32 --prefix _
;   DJGPP g++:     nasm paq7asm.asm -f coff  --prefix _
;   Borland, Mars: nasm paq7asm.asm -f obj   --prefix _
;   Linux:         nasm paq7asm.asm -f elf
;
; For other Windows compilers try -f win32 or -f obj.  Some old versions
; of Linux should use -f aout instead of -f elf.
;
; This code will only work on a Pentium-MMX or higher.  It doesn't
; use extended (Katmai/SSE) instructions.  It won't work
; in 64-bit mode.

section .text use32 class=CODE

; Reset after MMX
global do_emms
do_emms:
  emms
  ret

; Vector product a*b of n signed words, returning signed dword scaled
; down by 8 bits. n is rounded up to a multiple of 8.

global dot_product ; (short* a, short* b, int n)
align 16
dot_product:
  mov eax, [esp+4]      ; a
  mov edx, [esp+8]      ; b
  mov ecx, [esp+12]     ; n
  add ecx, 7            ; n rounding up
  and ecx, -8
  jz .done
  sub eax, 8
  sub edx, 8
  pxor mm0, mm0         ; sum = 0
.loop:                  ; each loop sums 4 products
  movq mm1, [eax+ecx*2] ; put halves of vector product in mm0
  pmaddwd mm1, [edx+ecx*2]
  movq mm2, [eax+ecx*2-8]
  pmaddwd mm2, [edx+ecx*2-8]
  psrad mm1, 8
  psrad mm2, 8
  paddd mm0, mm1
  paddd mm0, mm2
  sub ecx, 8
  ja .loop
  movq mm1, mm0         ; add 2 halves of mm0 and return in eax
  psrlq mm1, 32
  paddd mm0, mm1
  movd eax, mm0
  emms
.done
  ret

; This should work on a Pentium 4 or higher in 32-bit mode,
; but it isn't much faster than the MMX version so I don't use it.

global dot_product_sse2 ; (short* a, short* b, int n)
align 16
dot_product_sse2:
  mov eax, [esp+4]      ; a
  mov edx, [esp+8]      ; b
  mov ecx, [esp+12]     ; n
  add ecx, 7            ; n rounding up
  and ecx, -8
  jz .done
  sub eax, 16
  sub edx, 16
  pxor xmm0, xmm0       ; sum = 0
.loop:                  ; each loop sums 4 products
  movdqa xmm1, [eax+ecx*2] ; put parital sums of vector product in xmm0
  pmaddwd xmm1, [edx+ecx*2]
  psrad xmm1, 8
  paddd xmm0, xmm1
  sub ecx, 8
  ja .loop
  movdqa xmm1, xmm0      ; add 4 parts of xmm0 and return in eax
  psrldq xmm1, 8
  paddd xmm0, xmm1
  movdqa xmm1, xmm0
  psrldq xmm1, 4
  paddd xmm0, xmm1
  movd eax, xmm0
.done
  ret


; Train n neural network weights w[n] on inputs t[n] and err.
; w[i] += t[i]*err*2+1 >> 17 bounded to +- 32K.
; n is rounded up to a multiple of 8.

global train ; (short* t, short* w, int n, int err)
align 16
train:
  mov eax, [esp+16]     ; err
  and eax, 0xffff       ; put 4 copies of err in mm0
  movd mm0, eax
  movd mm1, eax
  psllq mm1, 16
  por mm0, mm1
  movq mm1, mm0
  psllq mm1, 32
  por mm0, mm1
  pcmpeqb mm1, mm1      ; 4 copies of 1 in mm1
  psrlw mm1, 15
  mov eax, [esp+4]      ; t
  mov edx, [esp+8]      ; w
  mov ecx, [esp+12]     ; n
  add ecx, 7            ; n/8 rounding up
  and ecx, -8
  sub eax, 8
  sub edx, 8
  jz .done
.loop:                  ; each iteration adjusts 8 weights
  movq mm2, [edx+ecx*2] ; w[i]
  movq mm3, [eax+ecx*2] ; t[i]
  movq mm4, [edx+ecx*2-8] ; w[i]
  movq mm5, [eax+ecx*2-8] ; t[i]
  paddsw mm3, mm3
  paddsw mm5, mm5
  pmulhw mm3, mm0
  pmulhw mm5, mm0
  paddsw mm3, mm1
  paddsw mm5, mm1
  psraw mm3, 1
  psraw mm5, 1
  paddsw mm2, mm3
  paddsw mm4, mm5
  movq [edx+ecx*2], mm2
  movq [edx+ecx*2-8], mm4
  sub ecx, 8
  ja .loop
.done:
  emms
  ret

