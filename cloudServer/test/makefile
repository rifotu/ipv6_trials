CC=gcc
CFLAGS= -g -Wall -std=gnu99
LFLAGS= -lrt 
SRC=.
INCLUDE = -I../inc 
OBJ  = $(SRC)/list.o $(SRC)/client2.o 
TARGET = test-client

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $^ $(LFLAGS)

clean:
	rm ./${TARGET} $(SRC)/*.o
