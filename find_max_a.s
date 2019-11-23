	.global find_max_a
	.func   find_max_a

  // r0 : Array base address
  // r1 : Size of the array
  // r2 : iterator
  // r3 : Max value

find_max_a:
	mov   r2, #0          // Initilize the iterator in r2
	ldr   r3, [r0]        // value from memory [r0] and load into r3 max = array[0]
loop:
	cmp   r2, r1          // Check if iterator == array size
	beq   exit            // exit if equal
	ldr   r4, [r0]        // r4 = array[0]
	cmp   r3, r4          // Check if max < r4
	movlt r3, r4          // move only if max < r4 -> max = r4
    add   r2, r2, #1      // Increment the iterator
	add   r0, r0, #4      // Get the next element in array (each element is 4 bytes, that's why we're incrementing by 4)
	b     loop            // Loop back
exit:
	mov   r0, r3          // Return value in r0
	bx    lr              // Return to main