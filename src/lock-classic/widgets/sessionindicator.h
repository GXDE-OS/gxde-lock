#ifndef SESSIONINDICATOR_H
#define SESSIONINDICATOR_H

#include <QWidget>
#include <dimagebutton.h>

class QLabel;
class QPropertyAnimation;

DWIDGET_USE_NAMESPACE

class SessionIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit SessionIndicator(QWidget *parent = nullptr);

public slots:
    void setSession(const QString &sessionKey);

signals:
    void clicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showTips();
    void hideTips();
    void setButtonImages(const QString &iconName);

    DImageButton *m_button = nullptr;
    QWidget *m_tipWidget = nullptr;
    QLabel *m_sessionTip = nullptr;
#ifndef SHENWEI_PLATFORM
    QPropertyAnimation *m_tipsAnimation = nullptr;
#endif
};

#endif // SESSIONINDICATOR_H
