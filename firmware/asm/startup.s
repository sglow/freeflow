# Startup code

   .globl _etext
   .globl _data
   .globl _edata
   .globl _bss
   .globl _ebss
   .globl main
   .globl mainStack

   .text
   .thumb
   .syntax unified

   .globl ResetVect
ResetVect:

   # Zero fill the bss segment.
   ldr      r0, =_bss
   ldr      r1, =_ebss
   cmp      r0, r1
   bge      zero_bss_done
   movs     r2, #0
zero_loop:
   str      r2, [r0]
   adds     r0, #4
   cmp      r0, r1
   blt      zero_loop
zero_bss_done:

   # Copy the data segment initializers from flash to RAM
   ldr      r0, =_etext
   ldr      r1, =_data
   ldr      r2, =_edata
copy_data:
   ldr      r3, [r0]
   str      r3, [r1]
   adds     r0, #4
   adds     r1, #4
   cmp      r1, r2
   blt      copy_data

   # Initialize the main stack to 0x11111111
   ldr      r2, =0x11111111
   ldr      r0, =mainStack
stack_loop:
   str      r2, [r0]
   adds     r0, #4
   cmp      r0, sp
   blt      stack_loop
   
   bl       main

exit:
   b        exit

