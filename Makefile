CC = gcc
CFLAGS = -I./include -I./lib -Wall
LIBS = -lcurl -lsqlite3

all: flight_pipeline

flight_pipeline: src/main.c lib/cJSON.c src/database.c src/metrics.c
	$(CC) $(CFLAGS) src/main.c lib/cJSON.c src/database.c src/metrics.c $(LIBS) -o flight_pipeline

clean:
	rm -f flight_pipeline

run: flight_pipeline
	./flight_pipeline