#include "LEDPanel.hpp"
#include "Bits.hpp"

LEDPanel::LEDPanel( QWidget *parent ) :
	QFrame( parent ),
	layout( this )
{
	layout.setMargin( 2 );
	for ( int i = 0 ; i < 4 ; ++i )
		layout.addWidget( &leds[ i ] );
}

void LEDPanel::updateLEDs( const quint8 led[ 4 ] )
{
	for ( int i = 0 ; i < 4 ; ++i )
		leds[ i ].setBrightness( led[ i ] );
}
