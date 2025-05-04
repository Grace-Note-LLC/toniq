#ifndef TDS_H
#define TDS_H
#include <stdint.h>
#include <stdbool.h>
// https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_dt/README.html

bool init_tds(void);
int read_tds(void);

#endif // TDS_H