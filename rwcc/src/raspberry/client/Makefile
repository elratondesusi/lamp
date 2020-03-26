FLAGS=-g

all: lampa test_connection test_hw

lampa: hw.c lampa.c
	gcc -o lampa $^ -lpthread $(FLAGS)

test_connection: hw.c test_connection.c
	gcc -o test_connection $^ $(FLAGS) -lpthread

test_hw: hw.c test_hw.c
	gcc -o test_hw $^ $(FLAGS) -lpthread

clean:
	rm -f lampa test_connection test_hw

