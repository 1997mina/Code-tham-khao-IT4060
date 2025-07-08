.data
	A: .word 4, 7, 2, 1, 5, 6, 34, 64
.text
	la   $s0, A          # $s0 = base address of A
	addi $s1, $0, 9      # length = 9
	add  $t0, $0, $0     # i = 0
	add  $s2, $0, $0     # sum = 0
LOOP:
	beq  $t0, $s1, EXIT  # If i = 9 then exit
	sll  $t1, $t0, 2     # $t1 = i * 4
	add  $t1, $t1, $s0   # $t1 = address of A[i]
	lw   $t2, 0 ($t1)    # $t2 = A[i]
	add  $s2, $s2, $t2   # sum += A[i]
	addi $t0, $t0, 1     # i += 1
	j    LOOP            # repeat
EXIT:
	addi $v0, $0, 1
	add  $a0, $0, $s2
	syscall