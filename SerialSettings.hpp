#ifdef USE_SERIAL

#ifndef SERIALSETTINGS_HPP
#define SERIALSETTINGS_HPP

#include "ui_SerialSettings.h"

#include <QSettings>

class SerialSettings : public QDialog
{
	Q_OBJECT
public:
	SerialSettings( QSettings &settings, QWidget *parent = 0 );
	~SerialSettings();
private slots:
	void on_defaultB_clicked();
private:
	Ui::SerialSettings ui;
	QSettings &settings;
};

#endif // SERIALSETTINGS_HPP

#endif // USE_SERIAL
