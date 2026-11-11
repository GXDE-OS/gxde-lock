#include "../src/lock-classic/widgets/virtualkbinstance.h"

#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>

class VirtualKeyboardTest : public QObject
{
    Q_OBJECT

private slots:
    void sendsKeysToFocusedInput();

private:
    static QPushButton *buttonWithText(VirtualKBInstance *keyboard, const QString &text);
};

QPushButton *VirtualKeyboardTest::buttonWithText(VirtualKBInstance *keyboard,
                                                 const QString &text)
{
    const auto buttons = keyboard->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
}

void VirtualKeyboardTest::sendsKeysToFocusedInput()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *input = new QLineEdit(&window);
    auto *keyboard = new VirtualKBInstance(&window);
    layout->addWidget(input);
    layout->addWidget(keyboard);
    window.show();

    keyboard->resize(400, keyboard->height());
    QCOMPARE(keyboard->width(), 400);

    input->setFocus();
    QCoreApplication::processEvents();

    QPushButton *aButton = buttonWithText(keyboard, QStringLiteral("a"));
    QVERIFY(aButton);
    QTest::mouseClick(aButton, Qt::LeftButton);
    QCOMPARE(input->text(), QStringLiteral("a"));

    QPushButton *shiftButton = buttonWithText(keyboard, QStringLiteral("Shift"));
    QVERIFY(shiftButton);
    QTest::mouseClick(shiftButton, Qt::LeftButton);
    aButton = buttonWithText(keyboard, QStringLiteral("A"));
    QVERIFY(aButton);
    QTest::mouseClick(aButton, Qt::LeftButton);
    QCOMPARE(input->text(), QStringLiteral("aA"));

    QPushButton *capsButton = buttonWithText(keyboard, QStringLiteral("Caps"));
    QVERIFY(capsButton);
    QTest::mouseClick(capsButton, Qt::LeftButton);
    aButton = buttonWithText(keyboard, QStringLiteral("A"));
    QVERIFY(aButton);
    QTest::mouseClick(aButton, Qt::LeftButton);
    QCOMPARE(input->text(), QStringLiteral("aAA"));

    QTest::mouseClick(shiftButton, Qt::LeftButton);
    aButton = buttonWithText(keyboard, QStringLiteral("a"));
    QVERIFY(aButton);
    QTest::mouseClick(aButton, Qt::LeftButton);
    QCOMPARE(input->text(), QStringLiteral("aAAa"));

    QPushButton *backspaceButton = buttonWithText(keyboard, QStringLiteral("Backspace"));
    QVERIFY(backspaceButton);
    QTest::mouseClick(backspaceButton, Qt::LeftButton);
    QCOMPARE(input->text(), QStringLiteral("aAA"));

    QSignalSpy returnPressed(input, &QLineEdit::returnPressed);
    QPushButton *enterButton = buttonWithText(keyboard, QStringLiteral("Enter"));
    QVERIFY(enterButton);
    QTest::mouseClick(enterButton, Qt::LeftButton);
    QCOMPARE(returnPressed.count(), 1);
}

QTEST_MAIN(VirtualKeyboardTest)

#include "VirtualKeyboardTest.moc"
