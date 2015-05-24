#include "LED.hpp"

#include <QPainter>

void LED::paintEvent( QPaintEvent * )
{
	const QColor green( 0x00FF00 );

	const int ledWidth = qMax( 0, qMin( width(), height() ) - 2 );
	const qreal penWidth = ledWidth / 8.0;
	const QPointF center( ledWidth / 2.0, ledWidth / 2.0 );
	const QColor fillColor = green.dark( 300 - brightness / 1.275f );

	QRadialGradient fillGradient( center, ledWidth / 2.0, QPointF( center.x(), ledWidth / 3.0 ) );
	fillGradient.setColorAt( 0.0, fillColor.light( 250 ) );
	fillGradient.setColorAt( 0.5, fillColor.light( 130 ) );
	fillGradient.setColorAt( 1.0, fillColor );

	QColor borderColor = palette().color( QPalette::Dark );

	QColor glowOverlay = fillColor;
	glowOverlay.setAlpha( brightness / 2 );
	QImage img( 1, 1, QImage::Format_ARGB32_Premultiplied );
	QPainter pImg( &img );
	pImg.fillRect( 0, 0, 1, 1, borderColor );
	pImg.fillRect( 0, 0, 1, 1, glowOverlay );
	borderColor = img.pixel( 0, 0 );

	QConicalGradient borderGradient( center, -90 );
	borderGradient.setColorAt( 0.2, borderColor );
	borderGradient.setColorAt( 0.5, palette().color( QPalette::Light ) );
	borderGradient.setColorAt( 0.8, borderColor );

	QPainter p( this );
	p.translate( ( width() - ledWidth ) / 2, ( height() - ledWidth ) / 2 );
	p.setRenderHint( QPainter::Antialiasing );
	p.setBrush( fillGradient );
	p.setPen( QPen( borderGradient, penWidth ) );
	p.drawEllipse( QRectF( penWidth / 2.0, penWidth / 2.0, ledWidth - penWidth, ledWidth - penWidth ) );
}
