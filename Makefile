CC = gcc
CFLAGS = -I./include -I./lib -Wall
LIBS = -lcurl

all: flight_pipeline

flight_pipeline: src/main.c lib/cJSON.c
	$(CC) $(CFLAGS) src/main.c lib/cJSON.c $(LIBS) -o flight_pipeline

clean:
	rm -f flight_pipeline

run: flight_pipeline
	./flight_pipeline