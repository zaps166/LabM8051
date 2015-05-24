#ifndef SEVENSEGMENT_HPP
#define SEVENSEGMENT_HPP

#include <QWidget>

class SevenSegment : public QWidget
{
public:
	SevenSegment( QWidget *parent = NULL );

	inline void setData( const quint8 data, const quint8 brightness )
	{
		if ( this->data != data || this->brightness != brightness )
		{
			this->data = data;
			this->brightness = brightness;
			update();
		}
	}
private:
	enum SEGMENT { DP, g, f, e, d, c, b, a };

	void paintEvent( QPaintEvent * );

	quint8 data, brightness;

	QPainterPath segments[ 7 ];
	QPainterPath dotP;
};

#endif // SEVENSEGMENT_HPP
