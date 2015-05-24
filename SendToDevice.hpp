#ifdef USE_SERIAL

#ifndef SENDTODEVICE_HPP
#define SENDTODEVICE_HPP

#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSettings>
#include <QDialog>

class SendToDevice : private QDialog
{
	Q_OBJECT
public:
	SendToDevice( QSettings &settings, QWidget *parent = NULL );

	bool sendAndShowProgress( const QByteArray &hex, QString &errStr );
private:
	void closeEvent( QCloseEvent *e );

	QSettings &settings;

	bool br;

	QHBoxLayout layout;
	QProgressBar progressB;
	QPushButton cancelB;
};

#endif // SENDTODEVICE_HPP

#endif // USE_SERIAL
