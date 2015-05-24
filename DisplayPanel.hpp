#ifndef DISPLAYPANEL_HPP
#define DISPLAYPANEL_HPP

#include <QHBoxLayout>
#include <QFrame>

#include "SevenSegment.hpp"

class DisplayPanel : public QFrame
{
public:
	DisplayPanel( QWidget *parent = NULL );

	void updateDisplays( quint32 data, const float brightness );
private:
	QHBoxLayout layout;
	SevenSegment sevenSegment[ 4 ];
};

#endif // DISPLAYPANEL_HPP
