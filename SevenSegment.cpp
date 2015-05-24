#include "SevenSegment.hpp"
#include "Bits.hpp"

#include <QPainter>

using Bits::getBit;

SevenSegment::SevenSegment( QWidget *parent ) :
	QWidget( parent ),
	data( 0xFF ),
	brightness( 0 )
{
	const int indexes[] = { 6, 0, 3, 1, 2, 5, 4 };
	for ( int i = 0 ; i < 3 ; ++i )
	{
		//poziome segmenty
		segments[ indexes[ i ] ].addPolygon( QPolygonF()
			<< QPointF( 0.12, i*0.4 + 0.1 )
			<< QPointF( 0.22, i*0.4 + 0.0 )
			<< QPointF( 0.60, i*0.4 + 0.0 )
			<< QPointF( 0.70, i*0.4 + 0.1 )
			<< QPointF( 0.60, i*0.4 + 0.2 )
			<< QPointF( 0.22, i*0.4 + 0.2 )
		);

		//pionowe segmenty
		if ( i <= 1 )
		{
			segments[ indexes[ i+3 ] ].addPolygon( QPolygonF() //lewa strona
				<< QPointF( 0.1, i*0.4 + 0.11 )
				<< QPointF( 0.2, i*0.4 + 0.21 )
				<< QPointF( 0.2, i*0.4 + 0.39 )
				<< QPointF( 0.1, i*0.4 + 0.49 )
				<< QPointF( 0.0, i*0.4 + 0.39 )
				<< QPointF( 0.0, i*0.4 + 0.21 )
			);
			segments[ indexes[ i+5 ] ].addPolygon( QPolygonF() //prawa strona
				<< QPointF( 0.72, i*0.4 + 0.11 )
				<< QPointF( 0.82, i*0.4 + 0.21 )
				<< QPointF( 0.82, i*0.4 + 0.39 )
				<< QPointF( 0.72, i*0.4 + 0.49 )
				<< QPointF( 0.62, i*0.4 + 0.39 )
				<< QPointF( 0.62, i*0.4 + 0.21 )
			);
		}
	}

	dotP.addEllipse( QRectF( 0.87, 0.9, 0.1, 0.1 ) );
}

void SevenSegment::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	QColor fillColor( 0xF00A0A );
	fillColor.setAlpha( brightness );
	p.setPen( QPen( QColor( 0x778899 ), 0 ) );
	const int s = qMin( width(), height() );
	p.translate( ( width() - s ) / 2, ( height() - s ) / 2 );
	p.scale( s - 1, s - 1 );

	for ( int i = g ; i <= a ; ++i )
	{
		if ( fillColor.alpha() > 0 && !getBit( data, i ) )
			p.fillPath( segments[ i - g ], fillColor );
		p.drawPolygon( segments[ i - g ].toFillPolygon() );
	}

	if ( fillColor.alpha() > 0 && !getBit( data, DP ) )
		p.fillPath( dotP, fillColor );
	p.drawPath( dotP );
}
