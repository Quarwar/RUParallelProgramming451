# Parallel Programming HW #3
# Vectorization of Sorting
# Author: Joseph Signorile
# Cite: ChatGPT (help with syntax, assembly, generating test data),
# Optimal Sorting Network for 14 items:
# https://bertdobbelaere.github.io/sorting_networks.html#TAOCPv3
#
# Description:
# Given a list of 14*8 items, uses AVX2 vector operations
# to optimize parallel sorting.
#
# Note: I did need to use Chat to figure out a lot of the assembly-specific details -- this was kinda tricky

    .macro vminmax reg1, reg2, tmp
        vpminsd \reg1, \reg2, \tmp       # tmp = min(reg1, reg2)
        vpmaxsd \reg1, \reg2, \reg2      # reg2 = max(reg1, reg2)
        vmovdqa \tmp, \reg1              # reg1 = min
    .endm

    .section .data
    .align 32
    .global array

# 14 * 8 = 112 input integers
array:
    .long 42, -17, 93, 5, 61, -84, 27, 10
    .long -3, 76, 15, 88, -42, 31, 7, 54
    .long 99, -12, 38, 21, 67, -55, 4, 82
    .long 16, 73, -91, 45, 29, 8, 64, -26
    .long 51, 19, -7, 94, 33, 72, -63, 6
    .long 28, -38, 85, 12, 57, -14, 3, 69
    .long -72, 41, 23, 97, -5, 62, 18, 35
    .long 79, -29, 11, 53, -88, 26, 70, 2
    .long 48, 14, -51, 83, 37, 9, 66, -19
    .long 25, 91, -6, 58, 43, -77, 13, 34
    .long 71, 0, -34, 87, 20, 59, -45, 96
    .long 30, -11, 74, 52, 17, -68, 81, 1
    .long 63, 22, -9, 46, 89, -31, 36, 78
    .long -25, 55, 68, -57, 24, 39, 90, -2

    .section .rodata

format:
    .string "%d "

newline:
    .string "\n"

    .section .bss
    .align 32

# Temporary array used by the 8-way merge
output:
    .zero 448                  # 112 * 4 bytes

    .align 8

ptrs:
    .zero 64                   # 8 * 8-byte pointers

    .align 4

# Number of remaining elements in each sequence
counts:
    .zero 32                   # 8 * 4-byte counts


    .section .text
    .align 32

    .global main
    .extern printf


main:

    # --------------------------------------------------------
    # Preserve callee-saved registers.
    # --------------------------------------------------------

    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # main starts with RSP % 16 == 8.
    # Six pushes change it back to 8.
    # Subtract 8 so RSP is 16-byte aligned before printf.
    subq $8, %rsp

    # load data into the ymm registers
    leaq array(%rip), %rcx

    vmovdqa    0(%rcx),   %ymm0
    vmovdqa   32(%rcx),   %ymm1
    vmovdqa   64(%rcx),   %ymm2
    vmovdqa   96(%rcx),   %ymm3
    vmovdqa  128(%rcx),   %ymm4
    vmovdqa  160(%rcx),   %ymm5
    vmovdqa  192(%rcx),   %ymm6
    vmovdqa  224(%rcx),   %ymm7
    vmovdqa  256(%rcx),   %ymm8
    vmovdqa  288(%rcx),   %ymm9
    vmovdqa  320(%rcx),   %ymm10
    vmovdqa  352(%rcx),   %ymm11
    vmovdqa  384(%rcx),   %ymm12
    vmovdqa  416(%rcx),   %ymm13

    # Optimal sorting network for 14 values

    # --------------------------------------------------------
    # Stage 1
    # [(0,1),(2,3),(4,5),(6,7),(8,9),(10,11),(12,13)]
    # --------------------------------------------------------

    vminmax %ymm0,  %ymm1,  %ymm14
    vminmax %ymm2,  %ymm3,  %ymm14
    vminmax %ymm4,  %ymm5,  %ymm14
    vminmax %ymm6,  %ymm7,  %ymm14
    vminmax %ymm8,  %ymm9,  %ymm14
    vminmax %ymm10, %ymm11, %ymm14
    vminmax %ymm12, %ymm13, %ymm14


    # --------------------------------------------------------
    # Stage 2
    # [(0,2),(1,3),(4,8),(5,9),(10,12),(11,13)]
    # --------------------------------------------------------

    vminmax %ymm0,  %ymm2,  %ymm14
    vminmax %ymm1,  %ymm3,  %ymm14
    vminmax %ymm4,  %ymm8,  %ymm14
    vminmax %ymm5,  %ymm9,  %ymm14
    vminmax %ymm10, %ymm12, %ymm14
    vminmax %ymm11, %ymm13, %ymm14


    # --------------------------------------------------------
    # Stage 3
    # [(0,10),(1,6),(2,11),(3,13),(5,8),(7,12)]
    # --------------------------------------------------------

    vminmax %ymm0,  %ymm10, %ymm14
    vminmax %ymm1,  %ymm6,  %ymm14
    vminmax %ymm2,  %ymm11, %ymm14
    vminmax %ymm3,  %ymm13, %ymm14
    vminmax %ymm5,  %ymm8,  %ymm14
    vminmax %ymm7,  %ymm12, %ymm14


    # --------------------------------------------------------
    # Stage 4
    # [(1,4),(2,8),(3,6),(5,11),(7,10),(9,12)]
    # --------------------------------------------------------

    vminmax %ymm1,  %ymm4,  %ymm14
    vminmax %ymm2,  %ymm8,  %ymm14
    vminmax %ymm3,  %ymm6,  %ymm14
    vminmax %ymm5,  %ymm11, %ymm14
    vminmax %ymm7,  %ymm10, %ymm14
    vminmax %ymm9,  %ymm12, %ymm14


    # --------------------------------------------------------
    # Stage 5
    # [(0,1),(3,9),(4,10),(5,7),(6,8),(12,13)]
    # --------------------------------------------------------

    vminmax %ymm0,  %ymm1,  %ymm14
    vminmax %ymm3,  %ymm9,  %ymm14
    vminmax %ymm4,  %ymm10, %ymm14
    vminmax %ymm5,  %ymm7,  %ymm14
    vminmax %ymm6,  %ymm8,  %ymm14
    vminmax %ymm12, %ymm13, %ymm14


    # --------------------------------------------------------
    # Stage 6
    # [(1,5),(2,4),(3,7),(6,10),(8,12),(9,11)]
    # --------------------------------------------------------

    vminmax %ymm1,  %ymm5,  %ymm14
    vminmax %ymm2,  %ymm4,  %ymm14
    vminmax %ymm3,  %ymm7,  %ymm14
    vminmax %ymm6,  %ymm10, %ymm14
    vminmax %ymm8,  %ymm12, %ymm14
    vminmax %ymm9,  %ymm11, %ymm14


    # --------------------------------------------------------
    # Stage 7
    # [(1,2),(3,5),(4,6),(7,9),(8,10),(11,12)]
    # --------------------------------------------------------

    vminmax %ymm1,  %ymm2,  %ymm14
    vminmax %ymm3,  %ymm5,  %ymm14
    vminmax %ymm4,  %ymm6,  %ymm14
    vminmax %ymm7,  %ymm9,  %ymm14
    vminmax %ymm8,  %ymm10, %ymm14
    vminmax %ymm11, %ymm12, %ymm14


    # --------------------------------------------------------
    # Stage 8
    # [(2,3),(4,5),(6,7),(8,9),(10,11)]
    # --------------------------------------------------------

    vminmax %ymm2,  %ymm3,  %ymm14
    vminmax %ymm4,  %ymm5,  %ymm14
    vminmax %ymm6,  %ymm7,  %ymm14
    vminmax %ymm8,  %ymm9,  %ymm14
    vminmax %ymm10, %ymm11, %ymm14


    # --------------------------------------------------------
    # Stage 9
    # [(3,4),(5,6),(7,8),(9,10)]
    # --------------------------------------------------------

    vminmax %ymm3,  %ymm4,  %ymm14
    vminmax %ymm5,  %ymm6,  %ymm14
    vminmax %ymm7,  %ymm8,  %ymm14
    vminmax %ymm9,  %ymm10, %ymm14

    # store data back to the array
    vmovdqa %ymm0,    0(%rcx)
    vmovdqa %ymm1,   32(%rcx)
    vmovdqa %ymm2,   64(%rcx)
    vmovdqa %ymm3,   96(%rcx)
    vmovdqa %ymm4,  128(%rcx)
    vmovdqa %ymm5,  160(%rcx)
    vmovdqa %ymm6,  192(%rcx)
    vmovdqa %ymm7,  224(%rcx)
    vmovdqa %ymm8,  256(%rcx)
    vmovdqa %ymm9,  288(%rcx)
    vmovdqa %ymm10, 320(%rcx)
    vmovdqa %ymm11, 352(%rcx)
    vmovdqa %ymm12, 384(%rcx)
    vmovdqa %ymm13, 416(%rcx)


    # 8-way merge:

    leaq array(%rip), %rax
    leaq ptrs(%rip), %rdi

    movq %rax,      0(%rdi)
    leaq 4(%rax),   %rdx
    movq %rdx,      8(%rdi)
    leaq 8(%rax),   %rdx
    movq %rdx,     16(%rdi)
    leaq 12(%rax),  %rdx
    movq %rdx,     24(%rdi)
    leaq 16(%rax),  %rdx
    movq %rdx,     32(%rdi)
    leaq 20(%rax),  %rdx
    movq %rdx,     40(%rdi)
    leaq 24(%rax),  %rdx
    movq %rdx,     48(%rdi)
    leaq 28(%rax),  %rdx
    movq %rdx,     56(%rdi)


    # every sequence initially has 14 elements.
    leaq counts(%rip), %rdi
    movl $14,  0(%rdi)
    movl $14,  4(%rdi)
    movl $14,  8(%rdi)
    movl $14, 12(%rdi)
    movl $14, 16(%rdi)
    movl $14, 20(%rdi)
    movl $14, 24(%rdi)
    movl $14, 28(%rdi)

    # 8-way merge
    leaq ptrs(%rip),   %r12
    leaq counts(%rip), %r13
    leaq output(%rip), %r14

    # Number of values produced
    xorq %r15, %r15


merge_loop:
    # largest possible value
    movl $0x7fffffff, %r8d

    # r9d = index of sequence containing current minimum
    movl $-1, %r9d

    cmpl $0, 0(%r13)
    je merge_seq1
    movq 0(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq1
    movl %r10d, %r8d
    movl $0, %r9d

merge_seq1:
    cmpl $0, 4(%r13)
    je merge_seq2
    movq 8(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq2
    movl %r10d, %r8d
    movl $1, %r9d

merge_seq2:
    cmpl $0, 8(%r13)
    je merge_seq3
    movq 16(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq3
    movl %r10d, %r8d
    movl $2, %r9d

merge_seq3:
    cmpl $0, 12(%r13)
    je merge_seq4
    movq 24(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq4
    movl %r10d, %r8d
    movl $3, %r9d

merge_seq4:
    cmpl $0, 16(%r13)
    je merge_seq5
    movq 32(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq5
    movl %r10d, %r8d
    movl $4, %r9d

merge_seq5:
    cmpl $0, 20(%r13)
    je merge_seq6
    movq 40(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq6
    movl %r10d, %r8d
    movl $5, %r9d

merge_seq6:
    cmpl $0, 24(%r13)
    je merge_seq7
    movq 48(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_seq7
    movl %r10d, %r8d
    movl $6, %r9d


merge_seq7:
    cmpl $0, 28(%r13)
    je merge_store
    movq 56(%r12), %rax
    movl (%rax), %r10d
    cmpl %r8d, %r10d
    jge merge_store
    movl %r10d, %r8d
    movl $7, %r9d


merge_store:
    # store minimum to output
    movl %r8d, (%r14)
    addq $4, %r14

    # Advance the pointer for the list that contains the minimum
    cmpl $0, %r9d
    jne advance1
    addq $32, 0(%r12)
    decl 0(%r13)
    jmp merge_next

advance1:
    cmpl $1, %r9d
    jne advance2
    addq $32, 8(%r12)
    decl 4(%r13)
    jmp merge_next

advance2:
    cmpl $2, %r9d
    jne advance3
    addq $32, 16(%r12)
    decl 8(%r13)
    jmp merge_next


advance3:
    cmpl $3, %r9d
    jne advance4
    addq $32, 24(%r12)
    decl 12(%r13)
    jmp merge_next


advance4:
    cmpl $4, %r9d
    jne advance5
    addq $32, 32(%r12)
    decl 16(%r13)
    jmp merge_next


advance5:
    cmpl $5, %r9d
    jne advance6
    addq $32, 40(%r12)
    decl 20(%r13)
    jmp merge_next


advance6:
    cmpl $6, %r9d
    jne advance7
    addq $32, 48(%r12)
    decl 24(%r13)
    jmp merge_next


advance7:
    addq $32, 56(%r12)
    decl 28(%r13)


merge_next:
    incq %r15
    cmpq $112, %r15
    jl merge_loop


    # ========================================================
    # COPY MERGED RESULT BACK INTO ARRAY
    # ========================================================

    leaq output(%rip), %rsi
    leaq array(%rip),  %rdi

    movl $112, %ecx

copy_loop:

    movl (%rsi), %eax
    movl %eax, (%rdi)

    addq $4, %rsi
    addq $4, %rdi

    decl %ecx
    jnz copy_loop


    # Print the final output for testing:
    leaq array(%rip), %r12
    xorq %r13, %r13


print_loop:
    movl (%r12,%r13,4), %esi
    leaq format(%rip), %rdi

    # Required for variadic functions such as printf:
    # number of vector registers used for arguments = 0
    xorl %eax, %eax
    call printf
    incq %r13
    cmpq $112, %r13
    jl print_loop

    # Print newline
    leaq newline(%rip), %rdi
    xorl %eax, %eax
    call printf
    addq $8, %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx

    # return 0
    xorl %eax, %eax
    ret