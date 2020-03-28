#replace g++-9 with your compiler
cc = g++
flags = -std=c++11 -pthread
all: clean NQueens


NQueens: 
	$(cc) $(flags) nqueens.cpp hermes-wps.c cotton-runtime.cpp dequeue.cpp  -o NQueens -lpthread


clean:
	rm -f NQueens
