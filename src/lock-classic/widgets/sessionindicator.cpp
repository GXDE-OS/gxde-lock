#include "sessionindicator.h"

#include "src/backend/sessions/sessions.h"

#include <QEvent>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>

#include <dimagebutton.h>

DWIDGET_USE_NAMESPACE

namespace {

constexpr int kTipWidth = 80;
constexpr int kIndicatorHeight = 40;
constexpr int kButtonWidth = 40;
constexpr int kSpacing = 26;

QString standardIconName(const QString &sessionName)
{
    if (sessionName.contains(QStringLiteral("gxde-flakewm"), Qt::CaseInsensitive)) {
        return QStringLiteral("gxde");
    }

    const QStringList standardIcons = {
        QStringLiteral("deepin"),
        QStringLiteral("enlightenment"),
        QStringLiteral("fluxbox"),
        QStringLiteral("gxde-wlcom"),
        QStringLiteral("gxde"),
        QStringLiteral("gnome"),
        QStringLiteral("lxde"),
        QStringLiteral("plasma"),
        QStringLiteral("ubuntu"),
        QStringLiteral("xfce"),
    };

    for (const QString &name : standardIcons) {
        if (sessionName.contains(name, Qt::CaseInsensitive))
            return name;
    }

    return QStringLiteral("unknown");
}

QString displayNameForSession(const QString &sessionKey)
{
    static const auto table = [] {
        gxdm::backend::Sessions sessions;
        return sessions.GetSessionHashTable();
    }();
    const auto session = table.find(sessionKey.toStdString());
    if (session != table.end())
        return QString::fromStdString(session->second.session_name);

    return sessionKey;
}

QString indicatorIconPath(const QString &iconName, const QString &state)
{
    // The "gxde" session (x11 deepin.desktop) shares the GXDE logo
    // assets with the gxde-wlcom session.
    const QString assetName = iconName == QStringLiteral("gxde")
        ? QStringLiteral("gxde-wlcom")
        : iconName;
    const QString suffix = (iconName == QStringLiteral("gxde-wlcom")
            || iconName == QStringLiteral("gxde"))
        ? QStringLiteral("png")
        : QStringLiteral("svg");
    return QStringLiteral(":/img/sessions/%1_indicator_%2.%3")
        .arg(assetName, state, suffix);
}

} // namespace

SessionIndicator::SessionIndicator(QWidget *parent)
    : QWidget(parent)
    , m_button(new DImageButton(this))
    , m_tipWidget(new QWidget(this))
    , m_sessionTip(new QLabel(m_tipWidget))
{
    setFixedSize(kTipWidth + kSpacing + kButtonWidth, kIndicatorHeight);

    m_tipWidget->setFixedSize(kTipWidth, kIndicatorHeight);
    m_sessionTip->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_sessionTip->setFixedSize(kTipWidth, kIndicatorHeight);
    m_sessionTip->setStyleSheet(QStringLiteral("color:white;font-size:12px;"));
    m_sessionTip->move(kTipWidth, 0);

    auto *shadow = new QGraphicsDropShadowEffect(m_sessionTip);
    shadow->setColor(Qt::white);
    shadow->setBlurRadius(4);
    shadow->setOffset(0, 0);
    m_sessionTip->setGraphicsEffect(shadow);

    m_button->setFixedSize(kButtonWidth, kIndicatorHeight);
    m_button->installEventFilter(this);
    setButtonImages(QStringLiteral("unknown"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kSpacing);
    layout->addWidget(m_tipWidget);
    layout->addWidget(m_button);

#ifndef SHENWEI_PLATFORM
    m_tipsAnimation = new QPropertyAnimation(m_sessionTip, "pos", this);
#endif

    connect(m_button, &DImageButton::clicked, this, &SessionIndicator::clicked);
}

void SessionIndicator::setSession(const QString &sessionKey)
{
    if (sessionKey.isEmpty())
        return;

    const QString displayName = displayNameForSession(sessionKey);
    m_sessionTip->setText(displayName);
    setButtonImages(standardIconName(displayName));
}

bool SessionIndicator::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_button) {
        if (event->type() == QEvent::Enter)
            showTips();
        else if (event->type() == QEvent::Leave)
            hideTips();
    }

    return QWidget::eventFilter(watched, event);
}

void SessionIndicator::showTips()
{
#ifndef SHENWEI_PLATFORM
    m_tipsAnimation->stop();
    m_tipsAnimation->setStartValue(m_sessionTip->pos());
    m_tipsAnimation->setEndValue(QPoint());
    m_tipsAnimation->start();
#else
    m_sessionTip->move(0, 0);
#endif
}

void SessionIndicator::hideTips()
{
#ifndef SHENWEI_PLATFORM
    m_tipsAnimation->stop();
    m_tipsAnimation->setStartValue(m_sessionTip->pos());
    m_tipsAnimation->setEndValue(QPoint(kTipWidth, 0));
    m_tipsAnimation->start();
#else
    m_sessionTip->move(kTipWidth, 0);
#endif
}

void SessionIndicator::setButtonImages(const QString &iconName)
{
    QString resolvedIcon = iconName;
    const QString normal = indicatorIconPath(resolvedIcon, QStringLiteral("normal"));
    const QString hover = indicatorIconPath(resolvedIcon, QStringLiteral("hover"));
    const QString pressed = indicatorIconPath(resolvedIcon, QStringLiteral("press"));

    if (!QFile::exists(normal) || !QFile::exists(hover) || !QFile::exists(pressed))
        resolvedIcon = QStringLiteral("unknown");

    m_button->setNormalPic(indicatorIconPath(resolvedIcon, QStringLiteral("normal")));
    m_button->setHoverPic(indicatorIconPath(resolvedIcon, QStringLiteral("hover")));
    m_button->setPressPic(indicatorIconPath(resolvedIcon, QStringLiteral("press")));
}
