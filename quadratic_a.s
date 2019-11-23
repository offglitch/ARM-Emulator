    .global quadratic_a
    .func quadratic_a

  // R0 : x
  // R1 : a
  // R2 : b
  // R3 : c

quadratic_a:
  mul   r4, r0, r0    // R4 = x*x 
  mul   r5, r4, r1    // R4 = R4 * a
  mov   r4, r5
  mul   r5, r2, r0    // R5 = b*x
  add   r5, r5, r3    // R5 = R5 + c
  add   r4, r4, r5    // R4 = R4 + R5

  mov   r0, r4
  bx    lr