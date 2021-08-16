# lab-dsp-fpga-tp1
Coursework 01 for the DSP laboratory at ITBA. Understanding the Motorola's DSP 56K architecture and programming model.

## Project's directory structure
* **assignment/**: Description of the assignment
* **compiler/**: Contains Motorola's DSP 56002 toolchain
* **src/**: Source code for the DSP using assembly language (*.asm)
* **tools/**: Tools to improve the development experience using the toolchain

## Workflow
Here we describe how to compile, link and run the assembly program in the simulator. Here it is assumed that the assembly source code is located at the **src/**, as it should.
1. Using the command line, go to the **tools/** folder
2. Compile your source code, using **asm source**
3. Link and create the executable, using **lnk source**
4. Run the simulator, using **sim source**
5. Use the **go** or **trace** directives to run the instructions
