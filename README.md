# Instrumentor
A very handy C++ profiler grab from [The Cherno](https://www.youtube.com/@TheCherno) C++ series.

## Usage
Includes `instrumentor.h` and call `Instrumentor::BeginSession` function.  
Defining `PROFILING` before including `instrumentor.h` enables some useful macros.
```cpp
#define PROFILING
#include "instrumentor.h"

int main() {
    // The session name will become the filename of the output json file.
    Instrumentor::BeginSession("test");
    {
        // Use this macro in any function or method to record.
        PROFILE_FUNC(); 
        printf("Hello, world!\n");
    }
    return 0;
}
```

## References
1. [VISUAL BENCHMARKING in C++ (how to measure performance visually) - The Cherno](https://youtu.be/xlAH4dbMVnU)
2. [GavinSun0921/InstrumentorTimer](https://github.com/GavinSun0921/InstrumentorTimer)