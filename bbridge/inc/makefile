CC=gcc
CFLAGS= -g -Wall -std=gnu99
LFLAGS= -pthread -lrt -lgps
INCLUDE = -I../inc -I/opt/gpsd/
OBJ = ./src/cloud.o ./src/list.o ./src/gpxlogger.o ./src/lcd.o ./src/sensorNode.o ./src/coordinator.o
TARGET = manager

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $^ $(LFLAGS)

clean:
	rm ./${TARGET} ./*.o
