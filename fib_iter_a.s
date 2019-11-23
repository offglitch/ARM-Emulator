    .global fib_iter_a
    .func   fib_iter_a

  // r0 : n
  // r1 : a
  // r2 : b
  // r3 : c
  // r4 : iterator

fib_iter_a:
  mov r1, #0 // initialize a in r1
  mov r2, #1 // initialize b in r2
  mov r3, #1 // initialize c in r3
  mov r4, #1 // initialize iterator in r4
  mov r5, r0 // move n from r0 to r5
  cmp r0, #0 // if (n <= 0)
  beq exit // exit, we don't want zeroes
loop:
  cmp  r4, r0 // compare our iterator's current value
  beq  exit       
  add  r4, r4, #1 // iterate and point to the next element
  add  r3, r1, r2 // cur_num = prev_prev_num + prev_num
  mov  r1, r2     
  mov  r2, r3     
  b    loop
  exit:
  mov r0, r3
  bx  lr

