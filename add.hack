# adapted from nand2tetris/projects/05/Add.hack
# distributed under Creative Commons BY-NC-SA 3.0

			# compute D=2+3, store result in R0
0 000000000000010	# 0: @2
1 11 0 110000 010 000	# 1: D=A; (~0 & A)
0 000000000000011	# 2: @3
1 11 0 000010 010 000	# 3: D=D+A;
0 000000000000000	# 4: @0
1 11 0 001100 001 000	# 5: M=D; (D & ~0)

			# terminate (enter infinite loop)
0 000000000000111	# 6: @7
1 11 0 101000 000 111	# 7: 0;JMP (0 & 0)
