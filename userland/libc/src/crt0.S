//
// GOOS userspace program stub
// Sets up execution environment for C programs
// Handles calling the exit() syscall upon returning
// from main()
//

.section .text
.global _start // Program entry point
.global main   // Main function
.global exit   // Exit system call

_start:
    
    // Start execution
    call main
    
    // Exit system call
    push %eax
    call exit