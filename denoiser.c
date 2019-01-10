/*

Student Name: Seyfi Kutay Kılıç
Student Number: 2015400042
Compile Status: Compiling
Program Status: Working
Notes: 
I implemented the project using the second approach.
My program first initializes some useful variables; such as
world_size, slave_size, slavePerDimension, lengthPerProcess; to use
in master and slave processes. Then,

The master process do:
    1)Read from the input file and fill the image matrix.
    2)Divide the matrix into <number of slaves> parts
    and send them the associated parts(line by line).
    3)Wait for the slave processes to send the processed 
    submatrixes and merge them.
    4)Print the whole processed matrix into the output file.

The slave processes do:
    1) Initialize beta, pi, and gamma.
    2) Intialize the necessary varaibles and arrays
    that will help in the flipping iterations
    3) Iterate the flipping loop 1M/|slaves| time, 
    doing:
        3-a) Send the most left and right columns to 
        left and right neighboor processes, most upper 
        and lower rows to upper and lower neighboor processes
        (to the existing ones).
        3-b) Receive the most left and right columns from 
        left and right neighboor processes, most upper 
        and lower rows from upper and lower neighboor processes
        (from the existing ones).
        3-c) Select a random pixel by picking random row and 
        column indexes.
        3-d) Sum all the neighboor pixels of the selected random 
        pixel.
        3-e) Calculate the acceptance probability, but without 
        taking the exponential. 
        (calculate the logarithm of the acceptance probability) 
        3-f) Select a random number between 0-1, then compare the 
        logarithm of it with the acceptance probability; to decide 
        whether or not to flip.
        3-g) If it is decided than flip the selected random pixel.
    4) Send the processesed submatrix to the master process.

*/


#include <mpi.h>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h> 
#include <math.h>
#include <string.h>

int N=200; //200x200 matrix

int randomDecide(double logOfProbability){
    
    double randomNum=rand()/((double) RAND_MAX ); //a random number between 0 and 1

    if(log(randomNum)<logOfProbability) return 1; //decide to flip
    else return 0;
}

int randomIndex(int end){

    int randomNum=rand()%(end+1); //a random number between 0 to end

    return randomNum;
}

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int slave_size=world_size-1;
    int slavePerDimension=(int)sqrt(slave_size); //suppose the number of slaves is a square of an integer

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int lengthPerProcess=N/slavePerDimension; //the length of matrix that is each slave process are responsible 

    srand(time(NULL)); //to handle random operations


    if(rank==0){ //for the master process


        int** X=(int**)malloc(sizeof(int*)*N);
        for(int i=0; i<N; i++){
            X[i]=(int*)malloc(sizeof(int)*N);
        }

        FILE* fileToRead = fopen(argv[1], "r");

        char line[N*3];
        for(int i=0;i<N;i++){ //fill the matrix X from the file

            fgets(line, N*3, fileToRead);

            X[i][0]=atoi(strtok(line, " "));
            for(int j=1; j<N; j++){
                X[i][j]=atoi(strtok(NULL, " "));
            }
        }

        fclose(fileToRead);


        for(int i=1; i<=slave_size; i++){ //send data to all the slaves
            int startRow=lengthPerProcess*((i-1)/slavePerDimension);
            int startColumn=lengthPerProcess*((i-1)%slavePerDimension);
            //send the associated sub matrix to a slave process, line by line:
            for(int j=0; j<lengthPerProcess; j++){
                int line[lengthPerProcess];
                memcpy(line, &X[startRow+j][startColumn], sizeof(int)*lengthPerProcess);
                MPI_Send(line, lengthPerProcess, MPI_INT, i, j, MPI_COMM_WORLD); //send the line
            }

        }


        int** Z=(int**)malloc(sizeof(int*)*N);
        for(int i=0; i<N; i++){
            Z[i]=(int*)malloc(sizeof(int)*N);
        }


        for(int i=1; i<=slave_size; i++){ //receive data from all the slaves
            int startLineOfZ=lengthPerProcess*((i-1)/slavePerDimension);
            int startColumnOfZ=lengthPerProcess*((i-1)%slavePerDimension);

            for(int j=0; j<lengthPerProcess; j++){
                MPI_Recv(&Z[startLineOfZ+j][startColumnOfZ], lengthPerProcess, MPI_INT, i, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        FILE* fileToWrite=fopen(argv[2], "w"); //create the output file

        // Print the processed matrix to the output file:
        for(int i=0; i<N; i++){
            for(int j=0; j<N; j++){
                fprintf(fileToWrite, "%d", Z[i][j]);
                fprintf(fileToWrite, " ");
            }
            fprintf(fileToWrite, "\n");
        }

        fclose(fileToWrite);


    }

    else{ //for the slave process

        double B = atof(argv[3]);
        double pi = atof(argv[4]);  
        double gamma=log((1-pi)/pi)/2.0;

        int beginRow=lengthPerProcess*((rank-1)/slavePerDimension);//begin row
        int endRow=beginRow+lengthPerProcess-1; //end row
        int lastRowIndex=lengthPerProcess-1;

        int beginColumn=lengthPerProcess*((rank-1)%slavePerDimension);//begin column
        int endColumn=beginColumn+lengthPerProcess-1; //end column
        int lastColumnIndex=lengthPerProcess-1;

        int hasUpperNeighboor=(beginRow>0);
        int hasLowerNeighboor=(endRow<N-1);
        int hasLeftNeighboor=(beginColumn>0);
        int hasRightNeighboor=(endColumn<N-1);

        int upperLine[lengthPerProcess];
        int lowerLine[lengthPerProcess];
        int leftLine[lengthPerProcess];
        int rightLine[lengthPerProcess];

        //initialy all -1(indicates that not exist)
        int leftUp=-1;
        int rightUp=-1;
        int leftBottom=-1;
        int rightBottom=-1;


        int subX[lengthPerProcess][lengthPerProcess];
        
        for(int i=0; i<lengthPerProcess; i++){
            MPI_Recv(subX[i], lengthPerProcess, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        int subZ[lengthPerProcess][lengthPerProcess];
        
        for(int i=0; i<lengthPerProcess; i++){ //deep copy subX to subZ
            for(int j=0; j<lengthPerProcess; j++){
                subZ[i][j]=subX[i][j];
            }
        }


        int iterations=1000000/slave_size;

        for(int count=0; count<iterations; count++){ //the iteration loop
           
            //Necessary Send Operations:
            if(hasUpperNeighboor){
                MPI_Send(subZ[0], lengthPerProcess, MPI_INT, rank-slavePerDimension, 0, MPI_COMM_WORLD); //send the first line of subZ to the upper neighboor
                //if there is upperleft neighboor:
                if(hasLeftNeighboor) MPI_Send(&subZ[0][0], 1, MPI_INT, rank-slavePerDimension-1, 0, MPI_COMM_WORLD);
                //if there is upperright neighboor:
                if(hasRightNeighboor) MPI_Send(&subZ[0][lastColumnIndex], 1, MPI_INT, rank-slavePerDimension+1, 0, MPI_COMM_WORLD);  
            }
            if(hasLowerNeighboor){
                MPI_Send(subZ[lastRowIndex], lengthPerProcess, MPI_INT, rank+slavePerDimension, 0, MPI_COMM_WORLD); //send the last line of subZ to the lower neighboor 
                //if there is lowerleft neighboor:
                if(hasLeftNeighboor) MPI_Send(&subZ[lastRowIndex][0], 1, MPI_INT, rank+slavePerDimension-1, 0, MPI_COMM_WORLD); 
                //if there is lowerright neighboor:
                if(hasRightNeighboor) MPI_Send(&subZ[lastRowIndex][lastColumnIndex], 1, MPI_INT, rank+slavePerDimension+1, 0, MPI_COMM_WORLD); 
            }

            if(hasLeftNeighboor){
                int leftColumn[lengthPerProcess];
                for(int i=0; i<lastRowIndex; i++){
                    leftColumn[i]=subZ[i][0];
                }
                MPI_Send(leftColumn, lengthPerProcess, MPI_INT, rank-1, 0, MPI_COMM_WORLD); //send the first column of subZ to the left neighboor 
            }
            if(hasRightNeighboor){
                int rightColumn[lengthPerProcess];
                for(int i=0; i<lastRowIndex; i++){
                    rightColumn[i]=subZ[i][lastColumnIndex];
                }
                MPI_Send(rightColumn, lengthPerProcess, MPI_INT, rank+1, 0, MPI_COMM_WORLD); //send the last column of subZ to the right neighboor 
            }


            //Necessary Receive Operations:
            if(hasUpperNeighboor){
                MPI_Recv(upperLine, lengthPerProcess, MPI_INT, rank-slavePerDimension, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //if there is upperleft neighboor:
                if(hasLeftNeighboor) MPI_Recv(&leftUp, 1, MPI_INT, rank-slavePerDimension-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //if there is upperright neighboor:
                if(hasRightNeighboor) MPI_Recv(&rightUp, 1, MPI_INT, rank-slavePerDimension+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
            }
            if(hasLowerNeighboor){
                MPI_Recv(lowerLine, lengthPerProcess, MPI_INT, rank+slavePerDimension, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //if there is lowerleft neighboor:
                if(hasLeftNeighboor) MPI_Recv(&leftBottom, 1, MPI_INT, rank+slavePerDimension-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
                //if there is lowerright neighboor:
                if(hasRightNeighboor) MPI_Recv(&rightBottom, 1, MPI_INT, rank+slavePerDimension+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
            }

            if(hasLeftNeighboor){
                MPI_Recv(leftLine, lengthPerProcess, MPI_INT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if(hasRightNeighboor){
                MPI_Recv(rightLine, lengthPerProcess, MPI_INT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }




            int randomRow=randomIndex(lastRowIndex);
            int randomColumn=randomIndex(lastColumnIndex);

            
            /// calculate 

            int sumOfNeighboors=0;
            for(int i=randomRow-1; i<=randomRow+1; i++){ //search every neighboor pixels
                for(int j=randomColumn-1; j<=randomColumn+1; j++){ 
                    //if the pixel in subZ
                    if(i<=lastRowIndex && i>=0 && j<=lastColumnIndex && j>=0){
                        sumOfNeighboors+=subZ[i][j];
                    }
                    //if the pixel outside of subZ
                    else{
                        if(i<0 && hasUpperNeighboor){ //pixel from an upper process
                            //the pixel from upper left process:
                            if(j<0 && hasLeftNeighboor) sumOfNeighboors+=leftUp;
                            //the pixel from upper right process:
                            else if(j>lastColumnIndex && hasRightNeighboor) sumOfNeighboors+=rightUp;
                            //a pixel from the upper process:
                            else sumOfNeighboors+=upperLine[j];
                        }
                        else if(i>lastRowIndex && hasLowerNeighboor){ //pixel from a lower process
                            //the pixel from lower left process:
                            if(j<0 && hasLeftNeighboor) sumOfNeighboors+=leftBottom;
                            //the pixel from lower right process:
                            else if(j>lastColumnIndex && hasRightNeighboor) sumOfNeighboors+=leftUp;
                            //a pixel from the lower process:
                            else sumOfNeighboors+=lowerLine[j];
                        }
                        else if(j<0 && hasLeftNeighboor){ //pixel from the left process
                            sumOfNeighboors+=leftLine[i];
                        }
                        else if(j>lastColumnIndex && hasRightNeighboor){  //pixel from the right process
                            sumOfNeighboors+=rightLine[i];
                        }
                        //else: the pixel is not exist.
                    }


                }
            }

           
            sumOfNeighboors=sumOfNeighboors-subZ[randomRow][randomColumn]; //substract itself because it is not a neighboor
            int pixelFromNoisy=subX[randomRow][randomColumn];
            int pixelFromCurrentZ=subZ[randomRow][randomColumn];

            double logOfAcceptance=(-2.0*gamma*pixelFromCurrentZ*pixelFromNoisy)
                                    -(2.0*B*pixelFromCurrentZ*sumOfNeighboors);

            if(randomDecide(logOfAcceptance)){
                subZ[randomRow][randomColumn]=-1*subZ[randomRow][randomColumn]; //flip
            }
                
            

        }

        //send the lines of subZ
        for(int i=0; i<lengthPerProcess; i++){
            MPI_Send(subZ[i], lengthPerProcess, MPI_INT, 0, i, MPI_COMM_WORLD); 
        }
        

    }

    // Finalize the MPI environment.
    MPI_Finalize();
}