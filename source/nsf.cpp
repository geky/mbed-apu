
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
#define PITCH    2
#define HIPITCH  3
#define DUTY     4


// Offset calculation for 16-bit NES address lookups
inline uint8_t *NSF::lookup(uint8_t *addr, unsigned off) {
    return &_data[addr[2*off] | (addr[2*off + 1] << 8)];
}


// Channel lifetime
NSF::Channel::Channel(apu::Channel *channel)
  : _channel(channel) {
    reset();
}

// Reset channel
void NSF::Channel::reset(void) {
    // Hacky, but sometimes the simpliest solutions are the best ones
    memset(&_cmds, 0, (uint8_t *)&_channel - (uint8_t *)&_cmds);

    _volume = 0xf;
}

// Enable/disable channel
void NSF::Channel::enable(void) {
    _enabled = true;
    _channel->enable();
}

void NSF::Channel::disable(void) {
    _enabled = false;
    _channel->disable();
}


// Loads channel setup
void NSF::Channel::frame(uint8_t *frame) {
    _cmds = frame;
    _delay = 0;
    _pdelay = 0xff;
}

void NSF::Channel::sequence(uint8_t *inst) {
    uint8_t mask = *inst++;

    for (unsigned i = 0; i < NSF_SEQUENCES; i++) {
        if (mask & (1 << i)) {
            uint8_t *seq = _nsf->lookup(inst, 0);
            inst += 2;

            _seq[i].count = seq[0];
            _seq[i].data = &seq[4];
            _seq[i].tick = 0;
            _seq[i].repeat = seq[1];
        } else {
            _seq[i].count = 0;
            _seq[i].data = 0;
            _seq[i].tick = 0;
            _seq[i].repeat = 0;
        }
    }

    _channel->pitch(0);
    _channel->duty(0);
}


// Channel updates
void NSF::Channel::exec() {
    // Check delay
    if (_delay) {
        _delay--;
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
                _enabled = false;
            } else {
                _note = cmd-1;

                if (!_enabled) {
                    _channel->enable();
                }

                if (!_port || !_enabled) {
                    _channel->pitch(0);
                    _channel->note(_note);
                }

                for (unsigned i = 0; i < NSF_SEQUENCES; i++) {
                    _seq[i].tick = 0;
                }

                _enabled = true;
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
                    _pdelay = 1;
                    break;

                case 0x88: // halt
                    _nsf->stop();
                    break;

                case 0x8a: // set volume
                    _channel->volume(arg);
                    break;

                case 0x8c: // set portamento
                    _slide = 0;
                    _port = arg;
                    break;

                case 0x8e: // set port up
                    _slide = arg;
                    _slide_target = 8;
                    break;

                case 0x90: // set port down
                    _slide = arg;
                    _slide_target = 0x7ff;
                    break;

                case 0x92: // sweep
                    _sweep = arg & 0x7f;
                    _sweep_div = (arg & 0x70) >> 4;

                    if (!_enabled) {
                        _channel->enable();
                        _enabled = true;
                    }

                    _channel->note(_note);
                    break;

                case 0x94: // arpeggio
                    _arpeggio = arg;
                    _arp_count = 0;
                    break;

                case 0x9a: // pitch
                    _channel->pitch(((int16_t)arg) - 0x80);
                    break;

                case 0xa0: // duty
                    _channel->duty(arg);
                    break;

                case 0xa4: // slide up
                    _note += arg & 0xf;
                    _slide = 2*(arg >> 4) + 1;
                    _slide_target = _channel->to_period(_note);
                    break;

                case 0xa6: // slide down
                    _note -= arg & 0xf;
                    _slide = 2*(arg >> 4) + 1;
                    _slide_target = _channel->to_period(_note);
                    break;

                case 0xaa: // note cut
                    _cut = arg;
                    break;

                case 0x96: // vibrato
                case 0x98: // tremelo
                case 0x9c: // delay
                case 0x9e: // dac
                case 0xa2: // offset
                case 0xa8: // volume slide
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
        _enabled = false;
    }

    if (_sweep && !(--_sweep_div)) {
        uint16_t target = _channel->get_period();

        if (_sweep & 0x8) {
            target -= target >> (_sweep & 0x7);
        } else {
            target += target >> (_sweep & 0x7);
        }

        if (target > 0x7ff || target < 8) {
            _channel->disable();
            _enabled = false;
            _sweep = 0;
        } else {
            _channel->adjust_period(target);
        }

        _sweep_div = (_sweep & 0x70) >> 4;
    }

    if (_arpeggio) {
        if (_arp_count == 0) {
            _channel->note(_note);
            _arp_count = 1;
        } else if (_arp_count == 1 && _arpeggio & 0xf0) {
            _channel->note(_note + (_arpeggio >> 4));
            _arp_count = 2;
        } else {
            _channel->note(_note + (_arpeggio & 0xf));
            _arp_count = 0;
        }
    }

    if (_slide) {
        uint16_t period = _channel->get_period();

        if (period > _slide_target) {
            period -= _slide;
            if (period <= _slide_target) {
                _slide = 0;
            }

            _channel->adjust_period(period);
        } else {
            period += _slide;
            if (period >= _slide_target) {
                _slide = 0;
            }

            _channel->adjust_period(period);
        }
    } else if (_port) {
        uint16_t period = _channel->get_period();
        uint16_t target = _channel->to_period(_note);

        if (period > target) {
            period -= _port;
            if (period <= target) {
                period = target;
            }

            _channel->adjust_period(period);
        } else if (period > target) {
            period += _port;
            if (period >= target) {
                period = target;
            }

            _channel->adjust_period(period);
        }
    }

    if (_enabled) {
        if (_seq[VOLUME].tick < _seq[VOLUME].count) {
            _channel->volume(_volume * _seq[VOLUME].data[_seq[VOLUME].tick++] / 0xf);
        } else if (_seq[VOLUME].repeat != 0xff) {
            _seq[VOLUME].tick = _seq[VOLUME].repeat;
        }

        if (_seq[ARPEGGIO].tick < _seq[ARPEGGIO].count) {
            _channel->note(_note + _seq[ARPEGGIO].data[_seq[ARPEGGIO].tick++]);
        } else if (_seq[ARPEGGIO].repeat != 0xff) {
            _seq[ARPEGGIO].tick = _seq[ARPEGGIO].repeat;
        }

        if (_seq[PITCH].tick < _seq[PITCH].count) {
            _channel->pitch(-_seq[PITCH].data[_seq[PITCH].tick++]);
        } else if (_seq[PITCH].repeat != 0xff) {
            _seq[PITCH].tick = _seq[PITCH].repeat;
        }

        if (_seq[HIPITCH].tick < _seq[HIPITCH].count) {
            _channel->pitch(-16*_seq[HIPITCH].data[_seq[HIPITCH].tick++]);
        } else if (_seq[HIPITCH].repeat != 0xff) {
            _seq[HIPITCH].tick = _seq[HIPITCH].repeat;
        }

        if (_seq[DUTY].tick < _seq[DUTY].count) {
            _channel->duty(_seq[DUTY].data[_seq[DUTY].tick++]);
        } else if (_seq[DUTY].repeat != 0xff) {
            _seq[DUTY].tick = _seq[DUTY].repeat;
        }
    }
}


// NSF lifetime
NSF::NSF(PinName pin)
  : _apu{
        Square(), Square(), Triangle(), Noise(), 
        {&_apu.square1, &_apu.square2, &_apu.triangle, &_apu.noise},
        APU(_apu.channels, NSF_CHANNELS, pin)
    }
  , _channels{
        Channel(&_apu.square1),
        Channel(&_apu.square2),
        Channel(&_apu.triangle),
        Channel(&_apu.noise),
    } {

    for (unsigned i = 0; i < NSF_CHANNELS; i++) {
        _channels[i]._nsf = this;
    }
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

    // Reset instruments
    for (unsigned i = 0; i < NSF_CHANNELS; i++) {
        _channels[i].reset();
    }
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
    _ticker.attach_us(this, &NSF::tick, 1000000/NSF_FREQ);
}

void NSF::stop() {
    _ticker.detach();

    for (unsigned i = 0; i < NSF_CHANNELS; i++) {
        _channels[i].disable();
    }
}

// Get the current 6-bit amplitude of the APU
uint8_t NSF::output() {
    return _apu.apu.output();
}

