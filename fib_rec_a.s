.global fib_rec_a
.func fib_rec_a

fib_rec_a:
  sub sp, sp, #4          // Make space in stack
  str lr, [sp]            // Push lr on top of the stack
  sub sp, sp, #4          // Make space in stack
  str r0, [sp]            // Store r0 on stack
  cmp r0, #0              // Check if n is zero
  bne _notZero
  ldr r0, [sp]            // Get r0 back
  add sp, sp, #4          // Move stack
  ldr lr, [SP]            // Get lr back
  add sp, sp, #4          // Move stack
  bx  lr                  // return zero if n is zero
  _notZero:
    cmp r0, #1            // check if n is 1
    bne _notOne
    ldr r0, [sp]          // Get r0 back
    add sp, sp, #4        // Move stack
    ldr lr, [SP]          // Get lr back
    add sp, sp, #4        // Move stack
    bx  lr                // return zero if n is zero
    _notOne:
    ldr r0, [sp]          // Get r0 back
    sub r0, r0, #1        // n = n-1
    bl  fib_rec_a         // Call fib_rec_a
    mov r3, r0            // Save result in r3
    ldr r0, [sp]          // Get r0 back
    add sp, sp, #4        // Move stack
    sub r0, r0, #2        // n = n-2
    sub sp, sp, #4        // Make space in stack
    str r3, [sp]          // Store r3 on stack
    bl  fib_rec_a         // call fib_rec_a
    ldr r3, [sp]          // Get r3 back
    add sp, sp, #4        // Move stack
    add r0, r3, r0        // fib(n-1) + fib(n-2)
    ldr lr, [SP]          // Get lr back
    add sp, sp, #4        // Move stack
    bx  lr                // return zero if n is zero

