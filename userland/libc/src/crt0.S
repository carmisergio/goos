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
    
    // We shouldn't be here
    // If we fall through, do the leasat destructive thing we can
    // Just loopidy loop loop forever
hlt_lp:
    pause
    jmp hlt_lp 