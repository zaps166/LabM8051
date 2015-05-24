#ifndef LEDPANEL_HPP
#define LEDPANEL_HPP

#include <QHBoxLayout>
#include <QFrame>

#include "LED.hpp"

class LEDPanel : public QFrame
{
public:
	LEDPanel( QWidget *parent = NULL );

	void updateLEDs( const quint8 led[ 4 ] );
private:
	QHBoxLayout layout;
	LED leds[ 4 ];
};

#endif // LEDPANEL_HPP
