#ifndef EEPROM_HPP
#define EEPROM_HPP

#include "I2CDevice.hpp"

/* NM24C02 */

class EEPROM : public I2CDevice
{
public:
	inline EEPROM( const bool A0 = false, const bool A1 = false, const bool A2 = false ) :
		I2CDevice( 0xA0 | A0 << 1 | A1 << 2 | A2 << 3 ),
		MEM_address( 0x00 )
	{}

	inline void setData( const quint8 data )
	{
		if ( first )
			MEM_address = data;
		else
			RAM[ MEM_address++ ] = data;
	}
	inline quint8 getData()
	{
		return RAM[ MEM_address++ ];
	}
private:
	quint8 RAM[ 256 ];
	quint8 MEM_address;
};

#endif // EEPROM_HPP
