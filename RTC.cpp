#include "RTC.hpp"

static inline quint8 fromBCD( quint8 BCD, quint8 bits )
{
	quint8 mask = 0x00;
	for ( int i = 0 ; i < bits ; ++i )
		mask |= 1 << i;
	BCD &= mask;
	return ( BCD >> 4 ) * 10 + ( BCD & 0x0F );
}
static inline quint8 toBCD( quint8 number )
{
	return ( number / 10 ) << 4 | ( number % 10 );
}

/**/

RTC::RTC() :
	I2CDevice( 0xD0 ),
	MEM_address( 0x00 )
{
	setDateTime( QDateTime::currentDateTime() );
	startTimer( 1000 );
}

void RTC::setDateTime( const QDateTime &dt )
{
	RAM[ 0 ] = toBCD( dt.time().second() );
	RAM[ 1 ] = toBCD( dt.time().minute() );
	RAM[ 2 ] = toBCD( dt.time().hour() );
	RAM[ 3 ] = toBCD( dt.date().dayOfWeek() );
	RAM[ 4 ] = toBCD( dt.date().day() );
	RAM[ 5 ] = toBCD( dt.date().month() );
	RAM[ 6 ] = toBCD( dt.date().year() - 2000 );
}

void RTC::timerEvent( QTimerEvent * )
{
	if ( !( RAM[ 0 ] & 0x80 ) )
	{
		const quint8 sec = fromBCD( RAM[ 0 ], 7 );
		const quint8 min = fromBCD( RAM[ 1 ], 7 );
		const quint8 hour = fromBCD( RAM[ 2 ], 6 );
		const quint8 day = fromBCD( RAM[ 4 ], 6 );
		const quint8 month = fromBCD( RAM[ 5 ], 5 );
		const quint8 year = fromBCD( RAM[ 6 ], 8 );
		setDateTime( QDateTime( QDate( year + 2000, month, day ), QTime( hour, min, sec ) ).addSecs( 1 ) );
	}
}
