#ifndef METRICS_H
#define METRICS_H

#include <time.h>


typedef struct {
    int total_fetched;
    int new_records;
    int duplicates;
    double api_latency_ms;
    double write_latency_ms;
    time_t batch_timestamp;
    int batch_number;
    
} BatchMetrics;


void print_batch_metrics(BatchMetrics *metrics);

void init_metrics(BatchMetrics *metrics, int batch_num);


#endif