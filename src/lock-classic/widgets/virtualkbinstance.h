#ifndef VIRTUALKBINSTANCE_H
#define VIRTUALKBINSTANCE_H

#include <QList>
#include <QPointer>
#include <QWidget>

class QHBoxLayout;
class QPushButton;

class VirtualKBInstance final : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualKBInstance(QWidget *parent = nullptr);

private:
    struct PrintableKey {
        QPushButton *button;
        QString normalText;
        QString shiftedText;
        int key;
        bool letter;
    };

    QPushButton *createButton(const QString &text, QHBoxLayout *layout, int stretch = 1);
    void addPrintableKey(QHBoxLayout *layout, const QString &normalText,
                         const QString &shiftedText, int key, bool letter = false);
    void sendKey(int key, const QString &text = QString(), bool consumeShift = false);
    void updateKeyLabels();
    QWidget *inputTarget();
    static bool acceptsTextInput(QWidget *widget);

    QList<PrintableKey> m_printableKeys;
    QPointer<QWidget> m_inputTarget;
    QPushButton *m_capsButton = nullptr;
    QPushButton *m_shiftButton = nullptr;
    bool m_capsLock = false;
    bool m_shift = false;
};

#endif // VIRTUALKBINSTANCE_H
