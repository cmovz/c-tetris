#ifndef BARRIER_H
#define BARRIER_H

#define barrier() __asm__ __volatile__ ( "" : : : "memory")

#endif