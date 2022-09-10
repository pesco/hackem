# simplified "hello world":
# put out the characters "Hi\n" on the tape punch.

# tape punch is mapped at address 0x7002 (28674), control word at 28675.


			# jump to the beginning of the program:
0 000000000010100	#  0: @20	= MAIN
1 11 0 101010 000 111	#  1: 0;JMP


#
# PUTC: print character in R4, return to location in R3
#
			#     (PUTC)

			# read control word until it becomes negative:
0 111000000000011	#  2: @28675	= 0x7003
1 11 1 110000 010 000	#  3: D=M;
0 000000000000010	#  4: @2	= PUTC
1 11 0 001100 000 011	#  5: D;JGE

			# if bit 14 is set, terminate with error:
0 100000000000000	#  6: @16384 (0x4000 = 2^14)
1 11 0 000000 010 000	#  7: D=D&A;
0 000000000001101	#  8: @13	= CONT
1 11 0 001100 000 010	#  9: D;JEQ
1 11 0 111111 010 000	# 10: D=1;
0 000000000010011	# 11: @19	= END
1 11 0 101010 000 111	# 12: 0;JMP
			#     (CONT)

			# write R4 to 0x7002:
0 000000000000100	# 13: @4
1 11 1 110000 010 000	# 14: D=M;
0 111000000000010	# 15: @28674	= 0x7002
1 11 0 001100 001 000 	# 16: M=D;

			# jump to return address stored in R3
0 000000000000011	# 17: @3
1 11 1 110000 100 111	# 18: A=M;JMP


#
# END: terminate (infinite loop)
#
			#     (END)
1 11 0 101010 000 111	# 19: 0;JMP


#
# MAIN: punch out message and terminate.
#
			#     (MAIN)

			# call PUTC('H'):
0 000000001001000	# 20: @72	= 'H'
1 11 0 110000 010 000	# 21: D=A;
0 000000000000100	# 22: @4
1 11 0 001100 001 000	# 23: M=D;
0 000000000011110	# 24: @30	= K1
1 11 0 110000 010 000	# 25: D=A;
0 000000000000011	# 26: @3
1 11 0 001100 001 000	# 27: M=D;
0 000000000000010	# 28: @2	= PUTC
1 11 0 101010 000 111	# 29: 0;JMP
			#     (K1)

			# call PUTC('i'):
0 000000001101001	# 30: @105	= 'i'
1 11 0 110000 010 000	# 31: D=A;
0 000000000000100	# 32: @4
1 11 0 001100 001 000	# 33: M=D;
0 000000000101000	# 34: @40	= K2
1 11 0 110000 010 000	# 35: D=A;
0 000000000000011	# 36: @3
1 11 0 001100 001 000	# 37: M=D;
0 000000000000010	# 38: @2	= PUTC
1 11 0 101010 000 111	# 39: 0;JMP
			#     (K2)

			# call PUTC('\n'):
0 000000000001010	# 40: @10	= '\n'
1 11 0 110000 010 000	# 41: D=A;
0 000000000000100	# 42: @4
1 11 0 001100 001 000	# 43: M=D;
0 000000000110010	# 44: @50	= K3
1 11 0 110000 010 000	# 45: D=A;
0 000000000000011	# 46: @3
1 11 0 001100 001 000	# 47: M=D;
0 000000000000010	# 48: @2	= PUTC
1 11 0 101010 000 111	# 49: 0;JMP
			#     (K3)

			# terminate successfully:
1 11 0 101010 010 000	# 50: D=0;
0 000000000010011	# 51: @19	= END
1 11 0 101010 000 111	# 52: 0;JMP