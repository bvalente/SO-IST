# Makefile, versao 2
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS = -g -Wall -pedantic
CC = gcc

all: heatSim_p2 heatSim_p0

heatSim_p2: p2_main.o matrix2d.o mplib4.o leQueue.o leQueue.h mplib4.h
	$(CC) $(CFLAGS) -o heatSim_p2 p2_main.o matrix2d.o mplib4.o leQueue.o

heatSim_p0: p0_main.o matrix2d.o leQueue.o
	$(CC) $(CFLAGS) -o heatSim_p0 p0_main.o matrix2d.o  leQueue.o

p2_main.o : p2_main.c
	$(CC) $(CFLAGS) -pthread -c p2_main.c

p0_main.o : p0_main.c
	$(CC) $(CFLAGS) -pthread -c p0_main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

leQueue.o: leQueue.c leQueue.h
	$(CC) $(CFLAGS) -c leQueue.c

mplib4.o: mplib4.c mplib4.h leQueue.h
	$(CC) $(CFLAGS) -c mplib4.c

clean:
	rm -f *.o heatSim_p2 heatSim_p0

zip:
	zip heatSim_p2_solucao.zip p2_main.c matrix2d.c matrix2d.h leQueue.c leQueue.h mplib4.c mplib4.h p0_main.c Makefile

run:
	./heatSim_p2 10 10.0 10.0 0.0 0.0 10 2 2
