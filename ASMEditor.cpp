#include "ASMEditor.hpp"

#include <QSyntaxHighlighter>
#include <QKeyEvent>

class ASMHighlighter : public QSyntaxHighlighter
{
public:
	ASMHighlighter( QTextDocument *document = NULL ) :
		QSyntaxHighlighter( document )
	{}
private:
	void highlightBlock( const QString &text );

	static inline bool chkFragment( const QString &txt, const QString allowedTxt = QString() )
	{
		return txt.isEmpty() || txt == " " || txt == "\t" || txt == allowedTxt || txt == ":" || txt == ";";
	}

	enum { AllowedText = QTextFormat::UserProperty, WholeLine };
	static inline QTextCharFormat makeTxtChrFmt( const QBrush &color, bool bold = false, bool wholeLine = false, const QString &allowedTxt = QString() )
	{
		QTextCharFormat txtChrFmt;
		txtChrFmt.setForeground( color );
		if ( bold )
			txtChrFmt.setFontWeight( QFont::Bold );
		txtChrFmt.setProperty( AllowedText, allowedTxt );
		txtChrFmt.setProperty( WholeLine, wholeLine );
		return txtChrFmt;
	}
};

void ASMHighlighter::highlightBlock( const QString &text )
{
	static const unsigned tabSize = 4;
	static const QPair< QStringList, QTextCharFormat > toHighlight[ tabSize ] =
	{
		qMakePair
		(
			QStringList() << "ADD" << "ADDC" << "SUBB" << "ANL" << "ORL" << "XRL" << "INC" << "DEC" << "CLR" << "CPL" << "SWAP" << "RL" << "RR" << "RLC" << "RRC" << "DA" << "MUL" << "DIV" << "MOV" << "MOVC" << "MOVX" << "XCH" << "XCHD"<< "SETB" << "AJMP" << "SJMP" << "LJMP" << "JMP" << "JC" << "JNC" << "JZ" << "JNZ" << "JB" << "JNB" << "JBC" << "CJNE" << "DJNZ" << "NOP" << "ACALL" << "LCALL" << "RET" << "RETI" << "PUSH" << "POP",
			makeTxtChrFmt( Qt::blue, true )
		),
		qMakePair
		(
		QStringList() << "A" << "B" << "C" << "DPTR" << "R0" << "R1" << "R2" << "R3" << "R4" << "R5" << "R6" << "R7",
			makeTxtChrFmt( Qt::blue, false, false, "," )
		),
		qMakePair
		(
			QStringList() << "ORG" << "EQU" << "DATA" << "BIT" << "CODE" << "DB" << "DW",
			makeTxtChrFmt( QColor( 0x8888FF ) )
		),
		qMakePair
		(
			QStringList() << "$INCLUDE",
			makeTxtChrFmt( QColor( 0x005500 ), true, true )
		)
	};

	QString txt = text.toUpper();
	int startIdx, endIdx;

	//Komentarze
	startIdx = txt.indexOf( ';' );
	if ( startIdx > -1 )
	{
		const int len = txt.length() - startIdx;
		setFormat( startIdx, len, QColor( Qt::gray ) );
		txt.remove( startIdx, len );
	}

	//LABELe
	endIdx = txt.indexOf( ':' );
	if ( endIdx > -1 )
	{
		startIdx = txt.lastIndexOf( ' ', endIdx );
		if ( startIdx < 0 )
			startIdx = txt.lastIndexOf( '\t', endIdx );
		if ( startIdx < 0 )
			startIdx = 0;
		setFormat( startIdx, endIdx - startIdx + 1, QColor( 0x885500 ) );
	}

	for ( unsigned i = 0 ; i < tabSize ; ++i )
		foreach ( QString t, toHighlight[ i ].first )
		{
			int idx = -1;
			while ( ( idx = txt.indexOf( t, idx + 1 ) ) > -1 )
			{
				const QString before = text.mid( idx - 1, 1 );
				const QString after = text.mid( idx + t.length(), 1 );
				const QString allowedTxt = toHighlight[ i ].second.property( AllowedText ).toString();
				if ( chkFragment( before, allowedTxt ) && chkFragment( after, allowedTxt ) )
				{
					if ( toHighlight[ i ].second.property( WholeLine ).toBool() )
						setFormat( 0, txt.length(), toHighlight[ i ].second );
					else
						setFormat( idx, t.length(), toHighlight[ i ].second );
				}
			}
		}
}

/**/

#include "Functions.hpp"

ASMEditor::ASMEditor( QWidget *parent ) :
	QTextEdit( parent ),
	lastBlockCount( document()->blockCount() )
{
	setAcceptRichText( false );
	QFont f = Functions::getMonospaceFont();
#ifdef Q_OS_WIN
	f.setPointSize( f.pointSize() + 1 );
#endif
	setFont( f );
	new ASMHighlighter( document() );
	connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT( cursorPositionChanged() ) );
	connect( document(), SIGNAL( blockCountChanged( int ) ), this, SLOT( blockCountChanged( int ) ) );
	cursorPositionChanged();
}

void ASMEditor::setCurrentLine( int line )
{
	if ( line > 0 )
	{
		QTextCursor txtCursor = textCursor();
		txtCursor.movePosition( QTextCursor::Start );
		txtCursor.movePosition( QTextCursor::Down, QTextCursor::MoveAnchor, line - 1 );
		txtCursor.movePosition( QTextCursor::EndOfLine );
		setTextCursor( txtCursor );
	}
}

void ASMEditor::setDebugMode( bool dm, int line )
{
	if ( dm )
	{
		cursor = textCursor();
		cursor.clearSelection();
	}
	setTextCursor( cursor );
	setTextInteractionFlags( dm ? Qt::NoTextInteraction : Qt::TextEditorInteraction );
	cursorPositionChanged();
	debugModeSelectLine( line );
}
void ASMEditor::debugModeSelectLine( int line )
{
	if ( textInteractionFlags() == Qt::NoTextInteraction )
	{
		ExtraSelection selection;
		if ( line > 0 )
		{
			selection.format.setBackground( QColor( 0xF07F0F ) );
			selection.format.setProperty( QTextFormat::FullWidthSelection, true );
			selection.cursor = cursor;
			selection.cursor.setPosition( document()->findBlockByLineNumber( line - 1 ).position() );
			setTextCursor( selection.cursor );
			emit lineNumber( line );
		}
		setExtraSelections( QList< ExtraSelection >() << breakpoints << selection );
	}
}

void ASMEditor::clearBreakpoints()
{
	breakpoints.clear();
	setExtraSelections( QList< ExtraSelection >() << currentLine );
}
QList< quint32 > ASMEditor::getBreakpoints()
{
	QList< quint32 > breakpointList;
	foreach ( ExtraSelection breakpoint, breakpoints )
		breakpointList.append( breakpoint.cursor.block().firstLineNumber() + 1 );
	return breakpointList;
}

void ASMEditor::toggleBreakpoint()
{
	for ( int i = 0 ; i < breakpoints.size() ; ++i )
	{
		if ( breakpoints[ i ].cursor.block().firstLineNumber() == textCursor().block().firstLineNumber() )
		{
			breakpoints.removeAt( i );
			setExtraSelections( QList< ExtraSelection >() << currentLine << breakpoints );
			return;
		}
	}
	ExtraSelection selection;
	selection.format.setBackground( QColor( 0xFFAAAA ) );
	selection.format.setProperty( QTextFormat::FullWidthSelection, true );
	selection.cursor = textCursor();
	breakpoints.append( selection );
	setExtraSelections( QList< ExtraSelection >() << currentLine << breakpoints );
}
void ASMEditor::commentSelected()
{
	QTextCursor txtCursor = textCursor();
	const QTextDocument *doc = document();
	const QTextBlock selectionEndBlock = doc->findBlock( txtCursor.selectionEnd() );

	int start_bolck_line = doc->findBlock( txtCursor.selectionStart() ).firstLineNumber();
	int end_bolck_line = selectionEndBlock.firstLineNumber();
	if ( end_bolck_line - start_bolck_line != 0 && txtCursor.selectionEnd() == selectionEndBlock.position() )
		--end_bolck_line;

	bool comment = false;
	txtCursor.beginEditBlock();
	for ( int j = 1 ; j <= 2 ; ++j ) //za pierwszym razem sprawdza, czy wykomentować, czy odkomentować, za drugim wykonuje operacje na tekście
		for ( int i = start_bolck_line ; i <= end_bolck_line ; ++i )
		{
			const QTextBlock block = doc->findBlockByLineNumber( i );
			const QString blockTxt = block.text();
			if ( !blockTxt.simplified().isEmpty() ) //puste linijki pomijam
			{
				int semicolonIdx = -1;
				if ( !comment )
					for ( int i = 0 ; i < blockTxt.length() ; ++i ) //szuka, czy już jest komentarz
					{
						if ( blockTxt.at( i ) == ';' )
						{
							semicolonIdx = i;
							break;
						}
						else if ( blockTxt.at( i ) != ' ' && blockTxt.at( i ) != '\t' )
							break;
					}
				if ( j == 1 && semicolonIdx < 0 )
				{
					comment = true;
					break;
				}
				else if ( j == 2 )
				{
					if ( semicolonIdx < 0 )
					{
						txtCursor.setPosition( block.position() );
						txtCursor.insertText( ";" );
					}
					else
					{
						txtCursor.setPosition( block.position() + semicolonIdx );
						txtCursor.deleteChar();
					}
				}
			}
		}
	txtCursor.endEditBlock();
}

void ASMEditor::cursorPositionChanged() //kolorowanie aktualnego wiersza
{
	if ( textInteractionFlags() != Qt::NoTextInteraction )
	{
		currentLine.format.setBackground( QColor( 0xE0EEF6 ) );
		currentLine.format.setProperty( QTextFormat::FullWidthSelection, true );
		currentLine.cursor = textCursor();
		currentLine.cursor.clearSelection();
		emit lineNumber( textCursor().block().firstLineNumber() + 1 );
	}
	setExtraSelections( QList< ExtraSelection >() << currentLine << breakpoints );
}
void ASMEditor::blockCountChanged( int blockCount ) //wcięcia
{
	QTextCursor txtCursor = textCursor();;
	if ( lastBlockCount < blockCount && !txtCursor.positionInBlock() ) //została utworzona nowa linijka
	{
		const int line = txtCursor.block().firstLineNumber();
		if ( line > 0 ) //jest poprzednia linijka
		{
			const QString prevBlock = document()->findBlockByLineNumber( line - 1 ).text();
			for ( int i = 0 ; i < prevBlock.length() ; ++i ) //utrzymywanie wcięć
			{
				if ( prevBlock.at( i ) == '\t' )
					txtCursor.insertText( "\t" );
				else if ( prevBlock.at( i ) == ' ' )
					txtCursor.insertText( " " );
				else
					break;
			}
			const int labelEndIdx = prevBlock.indexOf( ':' ); //dodatkowe wcięcie po LABELu
			if ( labelEndIdx > -1 && prevBlock.lastIndexOf( ';', labelEndIdx ) < 0 )
			{
				QChar chrBefore = prevBlock.at( labelEndIdx - 1 );
				if ( chrBefore.isLetterOrNumber() || chrBefore == '_' )
					txtCursor.insertText( "\t" );
			}
		}
	}
	lastBlockCount = blockCount;
}

void ASMEditor::keyPressEvent( QKeyEvent *e )
{
	if ( e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab )
	{
		QTextCursor txtCursor = textCursor();
		const QTextDocument *doc = document();
		const QTextBlock selectionEndBlock = doc->findBlock( txtCursor.selectionEnd() );
		int start_bolck_line = doc->findBlock( txtCursor.selectionStart() ).firstLineNumber();
		int end_bolck_line = selectionEndBlock.firstLineNumber();
		if ( start_bolck_line != end_bolck_line )
		{
			if ( txtCursor.selectionEnd() == selectionEndBlock.position() ) //nie robi wcięcia na linijce, która nie ma nic zaznaczone
				--end_bolck_line;
			txtCursor.beginEditBlock();
			for ( int i = start_bolck_line ; i <= end_bolck_line ; ++i )
			{
				const QTextBlock block = doc->findBlockByLineNumber( i );
				txtCursor.setPosition( block.position() );
				if ( e->key() == Qt::Key_Tab )
					txtCursor.insertText( "\t" );
				else
				{
					const char c = block.text().at( txtCursor.positionInBlock() ).toLatin1();
					if ( c == '\t' || c == ' ' )
						txtCursor.deleteChar();
				}
			}
			txtCursor.endEditBlock();
			return;
		}
	}
	QTextEdit::keyPressEvent( e );
}
