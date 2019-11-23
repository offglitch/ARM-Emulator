    .global sum_array_a
    .func   sum_array_a

  // r0 : Array base address
  // r1 : Array size
  // r2 : iterator
  // r3 : final sum value

sum_array_a:
  mov   r2, #0        // r2 holds the iterator i
  mov   r3, #0        // r3 holds the final sum 
loop:
  cmp   r2, r1        // compare r2 and r1
  beq   exit          // if it's true exit the loop
  ldr   r4, [r0]      // else get the value from memory [r0] and load into r4
  add   r3, r3, r4    // sum of each element added
  add   r0, r0, #4    // increment by 4 bytes for each element
  add   r2, r2, #1    // iterate 
  b     loop
exit:
  mov   r0, r3
  bx    lr

