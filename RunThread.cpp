#include "RunThread.hpp"
#include "MainWindow.hpp"

#ifdef Q_OS_WIN
	#include <windows.h>
#endif

static inline qint32 gettime()
{
#ifndef Q_OS_WIN
	timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#else
	LARGE_INTEGER Frequency, Counter;
	QueryPerformanceFrequency( &Frequency );
	QueryPerformanceCounter( &Counter );
	return Counter.QuadPart * 1000000 / Frequency.QuadPart;
#endif
}

/**/

void RunThread::start( Priority p )
{
	br = false;
	errorCode = 1; //brak błędu
	QThread::start( p );
}

void RunThread::run()
{
	qint32 sleep_us, diff, ret, t = gettime();
	while ( !br )
	{
		if ( ( ret = mainW.step( 33333 ) ) <= 0 ) //30Hz
		{
			errorCode = ret;
			break;
		}
		diff = gettime() - t;
		speed = ret * 10 / diff;
		if ( ( sleep_us = ret - diff ) > 0 )
		{
#ifndef Q_OS_WIN
			usleep( sleep_us );
#else
			Sleep( sleep_us / 1000 );
#endif
		}
		t = gettime();
	}
	speed = 0;
}
