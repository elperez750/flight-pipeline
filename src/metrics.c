#include <stdio.h>
#include <time.h>
#include "metrics.h"

/*

typedef struct {
    int total_fetched;
    int new_records;
    int duplicates;
    double api_latency_ms;
    double write_latency_ms;
    time_t batch_timestamp;
    int batch_number;
    
} BatchMetrics;



*/


void init_metrics(BatchMetrics *metrics, int batch_num) {
    metrics->total_fetched = 0;
    metrics->new_records = 0;
    metrics->duplicates = 0;
    metrics->api_latency_ms = 0.0;
    metrics->write_latency_ms = 0.0;
    metrics->batch_timestamp = time(NULL);
    metrics->batch_number = batch_num;
}


void print_batch_metrics(BatchMetrics *metrics) {
    char timestamp[20];
    struct tm *timeinfo = localtime(&metrics->batch_timestamp);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

      printf("[%s] Batch #%d | Fetched: %d | New: %d | Dupes: %d | API: %.0fms | DB: %.0fms\n",
           timestamp,
           metrics->batch_number,
           metrics->total_fetched,
           metrics->new_records,
           metrics->duplicates,
           metrics->api_latency_ms,
           metrics->write_latency_ms);
}