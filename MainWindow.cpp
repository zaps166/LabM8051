#include "MainWindow.hpp"

#include "SerialSettings.hpp"
#include "SendToDevice.hpp"
#include "Functions.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMimeData>
#include <QDebug>
#include <QFile>
#include <QUrl>

#ifdef Q_OS_WIN
	#include <windows.h>
	#include <mmsystem.h>
#endif

static void setMonospaceFont( const QObjectList &objects )
{
	foreach ( QObject *object, objects )
	{
		setMonospaceFont( object->children() );
		QWidget *widget = qobject_cast< QWidget * >( object );
		if ( widget )
			widget->setFont( Functions::getMonospaceFont( widget->font() ) );
	}
}

static inline QString getBoldText( const QString &txt, bool bold )
{
	return bold ? ( "<B><U>" + txt + "</U></B>" ) : txt;
}

template < typename QTextWidget > static inline void setText( QTextWidget *tW, const QString &txt )
{
	if ( tW->text() != txt )
		tW->setText( txt );
}

#define TODO "\n\nTODO:\n  - wyjścia i tryby analogowe w przetworniku,\n  - poprawnie rosnąca temperatura oraz jej wykres,\n  - schemat stanowsika i lista rozkazów"

/**/

using namespace Functions;
using namespace Bits;

MainWindow::MainWindow() :
	settings( QSettings::IniFormat, QSettings::UserScope, qApp->applicationName() ),
	ramE( uc.RAM, sizeof uc.RAM / 2, tr( "Edytor pamięci RAM" ), this ),
	romE( uc.ROM, sizeof uc.ROM, tr( "Edytor pamięci ROM" ), this ),
	runThread( *this ),
	keys( 0xF0 ),
	displayBrightness( 0 ),
	temperature( 21.0f ),
	userStepCycles( 0 )
{
	ui.setupUi( this );
	setMonospaceFont( ui.registersFrame->children() );
	ui.speedL->setText( tr( "Symulacja zatrzymana" ) );
	memset( ledBrightness, 0, sizeof ledBrightness );

	ui.voltageS->setValue( 125 );
	converter.setVoltage( 2.0f, 2 );
	converter.setVoltage( 0.0f, 3 );

	ui.findE->hide();
	ui.findNextB->hide();

	connect( this, SIGNAL( updateInterface_signal() ), this, SLOT( updateInterface() ) );

	connect( &ramE, SIGNAL( closed() ), ui.action_Pamiec_RAM, SLOT( trigger() ) );
	connect( &romE, SIGNAL( closed() ), ui.actionPamiec_ROM, SLOT( trigger() ) );

	connect( ui.menu_Edycja, SIGNAL( aboutToShow() ), this, SLOT( createEdycja() ) );
	connect( ui.menu_Edycja, SIGNAL( aboutToHide() ), this, SLOT( destroyEdycja() ) );

	connect( ui.actionWyslij_program_testujacy, SIGNAL( triggered() ), this, SLOT( on_actionWyslij_na_urzadzenie_triggered() ) );

	connect( ui.actionO_Qt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

	connect( ui.findE, SIGNAL( returnPressed() ), this, SLOT( on_findNextB_clicked() ) );

	connect( &runThread, SIGNAL( finished() ), this, SLOT( runFinished() ) );

	connect( ui.escapeB, SIGNAL( stateChanged( bool ) ), this, SLOT( buttons( bool ) ) );
	connect( ui.upB, SIGNAL( stateChanged( bool ) ), this, SLOT( buttons( bool ) ) );
	connect( ui.downB, SIGNAL( stateChanged( bool ) ), this, SLOT( buttons( bool ) ) );
	connect( ui.enterB, SIGNAL( stateChanged( bool ) ), this, SLOT( buttons( bool ) ) );

	on_asmE_lineNumber( 1 );

	directory = settings.value( "Directory" ).toString();
	restoreState( settings.value( "GUI/WindowState" ).toByteArray() ) ;
	restoreGeometry( settings.value( "GUI/WindowGEO" ).toByteArray() );
	ui.splitter->restoreState( settings.value( "GUI/SplitterState" ).toByteArray() );

	openASM( settings.value( "FilePath" ).toString() );
	if ( asmFile.isEmpty() )
		ui.action_Nowy->trigger();

	i2c.setDevices( I2CDevices() << &rtc << &converter << &eeprom );

	updateInterface();
	elapsedT.start();
	startTimer( 500 );
}
MainWindow::~MainWindow()
{
	settings.setValue( "FilePath", asmFile );
	settings.setValue( "GUI/WindowState", saveState() );
	settings.setValue( "GUI/WindowGEO", saveGeometry() );
	settings.setValue( "GUI/SplitterState", ui.splitter->saveState() );
	settings.setValue( "Directory", directory );
}

void MainWindow::buttons( bool pressed )
{
	if ( sender() == ui.escapeB )
		setBit( keys, 4, !pressed );
	else if ( sender() == ui.upB )
		setBit( keys, 5, !pressed );
	else if ( sender() == ui.downB )
		setBit( keys, 6, !pressed );
	else if ( sender() == ui.enterB )
		setBit( keys, 7, !pressed );
}

void MainWindow::on_asmE_lineNumber( int line )
{
	ui.lineL->setText( tr( "Linia " ) + QString::number( ( currentLine = line ) ) );
}

void MainWindow::on_findNextB_clicked()
{
	if ( ui.findE->text().isEmpty() )
		return;
	bool restoreCursor = true;
	const QTextCursor txtCursor = ui.asmE->textCursor();
	for ( ;; )
	{
		if ( ui.asmE->find( ui.findE->text() ) )
		{
			restoreCursor = false;
			break;
		}
		else if ( QMessageBox::question( this, tr( "Wyszukiwanie tekstu" ), tr( "Nie znaleziono podanego tekstu" ) + "\n" + tr( "Czy zacząć szukanie od początku dokumentu?" ), QMessageBox::Yes, QMessageBox::No ) == QMessageBox::No )
			break;
		ui.asmE->moveCursor( QTextCursor::Start );
	}
	if ( restoreCursor )
		ui.asmE->setTextCursor( txtCursor );
}
void MainWindow::on_actionSzukaj_w_tekscie_triggered( bool b )
{
	ui.findE->setVisible( b );
	ui.findNextB->setVisible( b );
	if ( b )
	{
		ui.findE->selectAll();
		ui.findE->setFocus();
	}
}

QByteArray MainWindow::assembly( bool &ok, QString &errStr, bool fillLinesInMemory )
{
	Asm8051::ReturnStruct ret;
	const QByteArray hex = Asm8051::assemblyToHEX( asmFile, ret, fillLinesInMemory );
	if ( ret.err == Asm8051::NO_ERR )
	{
		ok = true;
		errStr.clear();
		linesInMemory = ret.linesInMemory;
		breakpoints = ui.asmE->getBreakpoints();
	}
	else
	{
		ok = false;
		errStr = tr( "Błąd w linii " ) + QString::number( ret.line ) + ": " + ret.lineStr;
		switch ( ret.err )
		{
			case Asm8051::DATA_OVERWRITE_ERROR:
				errStr += "\n" + tr( "Nadpisywanie już zarezerwowanej pamięci programu" );
				break;
			case Asm8051::JMP_TOO_FAR:
				errStr += "\n" + tr( "Za daleki skok" );
				break;
			case Asm8051::NON_BIT_ADDRESSABLE:
				errStr += "\n" + tr( "Obszar pamięci nie adresowany bitowo" );
				break;
			case Asm8051::FILE_ERROR:
				errStr = tr( "Błąd otwierania pliku" ) + ": " + asmFile;
				break;
			default:
				break;
		}
		ui.asmE->setCurrentLine( ret.line );
	}
	return hex;
}

void MainWindow::on_action_Nowy_triggered()
{
	if ( saveQuestion() )
		nowy();
}
void MainWindow::on_action_Otworz_triggered()
{
	if ( saveQuestion() )
		openASM( QFileDialog::getOpenFileName( this, tr( "Wybierz kod źródłowy" ), getDirectory(), "Assembler (*.a51 *.asm)" ) );
}
void MainWindow::on_action_Zapisz_triggered()
{
	if ( asmFile.isEmpty() )
		ui.action_Zapisz_jako->trigger();
	else if ( ui.asmE->document()->isModified() )
		save();
}
void MainWindow::on_action_Zapisz_jako_triggered()
{
	QString fPath = QFileDialog::getSaveFileName( this, tr( "Zapisz kod źródłowy" ), getDirectory(), "Assembler (*.a51)" );
	if ( !fPath.isEmpty() )
	{
		if ( fPath.right( 4 ).toLower() != ".a51" )
			fPath.append( ".a51" );
		save( fPath );
	}
}
void MainWindow::on_actionZaladuj_HEX_triggered()
{
	if ( saveQuestion() )
		loadHex( QFileDialog::getOpenFileName( this, tr( "Wybierz plik HEX dla 8051" ), getDirectory(), "HEX files (*.hex)" ) );
}
void MainWindow::on_actionZapisz_HEX_triggered()
{
	ui.action_Zapisz->trigger();
	if ( !asmFile.isEmpty() )
	{
		bool ok;
		QString errStr;
		const QByteArray hex = assembly( ok, errStr, false );
		if ( ok )
		{
			QString fPath = QFileDialog::getSaveFileName( this, tr( "Zapisz plik HEX" ), getDirectory(), "HEX files (*.hex)" );
			if ( !fPath.isEmpty() )
			{
				if ( fPath.right( 4 ).toLower() != ".hex" )
					fPath.append( ".hex" );
				setDirectory( fPath );
				QFile f( fPath );
				if ( f.open( QFile::WriteOnly | QFile::Text ) )
					f.write( hex );
				else
					QMessageBox::critical( this, tr( "Błąd zapisywania pliku HEX" ), tr( "Błąd podczas tworzenia pliku" ) );
			}
		}
		else
			QMessageBox::critical( this, tr( "Błąd zapisywania pliku HEX" ), errStr );
	}
}

void MainWindow::on_action_Uruchom_symulator_triggered( bool b )
{
	if ( b )
	{
		bool ok = false;
		if ( !isOnlyHex() )
			ui.action_Zapisz->trigger();
		if ( !asmFile.isEmpty() || isOnlyHex() )
		{
			QByteArray hex;
			QString errStr;
			if ( !isOnlyHex() )
				hex = assembly( ok, errStr, true );
			else
				hex = loadHexFromFile( ok, errStr );
			if ( ok )
			{
				ok = loadSoftware( hex );
				if ( romE.isVisible() )
					romE.updateHex();
			}
			else
				QMessageBox::critical( this, isOnlyHex() ? tr( "Ładowanie pliku HEX" ) : tr( "Kompilacja" ), errStr );
		}
		if ( ok )
		{
			ui.actionSzukaj_w_tekscie->setDisabled( true );
			if ( ui.actionSzukaj_w_tekscie->isChecked() )
				ui.actionSzukaj_w_tekscie->trigger();
			ui.menu_Edycja->setDisabled( true );
			ui.action_Komentarz->setDisabled( true );
			ui.actionW_cz_wy_cz_breakpoint->setDisabled( true );
			ui.asmE->setDebugMode( true, isOnlyHex() ? 0 : linesInMemory.getLine( uc.getPC() ) );
			lastBreakpointAtPC = -1;
		}
		else
		{
			ui.action_Uruchom_symulator->setChecked( false );
			onlyHex( false );
		}
	}
	else
	{
		if ( ui.actionUruchom_program->isChecked() )
			ui.actionUruchom_program->trigger();
		ui.actionSzukaj_w_tekscie->setEnabled( true );
		ui.menu_Edycja->setEnabled( true );
		ui.action_Komentarz->setEnabled( true );
		ui.actionW_cz_wy_cz_breakpoint->setEnabled( true );
		ui.asmE->setDebugMode( false );
		if ( isOnlyHex() )
			onlyHex( false );
		else
		{
			linesInMemory.clear();
			breakpoints.clear();
		}
		uc.clear();
		displayBrightness = 0;
		memset( ledBrightness, 0, sizeof ledBrightness );
		ui.speedL->setText( tr( "Symulacja zatrzymana" ) );
		ui.asmE->setFocus();
		updateInterface();
	}
}
void MainWindow::on_actionUruchom_program_triggered()
{
	if ( runThread.isRunning() )
	{
		runThread.stop();
		return;
	}
	if ( runSimulator() )
	{
		if ( !isOnlyHex() )
		{
			ui.asmE->setDisabled( true );
			ui.asmE->debugModeSelectLine( 0 );
		}
#ifdef Q_OS_WIN
		timeBeginPeriod( 1 );
#endif
		runThread.start();
	}
}
void MainWindow::on_actionUruchom_program_testuj_cy_triggered()
{
	if ( saveQuestion() )
	{
		loadHex( ":/DEMO.HEX" );
		qApp->processEvents( QEventLoop::ExcludeUserInputEvents ); //czas na wykonanie zdarzeń, np. wciśnięcie przycisku uruchomienia symulacji
		ui.actionUruchom_program->trigger();
	}
}
void MainWindow::on_actionStep_triggered()
{
	if ( runSimulator() )
	{
		if ( runThread.isRunning() )
			runThread.stop();
		const qint8 ret = step();
		if ( ret <= 0 )
			showStepError( ( Cpu8051::ERR )ret );
		else
			userStepCycles += ret;
		if ( !isOnlyHex() )
			ui.asmE->debugModeSelectLine( linesInMemory.getLine( uc.getPC() ) );
	}
}

void MainWindow::on_actionUstawienia_portu_szeregowego_triggered()
{
#ifdef USE_SERIAL
	SerialSettings serialSettings( settings, this );
	serialSettings.exec();
#else
	QMessageBox::information( this, QString(), tr( "Program został skompilowany bez obsługi portu szeregowego" ) );
#endif
}
void MainWindow::on_actionWyslij_na_urzadzenie_triggered()
{
#ifdef USE_SERIAL
	const bool sendDemoHEX = sender() == ui.actionWyslij_program_testujacy;
	if ( !sendDemoHEX && !isOnlyHex() )
		ui.action_Zapisz->trigger();
	if ( sendDemoHEX || !asmFile.isEmpty() || isOnlyHex() )
	{
		bool ok = false;
		QString errStr;
		QByteArray hex;
		if ( sendDemoHEX )
		{
			const QString hexFileCopy = hexFile;
			hexFile = ":/DEMO.HEX";
			hex = loadHexFromFile( ok, errStr );
			hexFile = hexFileCopy;
		}
		else if ( !isOnlyHex() )
			hex = assembly( ok, errStr, false );
		else
			hex = loadHexFromFile( ok, errStr );
		if ( ok )
		{
			if ( settings.value( "Serial/Port" ).toString().isEmpty() )
				ui.actionUstawienia_portu_szeregowego->trigger();
			if ( !settings.value( "Serial/Port" ).toString().isEmpty() )
			{
				SendToDevice sendToDevice( settings, this );
				ok = sendToDevice.sendAndShowProgress( hex, errStr );
			}
			else
			{
				ok = false;
				errStr = tr( "Brak skonfigurowanego portu szeregowego" );
			}
		}
		if ( !ok )
			QMessageBox::critical( this, tr( "Błąd wysyłania na urządzenie" ), errStr );
	}
#else
	QMessageBox::information( this, QString(), tr( "Program został skompilowany bez obsługi portu szeregowego" ) );
#endif
}

void MainWindow::on_action_Pamiec_RAM_triggered()
{
	ramE.setVisible( ui.action_Pamiec_RAM->isChecked() );
}
void MainWindow::on_actionPamiec_ROM_triggered()
{
	romE.setVisible( ui.actionPamiec_ROM->isChecked() );
}

void MainWindow::on_action_O_programie_triggered()
{
	QMessageBox::information( this, qApp->applicationName() + " " + qApp->applicationVersion(), tr( "Symulator stanowiska laboratoryjnego z mikrokontrolerem 8051" ) + "\n\nBłażej Szczygieł" TODO );
}
void MainWindow::on_actionLista_rozkazow_triggered()
{
	QMessageBox::information( this, QString(), "Not implemented yet" );
}

void MainWindow::on_voltageS_valueChanged( int value )
{
	ui.voltageL->setText( QString::number( value / 100.0, 'f', 2 ) + "V" );
	converter.setVoltage( value / 100.0f, 1 );
}

void MainWindow::createEdycja()
{
	foreach ( QAction *action, ui.menu_Edycja->actions() )
		if ( action->objectName().isEmpty() ) //dodanie przed pierwszym separatorem
		{
			ui.menu_Edycja->insertActions( action, ui.asmE->createStandardContextMenu()->actions() );
			break;
		}
}
void MainWindow::destroyEdycja()
{
	QList< QAction * > edycjaActions = ui.menu_Edycja->actions();
	if ( !edycjaActions.isEmpty() )
		edycjaActions.first()->parent()->deleteLater();
}

void MainWindow::runFinished()
{
#ifdef Q_OS_WIN
	timeEndPeriod( 1 );
#endif
	if ( runThread.getErrorCode() <= 0 )
		showStepError( ( Cpu8051::ERR )runThread.getErrorCode() );
	updateInterface();
	if ( !isOnlyHex() )
		ui.asmE->debugModeSelectLine( linesInMemory.getLine( uc.getPC() ) );
	ui.actionUruchom_program->setChecked( false );
	if ( !isOnlyHex() )
		ui.asmE->setDisabled( false );
}

void MainWindow::updateInterface()
{
	setText( ui.ACCEditH, toHex( uc.ACC ) );
	setText( ui.ACCEditB, toBin( uc.ACC ) );
	setText( ui.BEdit, toHex( uc.B ) );
	setText( ui.SPEdit, toHex( uc.SP ) );
	setText( ui.T0Edit, toHex( ( quint16 )( ( uc.TH0 << 8 ) | uc.TL0 ) ) );
	setText( ui.T1Edit, toHex( ( quint16 )( ( uc.TH1 << 8 ) | uc.TL1 ) ) );
	setText( ui.DPTREdit, toHex( uc.DPTR() ) );
	setText( ui.P0Edit, toBin( uc.P0 & uc.inP0 ) );
	setText( ui.P1Edit, toBin( uc.P1 & uc.inP1 ) );
	setText( ui.P2Edit, toBin( uc.P2 & uc.inP2 ) );
	setText( ui.P3Edit, toBin( uc.P3 & uc.inP3 ) );
	setText( ui.SBUFEdit, toBin( uc.SBUF ) );
	setText( ui.R0Edit, toHex( uc.R[ 0 ] ) );
	setText( ui.R1Edit, toHex( uc.R[ 1 ] ) );
	setText( ui.R2Edit, toHex( uc.R[ 2 ] ) );
	setText( ui.R3Edit, toHex( uc.R[ 3 ] ) );
	setText( ui.R4Edit, toHex( uc.R[ 4 ] ) );
	setText( ui.R5Edit, toHex( uc.R[ 5 ] ) );
	setText( ui.R6Edit, toHex( uc.R[ 6 ] ) );
	setText( ui.R7Edit, toHex( uc.R[ 7 ] ) );
	setText( ui.PCEdit, toHex( uc.getPC() ) );

	setText( ui.PSWLabel,
		getBoldText( "C", getBit( uc.PSW, Cpu8051::CY ) ) + ' ' +
		getBoldText( "AC", getBit( uc.PSW, Cpu8051::AC ) ) + ' ' +
		getBoldText( "F0", getBit( uc.PSW, Cpu8051::F0 ) ) + ' ' +
		getBoldText( "RS1", getBit( uc.PSW, Cpu8051::RS1 ) ) + ' ' +
		getBoldText( "RS0", getBit( uc.PSW, Cpu8051::RS0 ) ) + ' ' +
		getBoldText( "OV", getBit( uc.PSW, Cpu8051::OV ) ) + ' ' +
		getBoldText( "-", getBit( uc.PSW, Cpu8051::PSWReserved1 ) ) + ' ' +
		getBoldText( "P", getBit( uc.PSW, Cpu8051::P ) )
	);
	setText( ui.TCONLabel,
		getBoldText( "TF1", getBit( uc.TCON, Cpu8051::TF1 ) ) + ' ' +
		getBoldText( "TR1", getBit( uc.TCON, Cpu8051::TR1 ) ) + ' ' +
		getBoldText( "TF0", getBit( uc.TCON, Cpu8051::TF0 ) ) + ' ' +
		getBoldText( "TR0", getBit( uc.TCON, Cpu8051::TR0 ) ) + ' ' +
		getBoldText( "IE1", getBit( uc.TCON, Cpu8051::IE1 ) ) + ' ' +
		getBoldText( "IT1", getBit( uc.TCON, Cpu8051::IT1 ) ) + ' ' +
		getBoldText( "IE0", getBit( uc.TCON, Cpu8051::IE0 ) ) + ' ' +
		getBoldText( "IT0", getBit( uc.TCON, Cpu8051::IT0 ) )
	);
	setText( ui.TMODLabel,
		getBoldText( "G1", getBit( uc.TMOD, Cpu8051::G1 ) ) + ' ' +
		getBoldText( "CT1", getBit( uc.TMOD, Cpu8051::CT1 ) ) + ' ' +
		getBoldText( "M11", getBit( uc.TMOD, Cpu8051::M11 ) ) + ' ' +
		getBoldText( "M10", getBit( uc.TMOD, Cpu8051::M10 ) ) + ' ' +
		getBoldText( "G0", getBit( uc.TMOD, Cpu8051::G0 ) ) + ' ' +
		getBoldText( "CT0", getBit( uc.TMOD, Cpu8051::CT0 ) ) + ' ' +
		getBoldText( "M01", getBit( uc.TMOD, Cpu8051::M01 ) ) + ' ' +
		getBoldText( "M00", getBit( uc.TMOD, Cpu8051::M00 ) )
	);
	setText( ui.IELabel,
		getBoldText( "EA", getBit( uc.IE, Cpu8051::EA ) ) + ' ' +
		getBoldText( "-", getBit( uc.IE, Cpu8051::IEReserved2 ) ) + ' ' +
		getBoldText( "-", getBit( uc.IE, Cpu8051::IEReserved1 ) ) + ' ' +
		getBoldText( "ES", getBit( uc.IE, Cpu8051::ES ) ) + ' ' +
		getBoldText( "ET1", getBit( uc.IE, Cpu8051::ET1 ) ) + ' ' +
		getBoldText( "EX1", getBit( uc.IE, Cpu8051::EX1 ) ) + ' ' +
		getBoldText( "ET0", getBit( uc.IE, Cpu8051::ET0 ) ) + ' ' +
		getBoldText( "EX0", getBit( uc.IE, Cpu8051::EX0 ) )
	);
	setText( ui.IPLabel,
		getBoldText( "-", getBit( uc.IP, Cpu8051::IPReserved3 ) ) + ' ' +
		getBoldText( "-", getBit( uc.IP, Cpu8051::IPReserved2 ) ) + ' ' +
		getBoldText( "-", getBit( uc.IP, Cpu8051::IPReserved1 ) ) + ' ' +
		getBoldText( "PS", getBit( uc.IP, Cpu8051::PS ) ) + ' ' +
		getBoldText( "PT1", getBit( uc.IP, Cpu8051::PT1 ) ) + ' ' +
		getBoldText( "PX1", getBit( uc.IP, Cpu8051::PX1 ) ) + ' ' +
		getBoldText( "PT0", getBit( uc.IP, Cpu8051::PT0 ) ) + ' ' +
		getBoldText( "PX0", getBit( uc.IP, Cpu8051::PX0 ) )
	);
	setText( ui.SCONLabel,
		getBoldText( "SM0", getBit( uc.SCON, Cpu8051::SM0 ) ) + ' ' +
		getBoldText( "SM1", getBit( uc.SCON, Cpu8051::SM1 ) ) + ' ' +
		getBoldText( "SM2", getBit( uc.SCON, Cpu8051::SM2 ) ) + ' ' +
		getBoldText( "REN", getBit( uc.SCON, Cpu8051::REN ) ) + ' ' +
		getBoldText( "TB8", getBit( uc.SCON, Cpu8051::TB8 ) ) + ' ' +
		getBoldText( "RB8", getBit( uc.SCON, Cpu8051::RB8 ) ) + ' ' +
		getBoldText( "TI", getBit( uc.SCON, Cpu8051::TI ) ) + ' ' +
		getBoldText( "RI", getBit( uc.SCON, Cpu8051::RI ) )
	);

	if ( ramE.isVisible() )
		ramE.updateHex();

	ui.displayPanel->updateDisplays( sr32.ParallelOutput(), displayBrightness );
	ui.ledPanel->updateLEDs( ledBrightness );
}

bool MainWindow::runSimulator()
{
	if ( !ui.action_Uruchom_symulator->isChecked() )
	{
		ui.action_Uruchom_symulator->trigger();
		if ( !ui.action_Uruchom_symulator->isChecked() )
		{
			ui.actionUruchom_program->setChecked( false );
			return false;
		}
	}
	return true;
}

void MainWindow::openASM( const QString &fPath )
{
	if ( !fPath.isEmpty() )
	{
		QFile f( fPath );
		if ( f.open( QFile::ReadOnly ) )
		{
			setDirectory( asmFile = fPath );
			ui.asmE->clearBreakpoints();
			ui.asmE->setText( f.readAll() );
			setWindowTitle( qApp->applicationName() + " - " + getASMFileName() );
		}
		else
			QMessageBox::warning( this, tr( "Otwieranie kodu źródłowego" ), tr( "Błąd otwierania pliku" ) );
	}
}
void MainWindow::loadHex( const QString &fPath )
{
	if ( !fPath.isEmpty() )
	{
		nowy();
		if ( !fPath.startsWith( ":/" ) ) //nie zapisuję ścieżki do pliku jak plik jest z zasobu
			setDirectory( fPath );
		onlyHex( true, fPath );
		ui.action_Uruchom_symulator->trigger();
	}
}
bool MainWindow::loadSoftware( const QByteArray &hex )
{
	HexFile::HexERROR err = uc.loadSoftware( hex.data(), hex.size() );
	if ( err == HexFile::NO_ERR )
		return true;
	const QString errStr = ( err == HexFile::OUT_OF_ROM_MEMORY ) ? tr( "Przekroczony zakres pamięci" ) : tr( "Błąd parsowania pliku HEX" );
	QMessageBox::warning( this, tr( "Błąd ładowania programu" ), errStr );
	return false;
}

QByteArray MainWindow::loadHexFromFile( bool &ok, QString &errStr )
{
	QByteArray hex;
	QFile f( hexFile );
	if ( f.open( QFile::ReadOnly | QFile::Text ) )
		hex = f.readAll();
	if ( !( ok = hex.size() > 0 ) )
		errStr = tr( "Błąd odczytu pliku HEX" );
	return hex;
}

void MainWindow::nowy()
{
	asmFile.clear();
	ui.asmE->clearBreakpoints();
	ui.asmE->setText( "$INCLUDE (REG51.INC)\n\n" );
	ui.asmE->moveCursor( QTextCursor::End );
	setWindowTitle( qApp->applicationName() );
}

void MainWindow::onlyHex( bool b, const QString &fPth )
{
	if ( isOnlyHex() != b )
	{
		ui.asmE->setDisabled( b );
		ui.action_Zapisz->setDisabled( b );
		ui.actionZapisz_HEX->setDisabled( b );
		ui.action_Zapisz_jako->setDisabled( b );
		if ( b )
			hexFile = fPth;
		else
			hexFile.clear();
	}
}

void MainWindow::showStepError( Cpu8051::ERR err )
{
	if ( err != Cpu8051::BREAKPOINT )
	{
		QString errStr = '\n' + toHex( uc.getInstrucion() ) + " at " + toHex( ( quint16 )( uc.getPC() - 1 ) );
		switch ( err )
		{
			case Cpu8051::ERR_A5_INSTRUCTION:
				errStr.prepend( tr( "Nieprawidłowy rozkaz o kodzie 0xA5" ) );
				break;
			case Cpu8051::ERR_NO_IMPLEMENTED:
				errStr.prepend( "Nieobsługiwany rozkaz" );
				break;
			case Cpu8051::ERR_UNSUPPORTED_MODE:
				errStr.prepend( tr( "Nieobsługiwany tryb urządzenia" ) );
				break;
			default:
				break;
		}
		QMessageBox::critical( this, tr( "Błąd wykonywania instrukcji" ), errStr );
	}
}

bool MainWindow::saveQuestion()
{
	stopSimulator();
	if ( ui.asmE->document()->isModified() )
	{
		const int choice = QMessageBox::question( this, tr( "Program zmodyfikowany" ), tr( "Czy chcesz zapisać zmiany?" ), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel );
		if ( choice == QMessageBox::Yes )
			ui.action_Zapisz->trigger();
		return choice != QMessageBox::Cancel;
	}
	return true;
}
void MainWindow::save( const QString &fPath )
{
	QFile f( fPath.isNull() ? asmFile : fPath );
	if ( f.open( QFile::WriteOnly | QFile::Text ) )
	{
		setDirectory( f.fileName() );
		if ( !fPath.isNull() )
			asmFile = fPath;
		f.write( ui.asmE->toPlainText().toUtf8() );
		ui.asmE->document()->setModified( false );
		setWindowTitle( qApp->applicationName() + " - " + getASMFileName() );
	}
	else
		QMessageBox::warning( this, tr( "Zapisywanie kodu źródłowego" ), tr( "Błąd zapisywania do pliku" ) );
}

void MainWindow::keyPressEvent( QKeyEvent *e )
{
	if ( ui.findE->isVisible() )
	{
		if ( e->key() == Qt::Key_Escape )
			ui.actionSzukaj_w_tekscie->trigger();
	}
	else if ( !e->isAutoRepeat() )
		switch ( e->key() )
		{
			case Qt::Key_Escape:
				ui.escapeB->setState( true );
				return;
			case Qt::Key_Up:
				ui.upB->setState( true );
				return;
			case Qt::Key_Down:
				ui.downB->setState( true );
				return;
			case Qt::Key_Return:
			case Qt::Key_Enter:
				ui.enterB->setState( true );
				return;
		}
	QMainWindow::keyPressEvent( e );
}
void MainWindow::keyReleaseEvent( QKeyEvent *e )
{
	if ( !ui.findE->isVisible() && !e->isAutoRepeat() )
		switch ( e->key() )
		{
			case Qt::Key_Escape:
				ui.escapeB->setState( false );
				return;
			case Qt::Key_Up:
				ui.upB->setState( false );
				return;
			case Qt::Key_Down:
				ui.downB->setState( false );
				return;
			case Qt::Key_Return:
			case Qt::Key_Enter:
				ui.enterB->setState( false );
				return;
		}
	QMainWindow::keyReleaseEvent( e );
}

qint32 MainWindow::step( const quint32 totalCycles )
{
	quint32 steps = 0, display = 0, cycles = 0, led[ 4 ] = { 0, 0, 0, 0 };
	const bool hasBreakpoints = !breakpoints.isEmpty();
	qint8 ret;
	do
	{
		if ( hasBreakpoints && totalCycles > 0 )
		{
			const quint16 PC = uc.getPC();
			if ( lastBreakpointAtPC != PC )
			{
				const qint32 line = linesInMemory.getLine( PC );
				if ( breakpoints.contains( line ) )
				{
					lastBreakpointAtPC = PC;
					ret = Cpu8051::BREAKPOINT;
					break;
				}
			}
			else
				lastBreakpointAtPC = -1;
		}
		uc.inP1 = ( uc.inP1 & 0x0F ) | keys;
		if ( ( ret = uc.step() ) <= 0 )
			break;
		else
		{
			const bool scl = getBit( uc.P1, 1 );
			const bool sda = getBit( uc.P1, 2 );
			if ( i2c.shouldStep( sda, scl ) )
				setBit( uc.inP1, 2, i2c.step( sda, scl ) );

			const bool load = getBit( uc.P3, 2 );
			const bool off = getBit( uc.P3, 3 );
			if ( !uc.serial_transmission )
				sr32.step( getBit( uc.P3, 0 ), getBit( uc.P3, 1 ), load, off );
			else
			{
				if ( uc.SCON & 0xF0 )
				{
					uc.serial_transmission = 0;
					return Cpu8051::ERR_UNSUPPORTED_MODE;
				}
				for ( qint8 i = 0 ; i < ret ; ++i )
				{
					if ( uc.serial_transmission > 8 )
					{
						--uc.serial_transmission;
						continue;
					}
					const bool bit = uc.SBUF & 0x01;
					Bits::setBit( uc.P3, Cpu8051::RxD, bit ); //Data
					sr32.step( bit, false, load, off );
					sr32.step( bit, true,  load, off );
					uc.SBUF >>= 1;
					if ( uc.serial_transmission == 8 )
						uc.SBUF |= 0x80;
					if ( !--uc.serial_transmission )
					{
						Bits::setBit( uc.SCON, Cpu8051::TI, true );
						break;
					}
				}
			}

			++steps;
			display += !off;
			for ( unsigned i = 3, j = 4 ; j <= 7 ; --i, ++j )
				led[ i ] += !getBit( uc.P3, j );
			cycles += ret;
		}
	} while ( cycles < totalCycles );
	displayBrightness = sqrtf( display / ( float )steps ) * 255;
	for ( unsigned i = 0 ; i < 4 ; ++i )
		ledBrightness[ i ] = sqrtf( led[ i ] / ( float )steps ) * 255;
	if ( ret <= 0 )
		return ret;
	emit updateInterface_signal();
	return cycles;
}

void MainWindow::timerEvent( QTimerEvent * )
{
	const float step = elapsedT.elapsed() / 1000.0f;

	if ( ui.action_Uruchom_symulator->isChecked() )
	{
		if ( runThread.isRunning() )
			ui.speedL->setText( tr( "Szybkość" ) + ": " + QString::number( runThread.getSpeed() / 10.0, 'f', 1 ) + "x" );
		else
			ui.speedL->setText( tr( "Szybkość" ) + ": " + QString::number( ( 12000000000LL * userStepCycles ) / elapsedT.nsecsElapsed() ) + "Hz" );
	}
	elapsedT.start();
	userStepCycles = 0;

	if ( getBit( uc.P3, 7 ) )
		temperature -= step;
	else
		temperature += step;
	if ( temperature < 21.0f )
		temperature = 21.0f;
	else if ( temperature > 86.0f )
		temperature = 86.0f;
	converter.setValue( temperature, 0 );
}
void MainWindow::closeEvent( QCloseEvent *e )
{
	stopSimulator();
	if ( saveQuestion() )
	{
		QMainWindow::closeEvent( e );
		qApp->quit();
	}
	else
		e->ignore();
}
void MainWindow::dragEnterEvent( QDragEnterEvent *e )
{
	if ( e && e->mimeData() && e->mimeData()->hasUrls() )
		e->accept();
	QMainWindow::dragEnterEvent( e );
}
void MainWindow::dropEvent( QDropEvent *e )
{
	if ( e && e->mimeData() )
	{
		QList< QUrl > urls = e->mimeData()->urls();
		if ( urls.size() == 1 )
		{
			const QString fPath = urls.first().toLocalFile();
			const QString extension = fPath.right( 4 ).toUpper();
			if ( extension == ".ASM" || extension == ".A51" )
			{
				if ( saveQuestion() )
					openASM( fPath );
			}
			else if ( extension == ".HEX" )
			{
				if ( saveQuestion() )
					loadHex( fPath );
			}
		}
	}
	QMainWindow::dropEvent( e );
}
