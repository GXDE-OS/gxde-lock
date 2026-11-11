/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 * Copyright (C) 2026 GXDE Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#ifndef GXDE_DPASSWDEDITANIMATEDSE_H
#define GXDE_DPASSWDEDITANIMATEDSE_H

#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>

#include <memory>

namespace GXDE {

class DPasswdEditAnimatedSEPrivate;

// Local DTK password editor fork. It keeps the original widget API while
// avoiding DTK's X11-only private keyboard monitor on native Wayland.
class DPasswdEditAnimatedSE : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY(DPasswdEditAnimatedSE)

    Q_PROPERTY(bool alert READ alert NOTIFY alertChanged)
    Q_PROPERTY(bool editFocus READ editFocus NOTIFY editFocusChanged)
    Q_PROPERTY(QColor loadingEffectColor READ loadingEffectColor WRITE setLoadingEffectColor)

public:
    explicit DPasswdEditAnimatedSE(QWidget *parent = nullptr);
    ~DPasswdEditAnimatedSE() override;

    bool alert() const;
    bool editFocus() const;
    QLineEdit *lineEdit() const;
    QPropertyAnimation *loadingAnimation() const;
    QLabel *invalidMessage() const;
    QColor loadingEffectColor() const;

Q_SIGNALS:
    void submit(const QString &input);
    void abort();
    void alertChanged(bool alert);
    void editFocusChanged(bool focus);
    void keyboardButtonClicked();

public Q_SLOTS:
    void setKeyboardButtonEnable(bool value);
    void setCapslockIndicatorEnable(bool value);
    void setEyeButtonEnable(bool value);
    void setSubmitButtonEnable(bool value);
    void setLoadAnimEnable(bool value);
    void setEchoMode(QLineEdit::EchoMode mode);
    void setSubmitIcon(const QString &normalPic, const QString &hoverPic, const QString &pressPic);
    void setLoadingEffectColor(const QColor &color);
    void showAlert(const QString &message);
    void hideAlert();
    void abortAuth();
    void updateAlertPosition();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
    void resetCapslockState();
    void onEyeButtonClicked();
    void showLoadSlider();
    void hideLoadSlider();
    void inputDone();
    void onKeyboardButtonClicked();

private:
    void refreshStyle();

    std::unique_ptr<DPasswdEditAnimatedSEPrivate> d;
};

} // namespace GXDE

#endif // GXDE_DPASSWDEDITANIMATEDSE_H
