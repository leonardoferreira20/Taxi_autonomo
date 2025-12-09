CC = gcc
#CFLAGS = -Wall -Wextra -g
CONTROLADOR = controlador
CLIENTE = cliente
VEICULO = veiculo

OBJS = controlador.o cliente.o veiculo.o

all: $(CONTROLADOR) $(CLIENTE) $(VEICULO)

# Compilar objectos
controlador.o: controlador.c Settings.h
	$(CC) $(CFLAGS) -c controlador.c -o controlador.o

cliente.o: cliente.c Settings.h
	$(CC) $(CFLAGS) -c cliente.c -o cliente.o

veiculo.o: veiculo.c Settings.h
	$(CC) $(CFLAGS) -c veiculo.c -o veiculo.o


# Criar executáveis
$(CONTROLADOR): controlador.o
	$(CC) controlador.o -o $(CONTROLADOR)

$(CLIENTE): cliente.o
	$(CC) cliente.o -o $(CLIENTE)

$(VEICULO): veiculo.o
	$(CC) veiculo.o -o $(VEICULO)

# Executar cliente
run-client: $(CLIENTE)
ifndef nome
	$(error Uso correto: make run-client nome=<username>)
endif
	@echo "Running client '$(nome)'..."
	./$(CLIENTE) $(nome)

rebuild-run:
	$(MAKE) clean
	$(MAKE)
	./$(CONTROLADOR)

clean:
	rm -f *.o $(CONTROLADOR) $(CLIENTE) $(VEICULO)
	rm -f fifo_* fifo* cli_*


