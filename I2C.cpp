#include "I2C.hpp"

bool I2C::setDevices( const I2CDevices &new_devs )
{
	if ( !devs.count() && new_devs.count() > 0 && new_devs.count() <= 112 )
	{
		devs = new_devs;
		return true;
	}
	return false;
}

bool I2C::step( const bool sda, const bool scl )
{
	if ( SDA && SCL && !sda ) //bit startu
	{
		clear();
		transferEnabled = true;
	}
	else if ( !SDA && SCL && sda ) //bit stopu
	{
		if ( currDev && currDev->isReadMode() )
			currDev->ticks = ticks; //tu ticks powinno wynosić 0, jeżeli jest inaczej to jakiś błąd w transmisji (np. receive ACK przed bitem stopu)
		clear();
		transferEnabled = false;
	}
	else if ( transferEnabled && !SCL && scl ) //zbocze zegara narastające
	{
		const bool readMode = currDev && currDev->isReadMode();
		if ( ticks != 8 ) //dane
		{
			if ( !readMode )
			{
				buff |= sda << ( 7 - ticks++ ); //buforowanie bitów
				if ( ticks == 8 ) //wysłano 8 bitów
				{
					if ( !currDev ) //wysłano adres urządzenia na magistralę
					{
						for ( int i = 0 ; i < devs.size() ; ++i ) //szukanie urządzenia o określonym adresie
						{
							I2CDevice *dev = devs.at( i );
							if ( dev->checkDevice( buff ) ) //jeżeli je znaleziono
							{
								currDev = dev;
								currDev->sda = false; //stan niski, oznacza ACK
								break;
							}
						}
					}
					else
					{
						currDev->setData( buff ); //zapisanie bajtu danych
						currDev->first = false;
						currDev->sda = false; //stan niski, oznacza ACK
					}
					buff = 0x00;
				}
			}
			else
				currDev->sda = ( ++ticks != 8 ) ? ( ( buff << ticks ) & 0x80 ) : true; //true - czeka na ACK
		}
		else //takt dla ACK
		{
			if ( !readMode )
			{
				if ( currDev )
					currDev->sda = true;
				ticks = 0;
			}
			else
			{
				if ( !SDA || currDev->first )
				{
					currDev->sda = ( buff = currDev->getData() ) & 0x80;
					if ( currDev->first )
					{
						currDev->first = false;
						ticks = currDev->ticks;
					}
					else
						ticks = 0;
				}
			}
		}
	}
	SCL = scl;
	SDA = sda;
	return currDev ? currDev->sda : true;
}
