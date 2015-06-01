CC=gcc
CFLAGS= -g -Wall -std=gnu99
LFLAGS= -pthread -lrt -lgps
SRC=./src
INCLUDE = -I./inc -I/opt/gpsd/
#OBJ = $(SRC)/cloud.o $(SRC)/src/list.o $(SRC)/src/gpxlogger.o $(SRC)/src/lcd.o ./src/sensorNode.o ./src/coordinator.o
OBJ  = $(SRC)/cloud.o $(SRC)/list.o $(SRC)/gpxlogger.o 
OBJ += $(SRC)/lcd.o $(SRC)/sensorNode.o $(SRC)/coordinator.o
TARGET = manage

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $^ $(LFLAGS)

clean:
	rm ./${TARGET} $(SRC)/*.o
