#pragma once
#include "pch.h"
#include "NES/Mappers/FDS/BaseFdsChannel.h"

class ModChannel : public BaseFdsChannel
{
private:
	const int8_t ModReset = -0x80;
	const int8_t _modLut[8] = { 0,1,2,4,ModReset,-4,-2,-1 };

	int8_t _counter = 0;
	bool _modCounterDisabled = false;
	bool _modCounterDisabledLatch = false;
	bool _forceCarryOut = false;
	bool _bit11CarryOut = false;

	uint32_t _modAccumulator = 0;		//18-bit accumulator
	uint8_t _modM2Counter = 0;
	uint8_t _modTable[32] = {};
	uint8_t _modTablePosition = 0;
	uint8_t _output = 0;

protected:
	void Serialize(Serializer& s) override
	{
		BaseFdsChannel::Serialize(s);
		
		SVArray(_modTable, 32);
		SV(_counter); SV(_modCounterDisabled); SV(_modCounterDisabledLatch); SV(_forceCarryOut); SV(_bit11CarryOut); SV(_modTablePosition); SV(_modAccumulator); SV(_modM2Counter); SV(_output);
	}

	void IncrementAccumulator(uint32_t value)
	{
		if(value == 0) return;
		_bit11CarryOut = (((_modAccumulator + value) & 0xFFF) < (_modAccumulator & 0xFFF));
		_modAccumulator += value;
		if(_modAccumulator > 0x3FFFF) {
			_modAccumulator -= 0x40000;
		}
	}

	void UpdateModTablePosition()
	{
		_modTablePosition = (_modAccumulator >> 13) & 0x1F;
	}

public:
	virtual void WriteReg(uint16_t addr, uint8_t value) override
	{
		switch(addr) {
			case 0x4084:
			case 0x4086:
				BaseFdsChannel::WriteReg(addr, value);
				break;
			case 0x4085:
				UpdateCounter(value & 0x7F);
				break;
			case 0x4087:
				BaseFdsChannel::WriteReg(addr, value);
				_modCounterDisabled = (value & 0x80) == 0x80;
				// "4087.6 forces a carry out from bit 11."
				_forceCarryOut = (value & 0x40) == 0x40;
				if(_modCounterDisabled) {
					// "Bits 0-12 are reset by 4087.7=1. Bits 13-17 have no reset."
					_modAccumulator &= 0x3E000;
				}
				else {
					// reset latch
					_modCounterDisabledLatch = _modCounterDisabled;
				}
				break;
		}
	}

	void WriteModTable(uint8_t value)
	{
		//"This register has no effect unless the mod unit is disabled via the high bit of $4087."
		if(_modCounterDisabled) {
			// "Writing $4088 increments the address (bits 13-17) when 4087.7=1."
			_modTable[_modTablePosition] = value & 0x07;
			IncrementAccumulator(0x2000);
			UpdateModTablePosition();
		}
	}

	void UpdateCounter(int8_t value)
	{
		_counter = value;
		if(_counter >= 64) {
			_counter -= 128;
		} else if(_counter < -64) {
			_counter += 128;
		}
	}

	bool TickModulator(bool haltWaveform)
	{
		if(++_modM2Counter == 16) {
			// $4083.7 also stops the mod table accumulator
			if(!haltWaveform) {
				IncrementAccumulator(_forceCarryOut ? 0x1000 : _frequency);
				// "On a carry out from bit 11, update the mod counter (increment $4085 with modtable)."
				if(_bit11CarryOut || _forceCarryOut) {
					if(!_modCounterDisabledLatch) {
						// delayed latch
						_modCounterDisabledLatch = _modCounterDisabled;
						int8_t offset = _modLut[_modTable[_modTablePosition]];
						// for some odd reason, only mod reset comes through
						// but not anything else when delayed latch is just turned on
						UpdateCounter(offset == ModReset ? 0 : _modCounterDisabledLatch ? _counter : _counter + offset);
					}
					// update mod position *after* updating counter
					UpdateModTablePosition();
				}
			}
			_modM2Counter = 0;
			return true;
		}
		return false;
	}

	void UpdateOutput(uint16_t volumePitch)
	{
		// code from new info by loopy
		// https://forums.nesdev.org/viewtopic.php?p=232662#p232662
		// pitch   = $4082/4083 (12-bit unsigned pitch value)
		// counter = $4085 (7-bit signed mod counter)
		// gain    = $4084 (6-bit unsigned mod gain)

		int32_t temp = _counter * _gain;
		if((temp & 0x0f) && !(temp & 0x800))
			temp += 0x20;
		temp += 0x400;
		temp = (temp >> 4) & 0xff;
		_output = temp;
	}

	uint8_t GetOutput()
	{
		return _output;
	}

	int8_t GetCounter()
	{
		return _counter;
	}

	int8_t GetCounterIncrement()
	{
		int8_t offset = _modLut[_modTable[_modTablePosition]];
		if(offset == ModReset) {
			return 0xC;
		}

		offset = _modCounterDisabledLatch ? 0 : offset;

		// convert to 7-bit signed
		if(offset < 0) {
			offset += 8;
		}
		// sign extend to bit 7
		offset &= 0x7;
		offset |= (offset & 0x4) << 1;
		return offset;
	}

	uint32_t GetModAccumulator()
	{
		return _modAccumulator;
	}

	bool GetForceCarryOut()
	{
		return _forceCarryOut;
	}

	bool IsModulationCounterDisabled()
	{
		return _modCounterDisabled;
	}
};
