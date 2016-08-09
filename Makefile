CC=clang++
CSTD=-std=c++14
CWARN=-Wall -Wextra -pedantic
CEXTRA=-g

CFLAGS=$(CSTD) $(CWARN) $(CEXTRA)
LDFLAGS=-lgnutls -lopendht -lreadline

EXEC=dhtim

all: $(EXEC)

dhtim:
	$(CC) $(LDFLAGS) $(CFLAGS) dhtim.cpp -o dhtim

clean:
	rm -v -f *.o *.gch *.gch.*

mr_proper: clean
	rm -v -f *.out
	rm -v -f $(EXEC)
