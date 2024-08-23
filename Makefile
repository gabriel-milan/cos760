CC = g++
MPICC = mpicxx
CFLAGS = -Wall -O2
OMPFLAGS = -fopenmp
TARGETS = serial mpi openmp

all: $(TARGETS)

serial: src/serial.cpp
	$(CC) $(CFLAGS) -o $@ $<

mpi: src/mpi.cpp
	$(MPICC) $(CFLAGS) -o $@ $<

openmp: src/openmp.cpp
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)