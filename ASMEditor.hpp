#ifndef ASMEDITOR_HPP
#define ASMEDITOR_HPP

#include <QTextEdit>

class ASMEditor : public QTextEdit
{
	Q_OBJECT
public:
	ASMEditor( QWidget *parent = NULL );

	void setCurrentLine( int line );

	void setDebugMode( bool dm, int line = 0 );
	void debugModeSelectLine( int line );

	void clearBreakpoints();
	QList< quint32 > getBreakpoints();
public slots:
	void toggleBreakpoint();
	void commentSelected();
signals:
	void lineNumber( int line );
private slots:
	void cursorPositionChanged();
	void blockCountChanged( int blockCount );
private:
	void keyPressEvent( QKeyEvent *e );

	int lastBlockCount;
	QTextCursor cursor;
	ExtraSelection currentLine;
	QList< ExtraSelection > breakpoints;
};

#endif // ASMEDITOR_HPP
