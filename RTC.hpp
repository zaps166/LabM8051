#ifndef RTC_HPP
#define RTC_HPP

#include "I2CDevice.hpp"

#include <QDateTime>
#include <QObject>

/* MK41T56 */

class RTC : public I2CDevice, public QObject
{
public:
	RTC();

	inline quint8 getData()
	{
		return RAM[ MEM_address++ ];
	}
	inline void setData( const quint8 data )
	{
		if ( first )
			MEM_address = data;
		else
			RAM[ MEM_address++ ] = data;
	}
private:
	void setDateTime( const QDateTime &dt );

	void timerEvent( QTimerEvent * );

	quint8 RAM[ 64 ];
	unsigned MEM_address : 6;
};

#endif // RTC_HPP
