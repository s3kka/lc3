# LC-3 Virtual Machine

A small, simple, single-file virtual machine for the LC-3, written in C.

## Overview

This VM supports most features of the LC-3 architecture specification:

| Feature 	  | Supported | Notes |
|-------------|-----------|-------|
| `ADD` 			  | Yes 			|   |
| `AND`				  | Yes 			|   |
| `BR` 					| Yes 			|   |
| `JMP/RET`			| Yes 			|   |
| `JSR/JSRR`		| Yes 			|   |
| `LD`  				| Yes 			|   |
| `LDI` 				| Yes 			|   |
| `LDR` 				| Yes 			|   |
| `LEA` 				| Yes 			|   |
| `NOT` 				| Yes 			|   |
| `ST`  				| Yes 			|   |
| `STI` 			  | Yes 			|   |
| `STR`				  | Yes 			|   |
| `RTI` 	      | No 			  | Logged and skipped  |
| `RES` 	      | No 			  | Treated as illegal; logged and skipped  |
| `TRAP` 				| Yes 			|   |
| `GETC` trap 	| Yes 			|   |
| `OUT` trap 		| Yes 			|   |
| `PUTS` trap  	| Yes 			|   |
| `IN` trap 		| Yes 			|   |
| `PUTSP` trap	| Yes 			|   |
| `HALT` trap 	| Yes 			|   |
|Interrupt-driven I/O| No |   |

## Build Instructions

1. Clone the repository
```
git clone https://github.com/s3kka/lc3.git
cd lc3
```
 
2. Compile `lc3.c`

### Linux / macOS
```
gcc -Wall -Wextra -o lc3 lc3.c
```

### Windows (MinGW / GCC)
```
gcc -Wall -Wextra -o lc3.exe lc3.c
```

### Windows with MSVC
```
cl lc3.c /out:lc3.exe
```

## Running
Execute the VM and pass an LC-3 `.obj` binary file: 
```
./lc3 <program.obj>
```
You can test the VM with a `./tests/2048.obj` example:
```
./lc3 ./tests/2048.obj
```
