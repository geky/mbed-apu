
// NSF Engine
//

#ifndef NSF_H
#define NSF_H

#include "apu/apu.h"

namespace apu {


// NSF Engine settings
#define NSF_FREQ 60
#define NSF_CHANNELS 4
#define NSF_SEQUENCES 5


// NSF Engine
class NSF {
private:
    // NSF Channel representation
    struct Channel {
        uint8_t *_cmds;

        bool _enabled;
        uint8_t _note;
        uint8_t _volume;
        int8_t _offset;

        uint8_t _delay;
        uint8_t _pdelay;
        uint8_t _cut;

        uint8_t _sweep;
        uint8_t _sweep_div;

        uint8_t _arpeggio;
        uint8_t _arp_count;

        uint8_t _port;
        uint8_t _slide;
        uint16_t _slide_target;

        struct {
            uint8_t *data;
            uint8_t count;
            uint8_t tick;
            uint8_t repeat;
        } _seq[NSF_SEQUENCES];

        apu::Channel *_channel;
        NSF *_nsf; 

        // Channel lifetime
        Channel(apu::Channel *channel);

        // Reset channel
        void reset();

        // Enable/disable channel
        void enable();
        void disable();

        // Loads channel setup
        void frame(uint8_t *frame);
        void sequence(uint8_t *inst);

        // Channel updates
        void exec();
        void tick();
    };

    // NSF Engine state
    uint8_t *_data;
    uint8_t *_frames;
    uint8_t *_insts;

    unsigned _frame;
    unsigned _frame_count;

    unsigned _pattern;
    unsigned _pattern_count;

    unsigned _tick;
    unsigned _tick_count;

    // APU Engine
    struct {
        Square   square1;
        Square   square2;
        Triangle triangle;
        Noise    noise;

        apu::Channel *channels[NSF_CHANNELS];

        APU apu;
    } _apu;

    // Channels and things
    Channel _channels[NSF_CHANNELS];

    minar::callback_handle_t _handle = 0;

    inline uint8_t *lookup(uint8_t *addr, unsigned off);

    void tick();

public:
    // NSF lifetime
    NSF(PinName pin=DAC0_OUT);

    // Loads a compiled NSF file
    void load(uint8_t *data, int song);

    // Starting/stopping the player
    void start();
    void stop();

    // Get the current 6-bit amplitude of the APU
    uint8_t output();
};


}

#endif

