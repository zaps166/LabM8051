#ifndef KEYBUTTON_HPP
#define KEYBUTTON_HPP

#include <QPushButton>

class KeyButton : public QPushButton
{
	Q_OBJECT
public:
	inline KeyButton( QWidget *parent = NULL ) :
		QPushButton( parent )
	{
		connect( this, SIGNAL( pressed() ), this, SLOT( emitStateChanged() ) );
		connect( this, SIGNAL( released() ), this, SLOT( emitStateChanged() ) );
	}

	inline void setState( bool pressed )
	{
		setDown( pressed );
		emit stateChanged( pressed );
	}
private slots:
	void emitStateChanged()
	{
		emit stateChanged( isDown() );
	}
signals:
	void stateChanged( bool pressed );
};

#endif // KEYBUTTON_HPP
