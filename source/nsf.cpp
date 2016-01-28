
// NSF Player
//

#include "apu/nsf.h"

using namespace apu;


// Sequence and Channel names
#define SQUARE1  0
#define SQUARE2  1
#define TRIANGLE 2
#define NOISE    3
#define DCPM     4

#define VOLUME   0
#define ARPEGGIO 1
#define PITGH    2
#define HIPITCH  3
#define DUTY     4


// Offset calculation for 16-bit NES address lookups
inline uint8_t *NSF::lookup(uint8_t *addr, unsigned off) {
    return &_data[addr[2*off] | (addr[2*off + 1] << 8)];
}


// Channel lifetime
NSF::Channel::Channel(NSF *nsf, apu::Channel *channel)
    : _channel(channel), _nsf(nsf) {
}

// Loads channel setup
void NSF::Channel::frame(uint8_t *frame) {
    _cmds = frame;
    _tick = 0;
    _delay = 0;
    _pdelay = 0xff;
}

void NSF::Channel::sequence(uint8_t *inst) {
    uint8_t mask = *inst++;

    for (unsigned i = 0; i < NSF_SEQUENCES; i++) {
        if (mask & (1 << i)) {
            uint8_t *seq = _nsf->lookup(inst, 0);
            inst += 2;

            _seq[i].count = *seq;
            _seq[i].data = seq + 4;
        } else {
            _seq[i].count = 0;
            _seq[i].data = 0;
        }
    }
}

// Channel updates
void NSF::Channel::exec() {
    // Check delay
    if (_delay) {
        _delay -= 1;
        return;
    }

    // Execute commands
    uint8_t cmd;

    do {
        cmd = *_cmds++;

        if ((cmd & 0x80) == 0x00) { // notes
            if (cmd == 0x00) {
                // pass
            } else if (cmd == 0x7f) {
                _channel->disable();
                _tick = 0xff;
            } else {
                _channel->note(cmd-1);
                _tick = 0x00;
            }
        } else if (cmd < 0xb0) { // other commands
            uint8_t arg = *_cmds++;

            switch (cmd) {
                case 0x80: // change instrument
                    sequence(_nsf->lookup(_nsf->_insts, arg));
                    break;

                case 0x82: // change speed
                    _nsf->_tick = 0;
                    _nsf->_tick_count = arg;
                    break;

                case 0x84: // jump to frame
                    _nsf->_pattern = _nsf->_pattern_count;
                    _nsf->_frame = arg;
                    _pdelay = 1;
                    break;

                case 0x86: // skip frame
                    _nsf->_pattern = _nsf->_pattern_count;
                    break;

                case 0x88: // halt
                    _nsf->stop();
                    break;

                case 0x8a: // set volume
                    _channel->volume(arg);
                    break;

                case 0x8c: // set portamento
                case 0x8e: // set port up
                case 0x90: // set port down
                case 0x92: // sweep
                case 0x94: // arpeggio
                case 0x96: // vibrato
                case 0x98: // tremelo
                case 0x9a: // pitch
                case 0x9c: // delay
                case 0x9e: // dac
                    break;

                case 0xa0: // duty
                    _channel->duty(arg);
                    break;

                case 0xa2: // offset
                case 0xa4: // slide up
                case 0xa6: // slide down
                 case 0xa8: // volume slide
                    break;

                case 0xaa: // note cut
                    _cut = arg;
                    break;

                case 0xac: // retrigger
                case 0xae: // dpcm pitch
                default:
                    break;
            }
        } else if ((cmd & 0xf0) == 0xe0) { // change instrument
            sequence(_nsf->lookup(_nsf->_insts, cmd & 0xf));

        } else if ((cmd & 0xf0) == 0xf0) { // volume change
            _channel->volume(cmd & 0xf);

        } else if (cmd == 0xb0) { // set delay
            _pdelay = *_cmds++;

        } else if (cmd == 0xb2) { // reset delay
            _pdelay = 0xff;

        }
    } while (cmd & 0x80);

    // Delay some amount
    if (_pdelay == 0xff) {
        _delay = *_cmds++;
    } else {
        _delay = _pdelay;
    }
}

// Step NSF engine
void NSF::Channel::tick() {
    if (_cut && !(--_cut)) {
        _channel->disable();
    }

    if (_tick < 0xff) {
        if (_tick < _seq[VOLUME].count) {
            _channel->volume(_seq[VOLUME].data[_tick]);
        }

        if (_tick < _seq[DUTY].count) {
            _channel->duty(_seq[DUTY].data[_tick]);
        }
    }

    _tick++;
}


// NSF lifetime
NSF::NSF(PinName pin)
        : _apu{
            Square(), Square(), Triangle(), Noise(), 
            {&_apu.square1, &_apu.square2, &_apu.triangle, &_apu.noise},
            APU(_apu.channels, NSF_CHANNELS, pin)
          },
          _channels{
            Channel(this, &_apu.square1),
            Channel(this, &_apu.square2),
            Channel(this, &_apu.triangle),
            Channel(this, &_apu.noise),
          },
          _handle(0) {
}

// Loads a compiled NSF file
void NSF::load(uint8_t *data, int song) {
    _data = data;

    // Get the song info
    uint8_t *info = lookup(lookup(data, 0), song);

    _frames = lookup(info, 0);
    _insts = lookup(_data, 1);

    _frame   = _frame_count   = info[2];
    _pattern = _pattern_count = info[3];
    _tick    = _tick_count    = info[4];
}

// Step NSF engine
void NSF::tick() {
    // tick interval
    if (_tick == _tick_count) {
        _tick = 0;

        // pattern interval
        if (_pattern == _pattern_count) {
            _pattern = 0;

            // frame interval
            if (_frame == _frame_count) {
                _frame = 0;
            }

            // setup next frame
            for (unsigned i = 0; i < NSF_CHANNELS; i++) {
                _channels[i].frame(lookup(lookup(_frames, _frame), i));
            }

            _frame++;
        }

        _pattern++;

        // run commands
        for (unsigned i = 0; i < NSF_CHANNELS; i++) {
            _channels[i].exec();
        }
    }

    _tick++;

    // update channels
    for (unsigned i = 0; i < NSF_CHANNELS; i++) {
        _channels[i].tick();
    }
}

// Starting/stopping the player
void NSF::start() {
    if (_handle) stop();

    _apu.apu.start();
    _handle = minar::Scheduler::postCallback(this, &NSF::tick)
            .period(minar::milliseconds(1000000/NSF_FREQ))
            .getHandle();
}

void NSF::stop() {
    _apu.apu.stop();
    minar::Scheduler::cancelCallback(_handle);
    _handle = 0;
}


