#ifndef SHIFTREGISTER32_HPP
#define SHIFTREGISTER32_HPP

#include <stdint.h>

class ShiftRegister32
{
public:
	inline ShiftRegister32() :
		bits( 0x00000000 ), tmp_bits( 0x00000000 ), lastBits( 0x00000000 ),
		CLOCK( true ), LOAD( true ), OFF( true )
	{}

	inline void step( const bool data, const bool clock, const bool load, const bool off )
	{
		if ( !CLOCK && clock ) //rising
			tmp_bits = ( tmp_bits << 1 ) | data;
		if ( !LOAD && load )
			bits = tmp_bits;
		if ( !off )
			lastBits = bits;
		CLOCK = clock;
		LOAD = load;
		OFF = off;
	}

	inline uint32_t ParallelOutput()
	{
		return lastBits;
	}
private:
	uint32_t bits, tmp_bits, lastBits;
	bool CLOCK, LOAD, OFF;
};

#endif // SHIFTREGISTER_HPP
