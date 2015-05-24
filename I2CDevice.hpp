#ifndef I2CDEVICE_HPP
#define I2CDEVICE_HPP

#include <QtGlobal>

class I2CDevice
{
public:
	virtual ~I2CDevice() {}

	inline bool checkDevice( quint8 address )
	{
		//No broadcast support
		if ( ( address & 0xFE ) == I2C_address )
		{
			ReadMode = address & 0x01;
			return ( first = true );
		}
		return false;
	}

	inline bool isReadMode() const
	{
		return ReadMode;
	}

	virtual quint8 getData() = 0;
	virtual void setData( const quint8 data ) = 0;

	bool sda, first;
	quint8 ticks;
protected:
	inline I2CDevice( quint8 I2C_address ) :
		sda( true ),
		ticks( 0 ),
		I2C_address( I2C_address )
	{}
private:
	const quint8 I2C_address;
	bool ReadMode;
};

#endif // I2CDEVICE_HPP
