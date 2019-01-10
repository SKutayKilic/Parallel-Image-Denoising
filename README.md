## Parallel-Image-Denoising
A parallel image denoising program that uses Ising Model and Metropolis-Hasting Algorithm.

Used C and MPI for the main program.
Python for the scripts.

# 1)To convert a png file into an input file: 
  python image_to_text.py <image-file> <input-file>
2)To compile the program:
  mpicc -g denoiser.c -o <executable-name> -lm
3)To execute the program:
  mpiexec –n <Number of processors> ./<executable-name> <input-file> <output-file> <Beta-value> <P-Value>
4)To convert the output file into a png file: 
  python text_to_image.py <output-file> <image-file>
