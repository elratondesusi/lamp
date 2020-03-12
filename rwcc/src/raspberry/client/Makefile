FLAGS=-g

all: lampa test_connection

lampa: hw.c lampa.c
	gcc -o lampa $^ -lpthread $(FLAGS)

test_connection: hw.c test_connection.c
	gcc -o test_connection $^ $(FLAGS) -lpthread

clean:
	rm -f lampa test_connection

