#ifndef HEXEDITOR_HPP
#define HEXEDITOR_HPP

#include "ui_HexEditor.h"

class HexEditor : public QWidget
{
	Q_OBJECT
public:
	HexEditor( quint8 *data, const quint32 data_size, const QString &title, QWidget *parent );
public slots:
	void updateHex();
signals:
	void closed();
private:
	void showEvent( QShowEvent * );
	void closeEvent( QCloseEvent * );

	Ui::HexEditor ui;

	QByteArray data;
};

#endif // HEXEDITOR_HPP
