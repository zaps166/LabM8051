#ifndef LED_HPP
#define LED_HPP

#include <QWidget>

class LED : public QWidget
{
public:
	inline LED( QWidget *parent = NULL ) :
		QWidget( parent ),
		brightness( 0 )
	{}

	inline void setBrightness( const quint8 brightness )
	{
		if ( this->brightness != brightness )
		{
			this->brightness = brightness;
			update();
		}
	}
private:
	void paintEvent( QPaintEvent * );

	quint8 brightness;
};

#endif // LED_HPP
