/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sessionwidget.h"
#include "sessionbasemodel.h"

#include "src/backend/sessions/sessions.h"

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QFile>
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QString>

static const int SessionButtonWidth = 160;
static const int SessionButtonHeight = 160;

const QString session_standard_icon_name(const QString &realName)
{
    if (realName.contains(QStringLiteral("gxde-flakewm"), Qt::CaseInsensitive)) {
        return QStringLiteral("gxde");
    }

    const QStringList standard_icon_list = {
        "deepin",
        "fluxbox",
        "gxde-wlcom",
        "gxde",
        "gnome",
        "plasma",
        "ubuntu",
        "xfce"
    };

    for (const auto &name : standard_icon_list)
        if (realName.contains(name, Qt::CaseInsensitive))
            return name;

    return QStringLiteral("unknown");
}

SessionWidget::SessionWidget(QWidget *parent)
    : QFrame(parent)
    , m_currentSessionIndex(0)
    , m_frameDataBind(FrameDataBind::Instance())
{
    // Replacing DDE-Daemon: Discover installed sessions
    // 代替DDE-Daemon: 发现安装的session
    gxdm::backend::Sessions sessions;
    const auto sessionTable = sessions.GetSessionHashTable();
    for (const std::string &key : sessions.GetSessionNameList()) {
        m_sessionKeys << QString::fromStdString(key);
        const auto profile = sessionTable.find(key);
        m_sessionNames << (profile == sessionTable.end()
            ? QString::fromStdString(key)
            : QString::fromStdString(profile->second.session_name));
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    loadSessionList();

    std::function<void (QVariant)> function = std::bind(&SessionWidget::onOtherPageChanged, this, std::placeholders::_1);
    int index = m_frameDataBind->registerFunction("SessionWidget", function);

    connect(this, &SessionWidget::destroyed, this, [=] {
        m_frameDataBind->unRegisterFunction("SessionWidget", index);
    });

    QTimer::singleShot(0, this, [=] {
        m_frameDataBind->refreshData("SessionWidget");
    });
}

void SessionWidget::setModel(SessionBaseModel * const model)
{
    m_model = model;
    connect(m_model, &SessionBaseModel::onSessionKeyChanged,
        this, &SessionWidget::selectSession);
}

SessionWidget::~SessionWidget()
{
    qDeleteAll(m_sessionBtns);
}

const QString SessionWidget::currentSessionName() const
{
    return m_sessionNames.value(m_currentSessionIndex);
}

const QString SessionWidget::currentSessionKey() const
{
    return m_sessionKeys.value(m_currentSessionIndex);
}

void SessionWidget::show()
{
    const int itemPadding = 20;
    const int itemWidth = m_sessionBtns.first()->width();
    const int itemTotal = itemPadding + itemWidth;

    // checked default session button
    m_sessionBtns.at(m_currentSessionIndex)->setChecked(true);

    const int cout = m_sessionBtns.size();
    const int maxLineCap = width() / itemWidth - 1; // 1 for left-offset and right-offset.
    const int offset = (width() - itemWidth * std::min(cout, maxLineCap)) / 2;

    int row = 0;
    int index = 0;
    for (auto it = m_sessionBtns.constBegin(); it != m_sessionBtns.constEnd(); ++it) {
        RoundItemButton *button = *it;
        // If the current value is the maximum, need to change the line.
        if (index >= maxLineCap) {
            index = 0;
            ++row;
        }

        QPropertyAnimation *ani = new QPropertyAnimation(button, "pos");
        ani->setStartValue(QPoint(width(), 0));
        ani->setEndValue(QPoint(QPoint(offset + index * itemWidth, itemTotal * row)));
        button->show();
        ani->start(QAbstractAnimation::DeleteWhenStopped);

        index++;
    }

    setFixedHeight(itemWidth * (row + 1));

    QWidget::show();
}

int SessionWidget::sessionCount() const
{
    return m_sessionKeys.size();
}

const QString SessionWidget::lastSessionName() const
{
    const QString session = m_model ? m_model->sessionKey() : QString();
    return session.isEmpty() ? m_sessionKeys.value(0) : session;
}

void SessionWidget::switchToUser(const QString &userName)
{
    qDebug() << "switch to user" << userName;
    if (m_currentUser != userName)
        m_currentUser = userName;

    const QString sessionName = lastSessionName();
    m_currentSessionIndex = sessionIndex(sessionName);

    m_model->setSessionKey(currentSessionKey());
    m_frameDataBind->updateValue("SessionWidget", m_currentSessionIndex);

    qDebug() << userName << "default session is: " << sessionName << m_currentSessionIndex;
}

void SessionWidget::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        chooseSession();
        break;
    case Qt::Key_Left:
        leftKeySwitch();
        break;
    case Qt::Key_Right:
    case Qt::Key_Tab:
        rightKeySwitch();
        break;
    case Qt::Key_Escape:
        emit hideFrame();
        break;
    default:
        break;
    }
}

void SessionWidget::mouseReleaseEvent(QMouseEvent *event)
{
    emit hideFrame();

    return QFrame::mouseReleaseEvent(event);
}

void SessionWidget::resizeEvent(QResizeEvent *event)
{
    QTimer::singleShot(0, this, &SessionWidget::show);

    return QFrame::resizeEvent(event);
}

void SessionWidget::onSessionButtonClicked()
{
    RoundItemButton *btn = qobject_cast<RoundItemButton *>(sender());
    Q_ASSERT(btn);
    Q_ASSERT(m_sessionBtns.contains(btn));

    btn->setChecked(true);
    m_currentSessionIndex = m_sessionBtns.indexOf(btn);

    m_model->setSessionKey(currentSessionKey());

    emit hideFrame();

//    emit sessionChanged(currentSessionName());
}

int SessionWidget::sessionIndex(const QString &sessionName)
{
    const int count = m_sessionKeys.size();
    Q_ASSERT(count);
    for (int i(0); i != count; ++i)
        if (!sessionName.compare(m_sessionKeys.value(i), Qt::CaseInsensitive))
            return i;

    // NOTE: The current session does not exist
    qWarning() << "The session does not exist, using the default value.";
    return 0;
}

void SessionWidget::selectSession(const QString& sessionKey) {
    const int index = sessionIndex(sessionKey);
    if (index == m_currentSessionIndex) {
        return;
    }

    m_frameDataBind->updateValue("SessionWidget", index);
}

void SessionWidget::onOtherPageChanged(const QVariant &value)
{
    const int index = value.toInt();

    qDebug() << index;

    if (index == m_currentSessionIndex) return;

    for (RoundItemButton *button : m_sessionBtns) {
        button->setChecked(false);
    }

    m_sessionBtns.at(index)->setChecked(true);
    m_currentSessionIndex = index;
}

void SessionWidget::leftKeySwitch()
{
    if (m_currentSessionIndex == 0) {
        m_currentSessionIndex = m_sessionBtns.size() - 1;
    } else {
        m_currentSessionIndex -= 1;
    }

    m_sessionBtns.at(m_currentSessionIndex)->setChecked(true);
    m_frameDataBind->updateValue("SessionWidget", m_currentSessionIndex);
}

void SessionWidget::rightKeySwitch()
{
    if (m_currentSessionIndex == m_sessionBtns.size() - 1) {
        m_currentSessionIndex = 0;
    } else {
        m_currentSessionIndex += 1;
    }

    m_sessionBtns.at(m_currentSessionIndex)->setChecked(true);
    m_frameDataBind->updateValue("SessionWidget", m_currentSessionIndex);
}

void SessionWidget::chooseSession()
{
    emit m_sessionBtns.at(m_currentSessionIndex)->clicked();
    emit hideFrame();
}

void SessionWidget::loadSessionList()
{
    // add sessions button
    const int count = m_sessionKeys.size();
    for (int i(0); i != count; ++i)
    {
        const QString &session_key = m_sessionKeys.value(i);
        const QString &session_name = m_sessionNames.value(i);
        const QString &session_icon = session_standard_icon_name(session_name);
        // The "gxde" session (x11 deepin.desktop) shares the GXDE logo
        // assets with the gxde-wlcom session.
        const QString iconFile = session_icon == QStringLiteral("gxde")
            ? QStringLiteral("gxde-wlcom")
            : session_icon;
        const QString iconSuffix = (session_icon == QStringLiteral("gxde-wlcom")
                || session_icon == QStringLiteral("gxde"))
            ? QStringLiteral("png")
            : QStringLiteral("svg");
        const QString normalIcon = QStringLiteral(":/img/sessions_icon/%1_normal.%2")
            .arg(iconFile, iconSuffix);
        const QString hoverIcon = QStringLiteral(":/img/sessions_icon/%1_hover.%2")
            .arg(iconFile, iconSuffix);
        const QString checkedIcon = QStringLiteral(":/img/sessions_icon/%1_press.%2")
            .arg(iconFile, iconSuffix);

        qDebug() << "(Frontend) SessionWidget: found session:"
            << session_key << "displayed as" << session_name << session_icon;
        RoundItemButton *sbtn = new RoundItemButton(session_name, this);
        sbtn->setFixedSize(SessionButtonWidth, SessionButtonHeight);
        sbtn->setAutoExclusive(true);
        sbtn->setNormalPic(normalIcon);
        sbtn->setHoverPic(hoverIcon);
        sbtn->setPressPic(checkedIcon);
        sbtn->hide();

        connect(sbtn, &RoundItemButton::clicked, this, &SessionWidget::onSessionButtonClicked);

        m_sessionBtns.append(sbtn);
    }
}
