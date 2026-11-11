/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 * Copyright (C) 2026 GXDE Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include "dpasswdeditanimatedse.h"

#include "keyboardmonitor.h"

#include <darrowrectangle.h>
#include <dimagebutton.h>
#include <dlabel.h>

#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QShowEvent>
#include <QStyle>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

namespace GXDE {

class LoadSlider : public QWidget
{
public:
    explicit LoadSlider(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }

    QColor color() const
    {
        return m_color;
    }

    void setColor(const QColor &color)
    {
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        QLinearGradient gradient(0, height() / 2, width(), height() / 2);
        gradient.setColorAt(0.0, Qt::transparent);
        gradient.setColorAt(1.0, m_color);
        painter.fillRect(0, 1, width(), height() - 2, gradient);
        QWidget::paintEvent(event);
    }

private:
    QColor m_color = Qt::gray;
};

class DPasswdEditAnimatedSEPrivate
{
public:
    DImageButton *keyboard = nullptr;
    QLineEdit *passwdEdit = nullptr;
    DLabel *caps = nullptr;
    DImageButton *eye = nullptr;
    DImageButton *submit = nullptr;
    DArrowRectangle *invalidTip = nullptr;
    DLabel *invalidMessage = nullptr;
    LoadSlider *loadSlider = nullptr;
    QPropertyAnimation *loadSliderAnimation = nullptr;
    KeyboardMonitor *keyboardMonitor = nullptr;

    bool capsEnabled = true;
    bool eyeEnabled = true;
    bool submitEnabled = true;
    bool loadAnimationEnabled = true;
    bool loading = false;
    bool alertBeforeHide = false;
};

DPasswdEditAnimatedSE::DPasswdEditAnimatedSE(QWidget *parent)
    : QFrame(parent)
    , d(std::make_unique<DPasswdEditAnimatedSEPrivate>())
{
    auto *mainLayout = new QHBoxLayout(this);
    auto *passwordLayout = new QVBoxLayout;

    d->keyboard = new DImageButton;
    d->passwdEdit = new QLineEdit;
    d->caps = new DLabel;
    d->eye = new DImageButton;
    d->submit = new DImageButton;
    d->invalidTip = new DArrowRectangle(DArrowRectangle::ArrowTop,
                                        DArrowRectangle::FloatWidget,
                                        parent ? parent : this);
    d->invalidMessage = new DLabel(d->invalidTip);
    d->invalidTip->setContent(d->invalidMessage);
    d->invalidTip->setMargin(10);

    d->loadSlider = new LoadSlider(this);
    d->loadSlider->hide();
    d->loadSliderAnimation = new QPropertyAnimation(d->loadSlider, "pos", d->loadSlider);
    d->loadSliderAnimation->setDuration(1000);
    d->loadSliderAnimation->setLoopCount(-1);
    d->loadSliderAnimation->setEasingCurve(QEasingCurve::Linear);

    d->keyboard->setObjectName("KeyboardButton");
    d->passwdEdit->setObjectName("PasswdEdit");
    d->caps->setObjectName("Capslock");
    d->eye->setObjectName("EyeButton");
    d->submit->setObjectName("SubmitButton");
    d->invalidMessage->setObjectName("InvalidMessage");
    d->invalidTip->setObjectName("InvalidTip");

    d->keyboard->setStyleSheet("background-color: transparent;");
    d->passwdEdit->setStyleSheet("background-color: transparent;");
    d->caps->setStyleSheet("background-color: transparent;");
    d->eye->setStyleSheet("background-color: transparent;");
    d->submit->setStyleSheet("background-color: transparent;");

    d->passwdEdit->setEchoMode(QLineEdit::Password);
    d->passwdEdit->setFrame(false);
    d->passwdEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->passwdEdit->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(d->passwdEdit);

    d->caps->setPixmap(QPixmap(":/img/capslock.svg"));
    d->invalidMessage->hide();
    d->invalidTip->hide();

    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(0);
    passwordLayout->addWidget(d->passwdEdit);

    mainLayout->addWidget(d->keyboard, 0, Qt::AlignLeft);
    mainLayout->addLayout(passwordLayout, 1);
    mainLayout->addWidget(d->caps, 0, Qt::AlignRight);
    mainLayout->addWidget(d->eye, 0, Qt::AlignRight);
    mainLayout->addWidget(d->submit, 0, Qt::AlignRight);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    d->keyboardMonitor = KeyboardMonitor::instance();
    const bool useX11 = QGuiApplication::platformName().contains(QLatin1String("xcb"));
    if (useX11 && !d->keyboardMonitor->isRunning())
        d->keyboardMonitor->start(QThread::LowestPriority);
    resetCapslockState();

    connect(d->keyboardMonitor, &KeyboardMonitor::capslockStatusChanged,
            this, &DPasswdEditAnimatedSE::resetCapslockState);
    connect(d->eye, &DImageButton::clicked, this, &DPasswdEditAnimatedSE::onEyeButtonClicked);
    connect(d->passwdEdit, &QLineEdit::returnPressed, this, &DPasswdEditAnimatedSE::inputDone);
    connect(d->passwdEdit, &QLineEdit::selectionChanged, this, &DPasswdEditAnimatedSE::hideAlert);
    connect(d->submit, &DImageButton::clicked, this, &DPasswdEditAnimatedSE::inputDone);
    connect(d->keyboard, &DImageButton::clicked, this, &DPasswdEditAnimatedSE::onKeyboardButtonClicked);
}

DPasswdEditAnimatedSE::~DPasswdEditAnimatedSE()
{
    delete d->invalidTip;
}

bool DPasswdEditAnimatedSE::alert() const
{
    return d->invalidTip->isVisible();
}

bool DPasswdEditAnimatedSE::editFocus() const
{
    return d->passwdEdit->hasFocus();
}

QLineEdit *DPasswdEditAnimatedSE::lineEdit() const
{
    return d->passwdEdit;
}

QPropertyAnimation *DPasswdEditAnimatedSE::loadingAnimation() const
{
    return d->loadSliderAnimation;
}

QLabel *DPasswdEditAnimatedSE::invalidMessage() const
{
    return d->invalidMessage;
}

QColor DPasswdEditAnimatedSE::loadingEffectColor() const
{
    return d->loadSlider->color();
}

void DPasswdEditAnimatedSE::setKeyboardButtonEnable(bool value)
{
    d->keyboard->setVisible(value);
}

void DPasswdEditAnimatedSE::setCapslockIndicatorEnable(bool value)
{
    if (d->capsEnabled == value)
        return;
    d->capsEnabled = value;
    resetCapslockState();
}

void DPasswdEditAnimatedSE::setEyeButtonEnable(bool value)
{
    if (d->eyeEnabled == value)
        return;
    d->eyeEnabled = value;
    d->eye->setVisible(value);
}

void DPasswdEditAnimatedSE::setSubmitButtonEnable(bool value)
{
    if (d->submitEnabled == value)
        return;
    d->submitEnabled = value;
    d->submit->setVisible(value);
}

void DPasswdEditAnimatedSE::setLoadAnimEnable(bool value)
{
    d->loadAnimationEnabled = value;
}

void DPasswdEditAnimatedSE::setEchoMode(QLineEdit::EchoMode mode)
{
    d->passwdEdit->setEchoMode(mode);
}

void DPasswdEditAnimatedSE::setSubmitIcon(const QString &normalPic,
                                        const QString &hoverPic,
                                        const QString &pressPic)
{
    d->submit->setNormalPic(normalPic);
    d->submit->setHoverPic(hoverPic);
    d->submit->setPressPic(pressPic);
}

void DPasswdEditAnimatedSE::setLoadingEffectColor(const QColor &color)
{
    d->loadSlider->setColor(color);
}

void DPasswdEditAnimatedSE::showAlert(const QString &message)
{
    hideLoadSlider();
    d->invalidMessage->setText(message);
    d->invalidMessage->adjustSize();
    d->passwdEdit->selectAll();
    d->passwdEdit->setFocus();

    if (!isVisible()) {
        d->alertBeforeHide = true;
        return;
    }

    if (!d->invalidTip->isVisible()) {
        d->invalidTip->setContent(d->invalidMessage);
        updateAlertPosition();
        Q_EMIT alertChanged(true);
        refreshStyle();
    }
}

void DPasswdEditAnimatedSE::hideAlert()
{
    if (!d->invalidTip->isVisible())
        return;

    d->invalidTip->hide();
    Q_EMIT alertChanged(false);
    refreshStyle();
}

void DPasswdEditAnimatedSE::abortAuth()
{
    if (!d->loading)
        return;
    hideLoadSlider();
    Q_EMIT abort();
}

void DPasswdEditAnimatedSE::updateAlertPosition()
{
    const QPoint pos = mapToParent(rect().bottomLeft());
    const int messageHalfWidth = d->invalidMessage->width() / 2;
    d->invalidTip->move(pos.x() + messageHalfWidth + 10, pos.y() + 5);

    const QPoint messagePos = d->invalidMessage->pos();
    const int heightOffset = d->invalidTip->height() - d->invalidMessage->height() - 10;
    d->invalidMessage->move(messagePos.x(), heightOffset / 2);

    d->invalidTip->setArrowX(20);
    d->invalidMessage->show();
    d->invalidTip->QWidget::show();
}

bool DPasswdEditAnimatedSE::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d->passwdEdit) {
        if (event->type() == QEvent::FocusIn) {
            Q_EMIT editFocusChanged(true);
            refreshStyle();
        } else if (event->type() == QEvent::FocusOut) {
            Q_EMIT editFocusChanged(false);
            refreshStyle();
        }
    }
    return QFrame::eventFilter(watched, event);
}

void DPasswdEditAnimatedSE::showEvent(QShowEvent *event)
{
    if (d->alertBeforeHide)
        showAlert(d->invalidMessage->text());
    QFrame::showEvent(event);
}

void DPasswdEditAnimatedSE::hideEvent(QHideEvent *event)
{
    d->alertBeforeHide = alert();
    hideAlert();
    QFrame::hideEvent(event);
}

void DPasswdEditAnimatedSE::resetCapslockState()
{
    d->caps->setVisible(d->capsEnabled && d->keyboardMonitor->isCapslockOn());
}

void DPasswdEditAnimatedSE::onEyeButtonClicked()
{
    const auto mode = d->passwdEdit->echoMode() == QLineEdit::Password
        ? QLineEdit::Normal
        : QLineEdit::Password;
    setEchoMode(mode);
}

void DPasswdEditAnimatedSE::showLoadSlider()
{
    if (!d->loadAnimationEnabled || d->loading)
        return;

    constexpr int sliderWidth = 40;
    d->loading = true;
    d->loadSlider->show();
    d->loadSlider->setGeometry(0, 0, sliderWidth, height());
    d->loadSliderAnimation->setStartValue(QPoint(-sliderWidth, 0));
    d->loadSliderAnimation->setEndValue(QPoint(width(), 0));
    d->loadSliderAnimation->start();
}

void DPasswdEditAnimatedSE::hideLoadSlider()
{
    if (!d->loading)
        return;
    d->loading = false;
    d->loadSliderAnimation->stop();
    d->loadSlider->hide();
}

void DPasswdEditAnimatedSE::inputDone()
{
    hideAlert();
    const QString input = d->passwdEdit->text();
    if (input.isEmpty())
        return;
    showLoadSlider();
    Q_EMIT submit(input);
}

void DPasswdEditAnimatedSE::onKeyboardButtonClicked()
{
    hideAlert();
    Q_EMIT keyboardButtonClicked();
}

void DPasswdEditAnimatedSE::refreshStyle()
{
    style()->unpolish(this);
    style()->polish(this);
    update();
}

} // namespace GXDE
