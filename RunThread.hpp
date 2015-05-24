#ifndef RUNTHREAD_HPP
#define RUNTHREAD_HPP

#include <QThread>

class MainWindow;

class RunThread : public QThread
{
	Q_OBJECT
public:
	inline RunThread( MainWindow &mainW ) :
		speed( 0 ),
		mainW( mainW )
	{}

	inline void stop()
	{
		br = true;
		wait();
	}

	inline quint32 getSpeed() const
	{
		return speed;
	}
	inline qint8 getErrorCode() const
	{
		return errorCode;
	}

	void start( Priority p = InheritPriority );
private:
	void run();

	quint32 speed;
	qint8 errorCode;
	volatile bool br;
	MainWindow &mainW;
};

#endif // RUNTHREAD_HPP
