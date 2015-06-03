CC=gcc
CFLAGS= -g -Wall -std=gnu99
LFLAGS= -lrt 
SRC=./src
INCLUDE = -I./inc 
OBJ  = $(SRC)/list.o $(SRC)/server2.o 
TARGET = cloud 

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $^ $(LFLAGS)

clean:
	rm ./${TARGET} $(SRC)/*.o
