#ifndef BITS_HPP
#define BITS_HPP

#include <stdint.h>

namespace Bits
{
	static const uint8_t PowerOf2[ 8 ] = {
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
	};
	static const uint8_t InvertedPowerOf2[ 8 ] = {
		0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F
	};

	static inline bool getBit( const uint8_t reg, const uint8_t n )
	{
		return reg & PowerOf2[ n ];
	}
	static inline void setBit( uint8_t &reg, const uint8_t n, const bool bit )
	{
		bit ? reg |= PowerOf2[ n ] : reg &= InvertedPowerOf2[ n ];
	}
	static inline void toggleBit( uint8_t &reg, const uint8_t n )
	{
		reg ^= PowerOf2[ n ];
	}
}

#endif // BITS_HPP
