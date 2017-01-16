============ WAG320N compile environment ==========================================
Compiling platform: Red Hat Enterprise Linux AS release 4 (Nahant Update 5)
GNU Make versions:  3.80
GCC     : 3.4.6
Binutils: 2.15.92.0.2
Glibc   : 3.4.6


============ WAG320N source codes ==========================================
WAG320N_v1.00.12-A_GPL.tgz

	Folder           									Description                   
	-----------------------  							------------------------------------------------
	WAG320N_v1.00.12-A_GPL/bcm963xx_4.02L.01			Broadcom BSP including Linux Kernel
	WAG320N_v1.00.12-A_GPL/kernel						A Folder linked into Linux Kernel
	WAG320N_v1.00.12-A_GPL/uclibc-crosstools-3.4.2		ToolChains
	WAG320N_v1.00.12-A_GPL/tk_apps   					Application 
	WAG320N_v1.00.12-A_GPL/tools/      					Some utilities that used to create FW image
	WAG320N_v1.00.12-A_GPL/Makefile       				A makefile to build "Linux_Kernel" and "File_System" 
	WAG320N_v1.00.12-A_GPL/target.tgz     				A compressed file that includes files for the root filesystem.
	WAG320N_v1.00.12-A_GPL/image/         				Binaries of "Loader", "Linux_Kernel" and "File_System" are put here.
		     	  										Also include a tool to combine "Loader", "Linux_Kernel" and "File_System".

============ WAG320N FW binarybuild procedure ===========
1. Install WAG320N source codes and change to the source code folder
   Un-tar "WAG320N_v1.00.12-A_GPL.tgz" to a source code folder, and enter the source code folder.
   	
2. Build F/W binrary
	#make auto
	image/wag320n_A_1.00.12.bin is the F/W binary, size is 8.0MB.
	
3. Build the toolchains
	#cd uclibc-crosstools-3.4.2
	#./build_all.sh
	The built toolchain exists in folder "uclibc-crosstools-3.4.2/BUILD/uclibc-crosstools-3.4.2/".


