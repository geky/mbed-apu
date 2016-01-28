NES APU Emulator
================
Emulation of the NES Audio Processing Unit.

How to use
----------
This simpliest usage of the APU Emulator is as a simple sound generator.

``` cpp
#include "apu/apu.h"
using namespace apu;

Square square;
Triangle triangle;

Channel channels[] = {
    &square,
    &triangle
};

APU audio(channels, 2, DAC0_OUT);

void app_start(int, char **) {
    square.enable();
    triangle.enable();

    for (unsigned i = 0; i < 80; i++) {
        square.note(i);
        triangle.note(i+4);
        wait(0.5);
    }
}
```

NSF Decoding
------------
In addition to emulating the APU, there is a class for decoding NSF files
that follow the Famitracker format. The NSF decoding and APU emulation
run entirely in the background, allowing other tasks to run.

``` cpp
#include "apu/nsf.h"
using namespace apu;

NSF nsf(DAC0_OUT);

uint8_t nsf_data[] = {
    // Put your NSF data here
};

void app_start(int, char **) {
    nsf.load(nsf_data, 0);
    nsf.start();
}
```

APU Hardware
------------
The APU on the NES is an impressively simple piece of hardware that uses a combination
of several hardware timers and counters to generate square waves, triangle waves, and noise.

You can find all the interesting details of the actual APU over on the
[Nesdev Wiki](http://wiki.nesdev.com/w/index.php/APU).

