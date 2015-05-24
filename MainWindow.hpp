#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "ui_MainWindow.h"

#include <QElapsedTimer>
#include <QSettings>
#include <QFileInfo>

#include "ShiftRegister32.hpp"
#include "DisplayPanel.hpp"
#include "RunThread.hpp"
#include "HexEditor.hpp"
#include "Asm8051.hpp"
#include "Cpu8051.hpp"
#include "Bits.hpp"
#include "I2C.hpp"

#include "Converter.hpp"
#include "EEPROM.hpp"
#include "RTC.hpp"

using namespace Bits;

class MainWindow : public QMainWindow
{
	friend class RunThread;
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();
private slots:
	void buttons( bool pressed );

	void on_asmE_lineNumber( int line );
	void on_findNextB_clicked();
	void on_actionSzukaj_w_tekscie_triggered( bool b );

	void on_action_Nowy_triggered();
	void on_action_Otworz_triggered();
	void on_action_Zapisz_triggered();
	void on_action_Zapisz_jako_triggered();
	void on_actionZaladuj_HEX_triggered();
	void on_actionZapisz_HEX_triggered();

	void on_action_Uruchom_symulator_triggered( bool b );
	void on_actionUruchom_program_triggered();
	void on_actionUruchom_program_testuj_cy_triggered();
	void on_actionStep_triggered();

	void on_actionUstawienia_portu_szeregowego_triggered();
	void on_actionWyslij_na_urzadzenie_triggered();

	void on_action_Pamiec_RAM_triggered();
	void on_actionPamiec_ROM_triggered();

	void on_action_O_programie_triggered();
	void on_actionLista_rozkazow_triggered();

	void on_voltageS_valueChanged( int value );

	void createEdycja();
	void destroyEdycja();

	void runFinished();

	void updateInterface();
signals:
	void updateInterface_signal();
private:
	bool runSimulator();
	inline void stopSimulator()
	{
		if ( ui.action_Uruchom_symulator->isChecked() )
			ui.action_Uruchom_symulator->trigger();
	}

	inline QString getASMFileName() const
	{
		QString directory = getDirectory();
		if ( !directory.endsWith( '/' ) )
			directory.append( '/' );
		return QString( asmFile ).remove( directory );
	}

	void openASM( const QString &fPath );
	void loadHex( const QString &fPath );
	bool loadSoftware( const QByteArray &hex );

	QByteArray loadHexFromFile( bool &ok, QString &errStr );

	QByteArray assembly( bool &ok, QString &errStr, bool fillLinesInMemory );

	void nowy();

	inline bool isOnlyHex() const
	{
		return !hexFile.isEmpty();
	}
	void onlyHex( bool b, const QString &fPth = QString() );

	void showStepError( Cpu8051::ERR err );

	bool saveQuestion();
	void save( const QString &fPath = QString() );

	void keyPressEvent( QKeyEvent * );
	void keyReleaseEvent( QKeyEvent * );

	qint32 step( const quint32 totalCycles = 0 );

	void setDirectory( const QString &fPath )
	{
		directory = QFileInfo( fPath ).absolutePath();
	}
	QString getDirectory() const
	{
		return directory;
	}

	void timerEvent( QTimerEvent * );
	void closeEvent( QCloseEvent * );
	void dragEnterEvent( QDragEnterEvent * );
	void dropEvent( QDropEvent * );

	Ui::MainWindow ui;

	QSettings settings;

	int currentLine;
	HexEditor ramE, romE;
	QElapsedTimer elapsedT;
	QList< quint32 > breakpoints;
	QString asmFile, hexFile, directory;

	Converter converter;
	EEPROM eeprom;
	RTC rtc;
	I2C i2c;
	Cpu8051 uc;
	RunThread runThread;
	ShiftRegister32 sr32;
	Asm8051::LinesInMemory linesInMemory;

	quint8 keys, displayBrightness, ledBrightness[ 4 ]; //Dla PWM
	float temperature, voltage;
	qint32 lastBreakpointAtPC;
	quint32 userStepCycles;
};

#endif // MAINWINDOW_HPP
