#ifndef ULTRASONIC_H
#define ULTRASONIC_H

int init_ultrasonic(void);

/* Trigger the sensor and return distance in cm, or -1 for timeout */
int read_ultrasonic_syncronous(void);

#endif /* ULTRASONIC_H */
