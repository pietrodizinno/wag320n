#source: elf-rel23.s
#as: -march=mips3 -mabi=64 -mno-shared
#objdump: -dr -Mgpr-names=numeric
#name: MIPS ELF reloc 23 -mabi=64 -mno-shared

.*: * file format elf64-tradbigmips

Disassembly of section \.text:

0+00 <.*>:
.*:	0380282d 	move	\$5,\$28
.*:	3c1c0000 	lui	\$28,0x0
			.*: R_MIPS_GPREL16	foo
			.*: R_MIPS_SUB	\*ABS\*
			.*: R_MIPS_HI16	\*ABS\*
.*:	279c0000 	addiu	\$28,\$28,0
			.*: R_MIPS_GPREL16	foo
			.*: R_MIPS_SUB	\*ABS\*
			.*: R_MIPS_LO16	\*ABS\*
.*:	0384e02d 	daddu	\$28,\$28,\$4
