# terminate with idiomatic two-instruction loop, return 23
0 000000000010111 	# 0: @23
1 11 0 110000 010 000 	# 1: D=A; (~0 & A)
0 000000000000010 	# 2: @2
1 11 0 101010 000 111	# 3: 0;JMP (0 + 0)
