#ifndef __HRTIME_H__
#define __HRTIME_H__

// gethrtime implementation by Kai Shen for x86 Linux

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// get the number of CPU cycles per nanosecond from Linux /proc filesystem
// return < 0 on error
inline  static
double getGHZ_x86(void)
{
#ifdef plum
    return 2.6666;
#else
    double mhz = -1;
    char line[1024], *s, search_str[] = "cpu MHz";
    FILE* fp;

    // open proc/cpuinfo
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
        return -1;

    // ignore all lines until we reach MHz information
    while (fgets(line, 1024, fp) != NULL) {
        if (strstr(line, search_str) != NULL) {
            // ignore all characters in line up to :
            for (s = line; *s && (*s != ':'); ++s);
            // get MHz number
            if (*s && (sscanf(s+1, "%lf", &mhz) == 1))
                break;
        }
    }

    if (fp != NULL)        fclose(fp);
    return mhz / 1000;
#endif
}


// Get the number of CPU cycles since startup using rdtsc instruction
static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline unsigned long long get_c()
{
    unsigned int tmp[2];

    asm volatile ("rdtsc" : "=a" (tmp[1]), "=d" (tmp[0]) : "c" (0x10) );
    return ( ( ((unsigned long long)tmp[0]) << 32 ) | tmp[1]);
}




// get the elapsed time (in nanoseconds) since startup
inline static
unsigned long long get_t()
{
    static bool once = false;
    static double CPU_GHZ;
    if (once == false)
    {
        CPU_GHZ = getGHZ_x86();
        once = true;
    }
    return (unsigned long long)(get_c() / CPU_GHZ);
}


// get the elapsed time (in miliseconds) since startup
static
#ifdef INTEL_TM
[[transaction_pure]]
#endif
inline unsigned long long get_tm()
{
    return (get_t() / 1000000);
}

// Conversion from cycles to seconds
static inline double c_to_t( unsigned long long cycles )
{
    static bool once = false;
    static double CPU_GHZ;
    if (once == false)
    {
        CPU_GHZ = getGHZ_x86();
        once = true;
    }
    return (double)( cycles / (CPU_GHZ * 1000000000) );
}

#endif // __HRTIME_H__
