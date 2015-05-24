#-------------------------------------------------
#
# Project created by QtCreator 2013-03-27T18:26:57
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): {
	QT += widgets
	qtHaveModule(serialport) {
		DEFINES += USE_SERIAL
		QT += serialport
	}
} else {
#    DEFINES += USE_SERIAL
#    INCLUDEPATH += /usr/include/QtSerialPort
#    LIBS += -lQtSerialPort
}

win32: {
	LIBS += -lwinmm
	RC_FILE = Windows/icon.rc
}
else:!android: LIBS += -lrt

QMAKE_CXXFLAGS_RELEASE += -O3 #GCC generates faster code than CLang

TARGET = LabM8051
TEMPLATE = app

SOURCES += main.cpp \
	MainWindow.cpp \
	Cpu8051.cpp \
	SevenSegment.cpp \
	DisplayPanel.cpp \
	LEDPanel.cpp \
	LED.cpp \
	RunThread.cpp \
	HexFile.cpp \
	ASMEditor.cpp \
	HexEditor.cpp \
	SendToDevice.cpp \
	SerialSettings.cpp \
	RTC.cpp \
	I2C.cpp \
	Asm8051.cpp

HEADERS += \
	MainWindow.hpp \
	Cpu8051.hpp \
	SevenSegment.hpp \
	ShiftRegister32.hpp \
	DisplayPanel.hpp \
	Bits.hpp \
	LEDPanel.hpp \
	LED.hpp \
	RunThread.hpp \
	KeyButton.hpp \
	HexFile.hpp \
	ASMEditor.hpp \
	HexEditor.hpp \
	Functions.hpp \
	SendToDevice.hpp \
	SerialSettings.hpp \
	I2C.hpp \
	RTC.hpp \
	I2CDevice.hpp \
	Asm8051.hpp \
	EEPROM.hpp \
	Converter.hpp

FORMS += MainWindow.ui \
	SerialSettings.ui \
	HexEditor.ui

RESOURCES += Resources.qrc
