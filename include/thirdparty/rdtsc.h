#ifndef __RDTSC_H_DEFINED__
#define __RDTSC_H_DEFINED__

static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif



// #ifndef RDTSC_H
// #define RDTSC_H
//
// #include <stdint.h>
//
// /**
//  * @brief Lê o contador de ciclos de clock do processador (x86/x86_64). (Gemini)
//  */
// static inline unsigned long long ler_rdtsc(void) {
//     unsigned int lo, hi;
//     // Dispara a instrução rdtsc direto no processador
//     __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
//     return ((unsigned long long)hi << 32) | lo;
// }
//
// #endif // RDTSC_H

