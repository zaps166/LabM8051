#include "DisplayPanel.hpp"

DisplayPanel::DisplayPanel( QWidget *parent ) :
	QFrame( parent ),
	layout( this )
{
	layout.setMargin( 2 );
	for ( int i = 0 ; i < 4 ; ++i )
		layout.addWidget( &sevenSegment[ i ] );
}

void DisplayPanel::updateDisplays( quint32 data, const float brightness )
{
	for ( int i = 0 ; i < 4 ; ++i )
	{
		sevenSegment[ i ].setData( data & 0xFF, brightness );
		data >>= 8;
	}
}
