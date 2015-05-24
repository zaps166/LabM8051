#include <QApplication>
#if QT_VERSION < 0x050000
	#include <QTextCodec>
#endif

#include "MainWindow.hpp"

#include <time.h>

int main( int argc, char *argv[] )
{
#if QT_VERSION < 0x050000
	QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );
	QTextCodec::setCodecForTr( QTextCodec::codecForName( "UTF-8" ) );
#endif
	QApplication app( argc, argv );
	app.setApplicationName( "LabM8051" );
	app.setApplicationVersion( "v0.7.0 (III-VI 2013r.)" );
	app.setQuitOnLastWindowClosed( false );

	qsrand( time( NULL ) );

	MainWindow w;
	w.show();

	return app.exec();
}
