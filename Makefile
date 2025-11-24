CC = gcc
#CFLAGS = -Wall -Wextra -g
CONTROLADOR = controlador
CLIENTE = cliente
VEICULO = veiculo

OBJS = controlador.o cliente.o veiculo.o


all: $(CONTROLADOR) $(CLIENTE) $(VEICULO)

# Compilar objectos
ontrolador.o: controlador.c Settings.h
	$(CC) $(CFLAGS) -c controlador.c -o controlador.o

cliente.o: cliente.c Settings.h
	$(CC) $(CFLAGS) -c cliente.c -o cliente.o

veiculo.o: veiculo.c Settings.h
	$(CC) $(CFLAGS) -c veiculo.c -o veiculo.o


# Criar executáveis
$(CONTROLADOR): controlador.o
	$(CC) controlador.o -o $(CONTROLADOR)

$(CLIENT): cliente.o
	$(CC) cliente.o -o $(CLIENTE)

$(VEICULO): veiculo.o
	$(CC) veiculo.o -o $(VEICULO)

# Executar controlador com variável de ambiente
run-server: $(CONTROLADOR)
	@echo "Starting server with NVEICULOS=4..."
	NVEICULOS=4 ./controlador

# Executar cliente
run-client: $(CLIENTE)
ifndef nome
	$(error Uso correto: make run-client nome=<username>)
endif
	@echo "Running client '$(nome)'..."
	./$(CLIENTE) $(nome)

clean:
	rm -f *.o $(CONTROLADOR) $(CLIENTE) $(VEICULO) fifo_* fifo*



