## Parallel-Image-Denoising
A parallel image denoising program that uses Ising Model and Metropolis-Hasting Algorithm.<br />
<br />
Used C and MPI for the main program.<br />
Python for the scripts.<br />
<br />
1)To convert a png file into an input file: <br />
&nbsp;&nbsp;&nbsp; python image_to_text.py <image-file> <input-file> <br />
2)To compile the program: <br />
&nbsp;&nbsp;&nbsp; mpicc -g denoiser.c -o <executable-name> -lm <br />
3)To execute the program: <br />
&nbsp;&nbsp;&nbsp; mpiexec –n <Number of processors> ./<executable-name> <input-file> <output-file> <Beta-value> <P-Value> <br />
4)To convert the output file into a png file: <br />
&nbsp;&nbsp;&nbsp; python text_to_image.py <output-file> <image-file> <br />
