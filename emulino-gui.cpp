/*
 * emulino - arduino emulator GUI
 * Copyright 2009 Greg Hewgill
 *
 * This file is part of Emulino.
 *
 * Emulino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Emulino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emulino.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qapplication.h>
#include <qframe.h>
#include <qhash.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qtimer.h>

#include "cpu.h"
#include "loader.h"

class EmulinoApp: public QApplication {
    Q_OBJECT
public:
    EmulinoApp(int &argc, char *argv[]);
public slots:
    void onIdle();
    void onButtonPress();
    void onButtonRelease();
private:
    QTimer timer;
};

EmulinoApp::EmulinoApp(int &argc, char *argv[])
 : QApplication(argc, argv)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(onIdle()));
    timer.start(0);
}

void EmulinoApp::onIdle()
{
    if (cpu_run() == CPU_HALT) {
        timer.stop();
    }
}

void EmulinoApp::onButtonPress()
{
    cpu_set_pin(PIN_PORTD+2, true);
}

void EmulinoApp::onButtonRelease()
{
    cpu_set_pin(PIN_PORTD+2, false);
}

class BoardWidget: public QWidget {
    Q_OBJECT
public:
    BoardWidget(QWidget *parent = 0);
protected:
    void paintEvent(QPaintEvent *event);
private:
    QImage board;
};

BoardWidget::BoardWidget(QWidget *parent)
 : QWidget(parent)
{
    board.load("arduino.jpg");
    setMinimumSize(board.size());
}

void BoardWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawImage(board.rect(), board, board.rect());
}

class Component {
public:
    virtual ~Component() {}
    virtual void setPin(int pin, bool state) = 0;
};

class PinMapper: QObject {
    Q_OBJECT
public:
    PinMapper(Component *dest);
    void connect(QObject *source, int id);
public slots:
    void stateChanged(bool state);
private:
    Component *dest;
    QHash<QObject *, int> pins;
};

PinMapper::PinMapper(Component *dest)
{
    this->dest = dest;
}

void PinMapper::connect(QObject *source, int id)
{
    QObject::connect(source, SIGNAL(stateChanged(bool)), this, SLOT(stateChanged(bool)));
    pins[source] = id;
}

void PinMapper::stateChanged(bool state)
{
    QHash<QObject *, int>::iterator i = pins.find(sender());
    if (i != pins.end()) {
        dest->setPin(i.value(), state);
    }
}

class Led: public QWidget {
    Q_OBJECT
public:
    Led(QColor color, QWidget *parent = 0);
public slots:
    void setState(bool newState);
protected:
    void paintEvent(QPaintEvent *event);
private:
    QColor color;
    bool state;
};

Led::Led(QColor color, QWidget *parent)
 : QWidget(parent)
{
    this->color = color;
    state = false;
}

void Led::setState(bool newState)
{
    state = newState;
    update();
}

void Led::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (state) {
        painter.setPen(color);
        painter.setBrush(color);
        painter.drawEllipse(rect());
    }
}

class Lcd: public QWidget, public Component {
    Q_OBJECT
public:
    Lcd(QWidget *parent = 0);
    enum Pin {pinRS, pinRW, pinE, pinD4, pinD5, pinD6, pinD7};
public slots:
    void setPin(int pin, bool state);
protected:
    void paintEvent(QPaintEvent *event);
private:
    bool RS;
    bool RW;
    u8 data;
    bool phase;
    u8 display[80];
    int cursor;
};

#include "lcd.inc"

Lcd::Lcd(QWidget *parent)
 : QWidget(parent), RS(false), RW(false), data(0), phase(false), cursor(0)
{
    memset(display, ' ', sizeof(display));
}

void Lcd::setPin(int _pin, bool state)
{
    Pin pin = static_cast<Pin>(_pin);
    switch (pin) {
    case pinRS:
        RS = state;
        break;
    case pinRW:
        RW = state;
        break;
    case pinE:
        if (state) {
            if (phase) {
                if (RS) {
                    display[cursor] = data;
                    cursor = (cursor + 1) % sizeof(display);
                } else {
                    switch (data) {
                    case 0x01:
                        memset(display, ' ', sizeof(display));
                        cursor = 0;
                        break;
                    case 0x02:
                        cursor = 0;
                        break;
                    }
                }
                update();
            }
            phase = !phase;
        } else {
            data = (data << 4) | (data & 0xf);
        }
        break;
    case pinD4: data = (data & ~0x01) | (state ? 0x01 : 0); break;
    case pinD5: data = (data & ~0x02) | (state ? 0x02 : 0); break;
    case pinD6: data = (data & ~0x04) | (state ? 0x04 : 0); break;
    case pinD7: data = (data & ~0x08) | (state ? 0x08 : 0); break;
    }
}

void Lcd::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setPen(Qt::gray);
    painter.setBrush(Qt::gray);
    painter.drawRect(rect());
    painter.setBrush(Qt::black);
    for (size_t i = 0; i < sizeof(display); i++) {
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 8; y++) {
                if (Chars[display[i]][x] & (0x80 >> y)) {
                    painter.drawRect(20*i+3*x, 3*y, 3, 3);
                }
            }
        }
    }
}

class Pin: public QObject {
    Q_OBJECT
public:
    Pin();
    void setState(bool newState);
signals:
    void stateChanged(bool state);
private:
    bool state;
};

Pin::Pin()
 : state(false)
{
}

void Pin::setState(bool newState)
{
    if (newState != state) {
        state = newState;
        stateChanged(state);
    }
}

Pin *Pins[PIN_COUNT];
void pin_change(int pin, bool state)
{
    //fprintf(stderr, "pin %d %d\n", pin, state);
    if (Pins[pin] != NULL) {
        Pins[pin]->setState(state);
    }
}

int main(int argc, char *argv[])
{
    EmulinoApp a(argc, argv);

    for (size_t i = 0; i < LENGTHOF(Pins); i++) {
        Pins[i] = new Pin();
    }

    QFrame frame;

    BoardWidget board(&frame);

    QPushButton hello("Hello world!", &frame);
    hello.resize(100, 30);

    Led led(Qt::yellow, &frame);
    led.setGeometry(225, 86, 10, 10);
    QObject::connect(Pins[PIN_PORTB+5], SIGNAL(stateChanged(bool)), &led, SLOT(setState(bool)));

    for (int i = 2; i < 8; i++) {
        Led *b = new Led(Qt::red, &frame);
        b->setGeometry(438-i*15, 10, 10, 10);
        QObject::connect(Pins[PIN_PORTD+i], SIGNAL(stateChanged(bool)), b, SLOT(setState(bool)));
    }

    Lcd lcd(&frame);
    lcd.setGeometry(20, 350, 20*20, 25);
    PinMapper mapper(&lcd);
    mapper.connect(Pins[PIN_PORTB+4], 0);
    mapper.connect(Pins[PIN_PORTB+3], 1);
    mapper.connect(Pins[PIN_PORTB+2], 2);
    mapper.connect(Pins[PIN_PORTD+5], 3);
    mapper.connect(Pins[PIN_PORTD+4], 4);
    mapper.connect(Pins[PIN_PORTD+3], 5);
    mapper.connect(Pins[PIN_PORTD+2], 6);

    frame.show();

    QObject::connect(&hello, SIGNAL(pressed()), &a, SLOT(onButtonPress()));
    QObject::connect(&hello, SIGNAL(released()), &a, SLOT(onButtonRelease()));

    u8 prog[PROGRAM_SIZE_WORDS*2];
    //u32 progsize = load_file("/Users/greg/arduino-0015/examples/Digital/Button/applet/Button.hex", prog, sizeof(prog));
    //u32 progsize = load_file("/Users/greg/arduino-0015/examples/Digital/Loop/applet/Loop.hex", prog, sizeof(prog));
    u32 progsize = load_file("/Users/greg/arduino-0015/hardware/libraries/LiquidCrystal/examples/HelloWorld/applet/HelloWorld.hex", prog, sizeof(prog));
    if (progsize == 0) {
        perror(argv[1]);
        exit(1);
    }

    u8 eeprom[512];
    u32 eepromsize = load_file("emulino.eeprom", eeprom, sizeof(eeprom));

    cpu_init();
    cpu_load_flash(prog, progsize);
    cpu_load_eeprom(eeprom, eepromsize);
    for (int i = 0; i < PIN_COUNT; i++) {
        cpu_pin_callback(i, pin_change);
    }

    return a.exec();
}

#include "emulino-gui.moc"
