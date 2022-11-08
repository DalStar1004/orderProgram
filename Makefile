
all : orderLotteria

orderLotteria.o : orderLotteria.c 
	$(CC) -c $< -lmysqlclient

orderLotteria : orderLotteria.o
	$(CC) -o orderLotteria $< -lmysqlclient

clean : 
	rm *o
	rm orderLotteria
