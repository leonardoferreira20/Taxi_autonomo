CC = gcc

all: controlador cliente veiculo

controlador: controlador.c Settings.h
	$(CC) -pthread controlador.c -o controlador

cliente: cliente.c Settings.h
	$(CC) -pthread cliente.c -o cliente

veiculo: veiculo.c Settings.h
	$(CC) veiculo.c -o veiculo

clean:
	rm -f controlador cliente veiculo *.o fifo_* fifo* cli_*