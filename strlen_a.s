.global strlen_a
.func   strlen_a

strlen_a:
  mov r2, #0
  loop:
    ldrb r1, [r0]       // Get the first character from string
    cmp  r1, #0         // Check if equal to '\0'
    beq  exit           // If equal then exit
    add  r2, r2, #1     // Otherwise, increment the length
    add  r0, r0, #1     // Get the pointer to next character
    b    loop           // Loop back
  exit:
  mov r0, r2            // Return value in R0
  bx  lr                // Return


