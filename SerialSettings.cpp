#ifdef USE_SERIAL

#include "SerialSettings.hpp"

#include <QSerialPortInfo>
#include <QSerialPort>

SerialSettings::SerialSettings( QSettings &settings, QWidget *parent ) :
	QDialog( parent ),
	settings( settings )
{
	ui.setupUi( this );
	foreach ( const QSerialPortInfo &info, QSerialPortInfo::availablePorts() )
	{
#ifdef Q_OS_WIN
		ui.portB->addItem( info.portName(), info.systemLocation() );
#else
		ui.portB->addItem( info.systemLocation(), info.portName() );
#endif
		if ( info.portName() == settings.value( "Serial/Port" ).toString() )
			ui.portB->setCurrentIndex( ui.portB->count() - 1 );
	}
	const int idx = ui.baudrateB->findText( settings.value( "Serial/Baudrate" ).toString() );
	ui.baudrateB->setCurrentIndex( idx > -1 ? idx : 3 );
}
SerialSettings::~SerialSettings()
{
#ifdef Q_OS_WIN
	settings.setValue( "Serial/Port", ui.portB->currentText() );
#else
	if ( ui.portB->currentIndex() > -1 )
		settings.setValue( "Serial/Port", ui.portB->itemData( ui.portB->currentIndex() ) );
	else
		settings.setValue( "Serial/Port", QString() );
#endif
	settings.setValue( "Serial/Baudrate", ui.baudrateB->currentText() );
}

void SerialSettings::on_defaultB_clicked()
{
	ui.portB->setCurrentIndex( 0 );
	ui.baudrateB->setCurrentIndex( 3 );
}

#endif // USE_SERIAL
