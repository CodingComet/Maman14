; Invalid test multiple identicle extern definitions

.entry LENGTH
.extern L3
.extern L3
.extern W
MAIN: mov S1.1, W
 addd r2,STR
LOOP: jp W
 prn #-5
 SUBT r1, r4
 
 inc K

mov S1.2, r3

 bne L3
END: hlt
STR: .string "abcdef"
LENGTH: .data 6,-9,15

K: .data 22
S1: .struct 8, "ab" 
S1: .data 9.3