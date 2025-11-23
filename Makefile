CC = gcc
CFLAGS = -Wall -Wextra -g

all: controlador cliente veiculo

controlador: controlador.c
	$(CC) $(CFLAGS) controlador.c -o controlador

cliente: cliente.c
	$(CC) $(CFLAGS) cliente.c -o cliente

veiculo: veiculo.c
	$(CC) $(CFLAGS) veiculo.c -o veiculo

clean:
	@echo "Limpando ficheiros..."
	rm -f *.o controlador cliente veiculo
	rm -f /tmp/controlador_fifo /tmp/cli_*
	@echo "Limpo!"

run_controlador: controlador
	./controlador

test: all
	@echo "Compilação bem-sucedida!"

.PHONY: all clean run_controlador test