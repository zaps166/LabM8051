#ifndef I2C_HPP
#define I2C_HPP

#include "I2CDevice.hpp"

#include <QVector>

typedef QVector< I2CDevice * > I2CDevices;

class I2C
{
public:
	inline I2C() :
		SCL( true ), SDA( true ), transferEnabled( false )
	{
		clear();
	}

	bool setDevices( const I2CDevices &new_devs );

	inline bool shouldStep( const bool sda, const bool scl ) const
	{
		return sda != SDA || scl != SCL;
	}

	bool step( const bool sda, const bool scl );
private:
	inline void clear()
	{
		currDev = NULL;
		buff = ticks = 0;
	}

	I2CDevices devs;
	bool SCL, SDA, transferEnabled;
	quint8 buff, ticks;
	I2CDevice *currDev;
};

#endif // I2C_HPP
