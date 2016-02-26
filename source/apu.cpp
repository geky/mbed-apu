
// NES APU
//

#include "apu/apu.h"

using namespace apu;


// APU lifetime
APU::APU(Channel **channels, unsigned count, PinName pin)
  : _channels(channels)
  , _count(count)
  , _dac(pin) {
    for (unsigned i = 0; i < _count; i++) {
        _channels[i]->_apu = this;
    }
}


// APU Emulation
void APU::update() {
    unsigned output = 0;

    for (unsigned i = 0; i < _count; i++) {
        output += _channels[i]->output();
    }

    if (output > 0x3f) output = 0x3f;
    _output = output;
    _dac.write_u16(output << 10);
}


// Enable/disable specified channel
void APU::enable(unsigned channel) {
    if (channel >= _count) return;
    _channels[channel]->enable();
}

void APU::disable(unsigned channel) {
    if (channel >= _count) return;
    _channels[channel]->disable();
}

// Set the note being played by a channel
// Note value starts at A0
void APU::note(unsigned channel, uint8_t note) {
    if (channel >= _count) return;
    _channels[channel]->note(note);
}

// Sets the period being played by the channel directly
// Period is based on NES clock cycles
void APU::period(unsigned channel, uint16_t period) {
    if (channel >= _count) return;
    _channels[channel]->set_period(period);
}

// Set the volume of a channel
void APU::volume(unsigned channel, uint8_t volume) {
    if (channel >= _count) return;
    _channels[channel]->volume(volume);
}

// Set the pitch offset of a channel
void APU::pitch(unsigned channel, int16_t offset) {
    if (channel >= _count) return;
    _channels[channel]->pitch(offset);
}

// Set the duty cycle of a channel
void APU::duty(unsigned channel, uint8_t duty) {
    if (channel >= _count) return;
    _channels[channel]->duty(duty);
}

