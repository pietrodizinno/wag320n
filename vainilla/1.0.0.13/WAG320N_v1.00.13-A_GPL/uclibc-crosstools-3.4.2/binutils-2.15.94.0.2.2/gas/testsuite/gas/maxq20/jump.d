#objdump: -dw
#name: Jump operations

.*: +file format .*

Disassembly of section .text:
0+000 <LableStart>:
   0:	00 0c [ 	]*JUMP  #00h
   2:	ff 2c [ 	]*JUMP C , #ffh
   4:	09 ac [ 	]*JUMP C , A\[0\]
   6:	19 ac [ 	]*JUMP C , A\[1\]
   8:	fc 6c [ 	]*JUMP NC , #fch
   a:	09 ec [ 	]*JUMP NC , A\[0\]
   c:	19 ec [ 	]*JUMP NC , A\[1\]
   e:	f9 4c [ 	]*JUMP S , #f9h
  10:	09 cc [ 	]*JUMP S , A\[0\]
  12:	19 cc [ 	]*JUMP S , A\[1\]
  14:	f6 1c [ 	]*JUMP Z , #f6h
  16:	09 9c [ 	]*JUMP Z , A\[0\]
  18:	19 9c [ 	]*JUMP Z , A\[1\]
  1a:	f3 5c [ 	]*JUMP NZ , #f3h
  1c:	09 dc [ 	]*JUMP NZ , A\[0\]
  1e:	19 dc [ 	]*JUMP NZ , A\[1\]
  20:	f0 3c [ 	]*JUMP E , #f0h
  22:	ef 7c [ 	]*JUMP NE , #efh
  24:	01 7c [ 	]*JUMP NE , #01h

0+026 <Lable1>:
  26:	00 0c [ 	]*JUMP  #00h
  28:	ff 2c [ 	]*JUMP C , #ffh
  2a:	09 ac [ 	]*JUMP C , A\[0\]
  2c:	19 ac [ 	]*JUMP C , A\[1\]
  2e:	fc 6c [ 	]*JUMP NC , #fch
  30:	09 ec [ 	]*JUMP NC , A\[0\]
  32:	19 ec [ 	]*JUMP NC , A\[1\]
  34:	f9 4c [ 	]*JUMP S , #f9h
  36:	09 cc [ 	]*JUMP S , A\[0\]
  38:	19 cc [ 	]*JUMP S , A\[1\]
  3a:	f6 1c [ 	]*JUMP Z , #f6h
  3c:	09 9c [ 	]*JUMP Z , A\[0\]
  3e:	19 9c [ 	]*JUMP Z , A\[1\]
  40:	f3 5c [ 	]*JUMP NZ , #f3h
  42:	09 dc [ 	]*JUMP NZ , A\[0\]
  44:	19 dc [ 	]*JUMP NZ , A\[1\]
  46:	f0 3c [ 	]*JUMP E , #f0h
  48:	ef 7c [ 	]*JUMP NE , #efh
  4a:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  4c:	c6 0c [ 	]*JUMP  #c6h
  4e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  50:	c6 2c [ 	]*JUMP C , #c6h
  52:	09 ac [ 	]*JUMP C , A\[0\]
  54:	19 ac [ 	]*JUMP C , A\[1\]
  56:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  58:	c6 6c [ 	]*JUMP NC , #c6h
  5a:	09 ec [ 	]*JUMP NC , A\[0\]
  5c:	19 ec [ 	]*JUMP NC , A\[1\]
  5e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  60:	c6 1c [ 	]*JUMP Z , #c6h
  62:	09 9c [ 	]*JUMP Z , A\[0\]
  64:	19 9c [ 	]*JUMP Z , A\[1\]
  66:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  68:	c6 5c [ 	]*JUMP NZ , #c6h
  6a:	09 dc [ 	]*JUMP NZ , A\[0\]
  6c:	19 dc [ 	]*JUMP NZ , A\[1\]
  6e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  70:	c6 4c [ 	]*JUMP S , #c6h
  72:	09 cc [ 	]*JUMP S , A\[0\]
  74:	19 cc [ 	]*JUMP S , A\[1\]
  76:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  78:	c6 3c [ 	]*JUMP E , #c6h
  7a:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  7c:	c6 7c [ 	]*JUMP NE , #c6h
  7e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  80:	c6 0c [ 	]*JUMP  #c6h
  82:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  84:	c6 2c [ 	]*JUMP C , #c6h
  86:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  88:	09 ac [ 	]*JUMP C , A\[0\]
  8a:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  8c:	19 ac [ 	]*JUMP C , A\[1\]
  8e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  90:	c6 7c [ 	]*JUMP NE , #c6h
  92:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  94:	c6 1c [ 	]*JUMP Z , #c6h
  96:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  98:	09 9c [ 	]*JUMP Z , A\[0\]
  9a:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  9c:	19 9c [ 	]*JUMP Z , A\[1\]
  9e:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  a0:	c6 5c [ 	]*JUMP NZ , #c6h
  a2:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  a4:	09 dc [ 	]*JUMP NZ , A\[0\]
  a6:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  a8:	19 dc [ 	]*JUMP NZ , A\[1\]
  aa:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  ac:	c6 4c [ 	]*JUMP S , #c6h
  ae:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  b0:	09 cc [ 	]*JUMP S , A\[0\]
  b2:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  b4:	19 cc [ 	]*JUMP S , A\[1\]
  b6:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  b8:	c6 6c [ 	]*JUMP NC , #c6h
  ba:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  bc:	09 ec [ 	]*JUMP NC , A\[0\]
  be:	00 0b [ 	]*MOVE  PFX\[0\], #00h
  c0:	19 ec [ 	]*JUMP NC , A\[1\]
  c2:	04 0b [ 	]*MOVE  PFX\[0\], #04h
  c4:	c6 3c [ 	]*JUMP E , #c6h
	...

0+4c6 <LongJump>:
 4c6:	3a da [ 	]*NOP 
 4c8:	3a da [ 	]*NOP 
 4ca:	3a da [ 	]*NOP 
 4cc:	3a da [ 	]*NOP 
 4ce:	3a da [ 	]*NOP 
