all: controlador cliente

# Compilar objectos
controlador.o: controlador.c Settings.h
	gcc -c controlador.c -o controlador.o

cliente.o: cliente.c Settings.h
	gcc -c cliente.c -o cliente.o

# Criar executáveis
controlador: controlador.o
	@echo $(PASSFILE)
	gcc controlador.o -o controlador

cliente: cliente.o
	gcc cliente.o -o cliente

# Executar controlador
run-server: controlador
	@echo "Starting the server..."
	./controlador

# Executar cliente
run-client: cliente
	@echo "Running the client..."
	./cliente

# Limpar ficheiros
clean:
	rm -f *.o controlador cliente fifo_* fifo* 