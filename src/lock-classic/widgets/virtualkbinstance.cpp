#include "virtualkbinstance.h"

#include <DPushButton>

#include <QAbstractSpinBox>
#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

DWIDGET_USE_NAMESPACE

namespace {
constexpr int KeyboardWidth = 680;
constexpr int KeyboardHeight = 228;
}

VirtualKBInstance::VirtualKBInstance(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("VirtualKeyboard"));
    setAttribute(Qt::WA_StyledBackground);
    setMinimumWidth(280);
    setMaximumWidth(KeyboardWidth);
    setFixedHeight(KeyboardHeight);
    resize(KeyboardWidth, KeyboardHeight);
    setStyleSheet(QStringLiteral(
        "#VirtualKeyboard {"
        "  background-color: rgba(24, 24, 24, 225);"
        "  border: 1px solid rgba(255, 255, 255, 45);"
        "  border-radius: 8px;"
        "}"
        "#VirtualKeyboard QPushButton {"
        "  min-height: 34px;"
        "  color: white;"
        "  background-color: rgba(255, 255, 255, 28);"
        "  border: 1px solid rgba(255, 255, 255, 38);"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "}"
        "#VirtualKeyboard QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 48);"
        "}"
        "#VirtualKeyboard QPushButton:pressed,"
        "#VirtualKeyboard QPushButton:checked {"
        "  background-color: rgba(0, 129, 255, 190);"
        "}"
    ));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);

    auto addRow = [mainLayout] {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);
        mainLayout->addLayout(row);
        return row;
    };

    QHBoxLayout *numberRow = addRow();
    const QStringList numbers = {
        QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
        QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
        QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
        QStringLiteral("0")
    };
    const QStringList symbols = {
        QStringLiteral("!"), QStringLiteral("@"), QStringLiteral("#"),
        QStringLiteral("$"), QStringLiteral("%"), QStringLiteral("^"),
        QStringLiteral("&"), QStringLiteral("*"), QStringLiteral("("),
        QStringLiteral(")")
    };
    for (int i = 0; i < numbers.size(); ++i) {
        const int key = i == numbers.size() - 1 ? Qt::Key_0 : Qt::Key_1 + i;
        addPrintableKey(numberRow, numbers.at(i), symbols.at(i), key);
    }
    QPushButton *backspaceButton = createButton(tr("Backspace"), numberRow, 2);

    QHBoxLayout *topRow = addRow();
    const QString topLetters = QStringLiteral("qwertyuiop");
    for (const QChar letter : topLetters)
        addPrintableKey(topRow, letter, letter.toUpper(), Qt::Key_A + letter.unicode() - 'a', true);

    QHBoxLayout *homeRow = addRow();
    m_capsButton = createButton(tr("Caps"), homeRow, 2);
    m_capsButton->setCheckable(true);
    const QString homeLetters = QStringLiteral("asdfghjkl");
    for (const QChar letter : homeLetters)
        addPrintableKey(homeRow, letter, letter.toUpper(), Qt::Key_A + letter.unicode() - 'a', true);
    QPushButton *enterButton = createButton(tr("Enter"), homeRow, 2);

    QHBoxLayout *bottomRow = addRow();
    m_shiftButton = createButton(tr("Shift"), bottomRow, 2);
    m_shiftButton->setCheckable(true);
    const QString bottomLetters = QStringLiteral("zxcvbnm");
    for (const QChar letter : bottomLetters)
        addPrintableKey(bottomRow, letter, letter.toUpper(), Qt::Key_A + letter.unicode() - 'a', true);
    addPrintableKey(bottomRow, QStringLiteral(","), QStringLiteral("<"), Qt::Key_Comma);
    addPrintableKey(bottomRow, QStringLiteral("."), QStringLiteral(">"), Qt::Key_Period);
    addPrintableKey(bottomRow, QStringLiteral("/"), QStringLiteral("?"), Qt::Key_Slash);

    QHBoxLayout *spaceRow = addRow();
    spaceRow->addStretch(2);
    QPushButton *spaceButton = createButton(tr("Space"), spaceRow, 8);
    spaceRow->addStretch(2);

    connect(backspaceButton, &QPushButton::clicked, this, [this] {
        sendKey(Qt::Key_Backspace);
    });
    connect(enterButton, &QPushButton::clicked, this, [this] {
        sendKey(Qt::Key_Return, QString(), true);
    });
    connect(spaceButton, &QPushButton::clicked, this, [this] {
        sendKey(Qt::Key_Space, QStringLiteral(" "), true);
    });
    connect(m_capsButton, &QPushButton::clicked, this, [this](bool checked) {
        m_capsLock = checked;
        updateKeyLabels();
    });
    connect(m_shiftButton, &QPushButton::clicked, this, [this](bool checked) {
        m_shift = checked;
        updateKeyLabels();
    });

    if (acceptsTextInput(QApplication::focusWidget()))
        m_inputTarget = QApplication::focusWidget();
    connect(qApp, &QApplication::focusChanged, this,
            [this](QWidget *, QWidget *now) {
                if (acceptsTextInput(now))
                    m_inputTarget = now;
            });
}

QPushButton *VirtualKBInstance::createButton(const QString &text, QHBoxLayout *layout, int stretch)
{
    auto *button = new DPushButton(text, this);
    button->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(button, stretch);
    return button;
}

void VirtualKBInstance::addPrintableKey(QHBoxLayout *layout, const QString &normalText,
                                        const QString &shiftedText, int key, bool letter)
{
    QPushButton *button = createButton(normalText, layout);
    m_printableKeys.append({button, normalText, shiftedText, key, letter});
    connect(button, &QPushButton::clicked, this, [this, button] {
        const auto it = std::find_if(m_printableKeys.cbegin(), m_printableKeys.cend(),
                                     [button](const PrintableKey &item) {
                                         return item.button == button;
                                     });
        if (it == m_printableKeys.cend())
            return;

        const bool useShiftedText = it->letter ? (m_capsLock != m_shift) : m_shift;
        sendKey(it->key, useShiftedText ? it->shiftedText : it->normalText, true);
    });
}

void VirtualKBInstance::sendKey(int key, const QString &text, bool consumeShift)
{
    QWidget *target = inputTarget();
    if (!target)
        return;

    const Qt::KeyboardModifiers modifiers = m_shift ? Qt::ShiftModifier : Qt::NoModifier;
    QKeyEvent pressEvent(QEvent::KeyPress, key, modifiers, text);
    QKeyEvent releaseEvent(QEvent::KeyRelease, key, modifiers, text);
    QApplication::sendEvent(target, &pressEvent);
    QApplication::sendEvent(target, &releaseEvent);

    if (consumeShift && m_shift) {
        m_shift = false;
        m_shiftButton->setChecked(false);
        updateKeyLabels();
    }
}

void VirtualKBInstance::updateKeyLabels()
{
    for (const PrintableKey &item : std::as_const(m_printableKeys)) {
        const bool useShiftedText = item.letter ? (m_capsLock != m_shift) : m_shift;
        item.button->setText(useShiftedText ? item.shiftedText : item.normalText);
    }
}

QWidget *VirtualKBInstance::inputTarget()
{
    QWidget *focused = QApplication::focusWidget();
    if (acceptsTextInput(focused))
        m_inputTarget = focused;

    if (!m_inputTarget || !m_inputTarget->isVisible() || !m_inputTarget->isEnabled())
        return nullptr;
    return m_inputTarget;
}

bool VirtualKBInstance::acceptsTextInput(QWidget *widget)
{
    return qobject_cast<QLineEdit *>(widget)
        || qobject_cast<QTextEdit *>(widget)
        || qobject_cast<QPlainTextEdit *>(widget)
        || qobject_cast<QAbstractSpinBox *>(widget);
}
