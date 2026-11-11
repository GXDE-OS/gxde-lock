/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
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

#include "controlwidget.h"
#include "sessionindicator.h"

#include <QHBoxLayout>
#include <dimagebutton.h>

DWIDGET_USE_NAMESPACE

ControlWidget::ControlWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
    initConnect();
}

void ControlWidget::setVirtualKBVisible(bool visible)
{
    m_virtualKBBtn->setVisible(visible);
}

void ControlWidget::initUI()
{
    m_mediaWidget = nullptr;

    m_mainLayout = new QHBoxLayout;

    m_virtualKBBtn = new DImageButton;
    m_virtualKBBtn->setNormalPic(":/img/screen_keyboard_normal.svg");
    m_virtualKBBtn->setHoverPic(":/img/screen_keyboard_hover.svg");
    m_virtualKBBtn->setPressPic(":/img/screen_keyboard_press.svg");
    m_virtualKBBtn->hide();

    m_switchUserBtn = new DImageButton;
    m_switchUserBtn->setNormalPic(":/img/bottom_actions/userswitch_normal.svg");
    m_switchUserBtn->setHoverPic(":/img/bottom_actions/userswitch_hover.svg");
    m_switchUserBtn->setPressPic(":/img/bottom_actions/userswitch_press.svg");

    m_powerBtn = new DImageButton;
    m_powerBtn->setNormalPic(":/img/bottom_actions/shutdown_normal.svg");
    m_powerBtn->setHoverPic(":/img/bottom_actions/shutdown_hover.svg");
    m_powerBtn->setPressPic(":/img/bottom_actions/shutdown_press.svg");

    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(26);
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_virtualKBBtn, 0, Qt::AlignBottom);
    m_mainLayout->addWidget(m_switchUserBtn, 0, Qt::AlignBottom);
    m_mainLayout->addWidget(m_powerBtn, 0, Qt::AlignBottom);
    m_mainLayout->addSpacing(60);

    setLayout(m_mainLayout);
}

void ControlWidget::initConnect()
{
    connect(m_switchUserBtn, &DImageButton::clicked, this, &ControlWidget::requestSwitchUser);
    connect(m_powerBtn, &DImageButton::clicked, this, &ControlWidget::requestShutdown);
    connect(m_virtualKBBtn, &DImageButton::clicked, this, &ControlWidget::requestSwitchVirtualKB);
}

void ControlWidget::setMPRISEnable(const bool state)
{
    if (m_mediaWidget) {
        m_mediaWidget->setVisible(state);
    } else {
        m_mediaWidget = new MediaWidget;
        m_mediaWidget->initMediaPlayer();
        m_mainLayout->insertWidget(0, m_mediaWidget);
        m_mainLayout->insertStretch(0);
    }
}

void ControlWidget::setUserSwitchEnable(const bool visible)
{
    m_switchUserBtn->setVisible(visible);
}

void ControlWidget::setSessionSwitchEnable(const bool visible)
{
    if (!m_sessionIndicator) {
        m_sessionIndicator = new SessionIndicator;
        m_mainLayout->insertWidget(1, m_sessionIndicator, 0, Qt::AlignBottom);
        connect(m_sessionIndicator, &SessionIndicator::clicked,
            this, &ControlWidget::requestSwitchSession);
    }

    m_sessionIndicator->setVisible(visible);
}

void ControlWidget::chooseToSession(const QString &session)
{
    if (m_sessionIndicator)
        m_sessionIndicator->setSession(session);
}
