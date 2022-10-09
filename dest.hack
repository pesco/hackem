			# writing to A in a jump does not affect the destination
0 000000000000110 	# 0: @Y
1 11 0 110000 010 000 	# 1: D=A;
0 000000000000101	# 2: @X
1 11 0 001100 100 111	# 3: A=D;JMP	// should go to X (5) with A=D=Y (6)
1 11 0 101010 100 000	# 4: A=0;	// not reached (infinite loop)
			# 5: (X)
1 11 0 101010 010 000	#    D=0;
			# 6: (Y)	// only reached with A=6 D=0
1 11 0 101010 000 111	#    0;JMP	// terminates
