#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include "I2CDevice.hpp"
#include "Bits.hpp"

#include <math.h>

/* PCF8591 */

class Converter : public I2CDevice //TODO: analog, analog_programming_mode
{
public:
	inline Converter( const bool A0 = false, const bool A1 = false, const bool A2 = false, const float Vlsb = 2.5f / 255.9f ) :
		I2CDevice( 0x90 | A0 << 1 | A1 << 2 | A2 << 3 ),
		Vlsb( Vlsb ),
		inputPTR( 0 ),
		analog_programming_mode( 0 ),
		inputIncrement( false ),
		firstGet( true )
	{
		input[ 0 ] = input[ 1 ] = input[ 2 ] = input[ 3 ] = 0x80;
	}

	inline void setValue( const quint8 val, const quint8 i )
	{
		input[ i ] = val;
	}
	inline void setVoltage( const float voltage, const quint8 i )
	{
		input[ i ] = floor( voltage / Vlsb );
	}

	inline quint8 getData()
	{
		if ( firstGet )
		{
			firstGet = false;
			return 0x80;
		}
		const quint8 val = input[ inputPTR ];
		inputPTR += inputIncrement;
		return val;
	}
	inline void setData( const quint8 data )
	{
		if ( first )
		{
			inputPTR = data & 0x03;
			inputIncrement = Bits::getBit( data, 2 );
			analog_programming_mode = ( data & 0x30 ) >> 3;
		}
	}
private:
	const float Vlsb;
	quint8 input[ 4 ];
	unsigned inputPTR : 2, analog_programming_mode : 2;
	bool inputIncrement, firstGet;
};

#endif // CONVERTER_HPP
