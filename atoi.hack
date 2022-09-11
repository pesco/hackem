# read an ASCII decimal number from tape into R0
# tape reader is mapped at 0x7000 and 0x7001


			# jump to start of program
0 000000000001111	#  0: @15	= MAIN
1 11 0 101010 000 111	#  1: 0;JMP

#
# GETC: read a byte of input from tape into R4. return to R3.
#
			# (GETC)

			# write 1 to control word at 0x7001:
0 111000000000001	#  2: @28673	= 0x7001
1 11 0 111111 001 000	#  3: M=1;

			# wait until 0x7001 negative (position valid):
			#     (WAIT)
0 111000000000001	#  4: @28673	= 0x7001
1 11 1 110000 010 000	#  5: D=M;
0 000000000000100	#  6: @4	= WAIT
1 11 0 001100 000 011	#  7: D;JGE

			# read 0x7000 into R4 (negative indicates EOF):
0 111000000000000	#  8: @28672	= 0x7000
1 11 1 110000 010 000	#  9: D=M;
0 000000000000100	# 10: @4
1 11 0 001100 001 000	# 11: M=D;

			# jump to return address in R3:
0 000000000000011	# 12: @3
1 11 1 110000 100 111	# 13: A=M;JMP


#
# END: terminate (infinite loop).
#
			# (END)
1 11 0 101010 000 111	# 14: 0;JMP


#
# MAIN: read digits and accumulate value in R0.
#
			# (MAIN)

			# read input character (into R4):
0 000000000010101	# 15: @21	= K1
1 11 0 110000 010 000	# 16: D=A;
0 000000000000011	# 17: @3
1 11 0 001100 001 000	# 18: M=D;
0 000000000000010	# 19: @2	= GETC
1 11 0 101010 000 111	# 20: 0;JMP
			#     (K1)

			# terminate on end of input or non-digit
0 000000000000100	# 21: @4
1 11 1 110000 010 000	# 22: D=M;
0 000000000110110	# 23: @54	= DONE
1 11 0 001100 000 100	# 24: D;JLT
0 000000000111001	# 25: @57	= '9'
1 11 0 010011 010 000	# 26: D=D-A;
0 000000000110110	# 27: @54	= DONE
0 11 0 001100 000 001	# 28: D;JGT
0 000000000000100	# 29: @4
1 11 1 110000 010 000	# 30: D=M;
0 000000000110000	# 31: @48	= '0'
1 11 0 010011 010 000	# 32: D=D-A;
0 000000000110110	# 33: @54	= DONE
1 11 0 001100 000 100	# 34: D;JLT

			# replace R4 with digit value
0 000000000000100	# 35: @4
1 11 0 001100 001 000	# 36: M=D;

			# multiply R0 by 10:
0 000000000000000	# 37: @0
1 11 1 110000 010 000	# 38: D=M;
1 11 1 000010 001 000	# 39: M=M+D;	(* 2)
1 11 1 000010 001 000	# 40: M=M+D;	(* 3)
1 11 1 000010 001 000	# 41: M=M+D;	(* 4)
1 11 1 000010 001 000	# 42: M=M+D;	(* 5)
1 11 1 000010 001 000	# 43: M=M+D;	(* 6)
1 11 1 000010 001 000	# 44: M=M+D;	(* 7)
1 11 1 000010 001 000	# 45: M=M+D;	(* 8)
1 11 1 000010 001 000	# 46: M=M+D;	(* 9)
1 11 1 000010 001 000	# 47: M=M+D;	(* 10)

			# add input digit (R4):
0 000000000000100	# 48: @4
1 11 1 110000 010 000	# 49: D=M;
0 000000000000000	# 50: @0
1 11 1 000010 001 000	# 51: M=M+D;

			# process next input character:
0 000000000001111	# 52: @15	= MAIN
1 11 0 101010 000 111	# 53: 0;JMP

			# terminate successfully:
			#     (DONE)
1 11 0 101010 010 000	# 54: D=0;
0 000000000001110	# 55: @14	= END
1 11 0 101010 000 111	# 56: 0;JMP
