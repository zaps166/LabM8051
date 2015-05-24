#include "HexEditor.hpp"
#include "Functions.hpp"

HexEditor::HexEditor( quint8 *data, const quint32 data_size, const QString &title, QWidget *parent ) :
	QWidget( parent ),
	data( QByteArray::fromRawData( ( char * )data, data_size ) )
{
	ui.setupUi( this );
	resize( 350, 300 );
	setWindowTitle( title );
	setWindowFlags( Qt::Window );
	ui.pageB->setMaximum( qMax( 1U, data_size >> 12 ) );
	ui.pagesW->setVisible( ui.pageB->maximum() > 1 );
	ui.hexE->setFont( Functions::getMonospaceFont() );
	connect( ui.pageB, SIGNAL( valueChanged( int ) ), this, SLOT( updateHex() ) );
}

void HexEditor::updateHex()
{
	const int begin = ( ui.pageB->value() - 1 ) << 12;
	const int end = qMin( begin + 4096, data.size() );
	QString memory;
	for ( int i = begin ; i < end ; ++i )
		memory += Functions::toHex( ( quint8 )data[ i ] ) + ' ';
	memory.chop( 1 );
	if ( ui.hexE->toPlainText() != memory )
		ui.hexE->setPlainText( memory );
}

void HexEditor::showEvent( QShowEvent * )
{
	updateHex();
}
void HexEditor::closeEvent( QCloseEvent * )
{
	ui.hexE->clear();
	emit closed();
}
