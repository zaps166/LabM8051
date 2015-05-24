#ifdef USE_SERIAL

#include "SendToDevice.hpp"

#include <QCoreApplication>
#include <QSerialPortInfo>
#include <QCloseEvent>
#include <QSerialPort>

SendToDevice::SendToDevice( QSettings &settings, QWidget *parent ) :
	QDialog( parent ),
	settings( settings ),
	br( false )
{
	setWindowTitle( tr( "Wysyłanie na urządzenie" ) );

	cancelB.setText( tr( "Anuluj" ) );
	connect( &cancelB, SIGNAL( clicked() ), this, SLOT( close() ) );

	setLayout( &layout );
	layout.setMargin( 2 );
	layout.addWidget( &progressB );
	layout.addWidget( &cancelB );

	resize( 300, 0 );
	setMinimumSize( 300, 0 );
	setMaximumSize( 300, 0 );
}

bool SendToDevice::sendAndShowProgress( const QByteArray &hex, QString &errStr )
{
	QSerialPort serial( settings.value( "Serial/Port" ).toString() );
	if ( serial.open( QSerialPort::WriteOnly ) )
	{
		serial.setSettingsRestoredOnClose( false );

		serial.setDataBits( QSerialPort::Data8 );
		serial.setParity( QSerialPort::NoParity );
		serial.setStopBits( QSerialPort::OneStop );
		serial.setFlowControl( QSerialPort::NoFlowControl );
		serial.setBaudRate( settings.value( "Serial/Baudrate", 9600 ).toInt() );

		QList< QByteArray > hexLines = hex.split( '\n' );
		for ( int i = hexLines.size() - 1 ; i >= 0 ; --i )
			if ( hexLines[ i ].isEmpty() )
				hexLines.removeAt( i );
			else if ( i != hexLines.size() - 1 )
				hexLines[ i ].append( '\n' );

		progressB.setMaximum( hexLines.count() - 1 );
		open();
		qApp->processEvents();

		for ( int i = 0 ; !br && i < hexLines.count() ; ++i )
		{
			progressB.setValue( i );
			qApp->processEvents();
			if ( serial.write( hexLines[ i ] ) != hexLines[ i ].length() || !serial.flush() )
			{
				errStr = tr( "Błąd wysyłania danych" );
				break;
			}
		}
	}
	else
	{
		errStr = tr( "Błąd otwierania portu" ) + " \"" + serial.portName() + "\",\n";
		switch ( serial.error() )
		{
			case QSerialPort::PermissionError:
				errStr += tr( "sprawdź uprawnienia do portu" );
				break;
			case QSerialPort::OpenError:
				errStr += tr( "sprawdź ustawienia portu oraz czy nie jest on używany" );
				break;
			default:
				errStr += tr( "sprawdź ustawienia portu szeregowego" );
		}
	}
	return errStr.isEmpty();
}

void SendToDevice::closeEvent( QCloseEvent *e )
{
	br = true;
	e->ignore();
}

#endif // USE_SERIAL
