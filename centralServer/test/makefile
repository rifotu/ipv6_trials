CC=gcc
CFLAGS= -g -Wall -std=gnu99
LFLAGS= -pthread -lrt -lm
INCLUDE = -I../inc
OBJ =  cJSON.o pack.o cfg.o msgformat.o clientApp.o clientSnmp.o
TARGET1 = clientApp
TARGET2 = clientSnmp

all: $(TARGET1) $(TARGET2)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET1): $(OBJ)
	$(CC) -o $(TARGET1) cJSON.o pack.o cfg.o msgformat.o clientApp.o $(LFLAGS)

$(TARGET2): $(OBJ)
	$(CC) -o $(TARGET2) cJSON.o pack.o cfg.o msgformat.o clientSnmp.o $(LFLAGS)

clean:
	rm ./${TARGET1} ./${TARGET2} ./*.o
