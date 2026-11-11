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

#include "fullscreenbackground.h"

#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QGraphicsBlurEffect>
#include <QImageReader>
#include <QLabel>
#include <QWindow>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <cmath>

namespace {

constexpr int kBackgroundFocusDuration = 400;
constexpr qreal kBackgroundFocusZoom = 0.06;
constexpr qreal kBackgroundFocusBlurRadius = 28.0;

bool isX11Platform()
{
    return QGuiApplication::platformName().contains(QLatin1String("xcb"));
}

QRect x11OutputGeometry(QScreen *screen) {
    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return QRect();

    const Window root = DefaultRootWindow(display);
    XRRScreenResources *resources = XRRGetScreenResourcesCurrent(display, root);
    if (!resources)
        resources = XRRGetScreenResources(display, root);

    const QByteArray targetName = screen ? screen->name().toUtf8() : QByteArray();
    QRect matchedGeometry;
    QRect onlyGeometry;
    int activeOutputs = 0;

    if (resources) {
        for (int i = 0; i < resources->noutput; ++i) {
            XRROutputInfo *output = XRRGetOutputInfo(
                display, resources, resources->outputs[i]);
            if (!output)
                continue;

            if (output->connection == RR_Connected && output->crtc != None) {
                XRRCrtcInfo *crtc = XRRGetCrtcInfo(display, resources, output->crtc);
                if (crtc) {
                    const QRect geometry(crtc->x, crtc->y, crtc->width, crtc->height);
                    ++activeOutputs;
                    onlyGeometry = geometry;
                    if (QByteArray(output->name, output->nameLen) == targetName)
                        matchedGeometry = geometry;
                    XRRFreeCrtcInfo(crtc);
                }
            }
            XRRFreeOutputInfo(output);
        }
        XRRFreeScreenResources(resources);
    }
    XCloseDisplay(display);

    QRect geometry = matchedGeometry;
    if (geometry.isEmpty() && activeOutputs == 1)
        geometry = onlyGeometry;
    if (geometry.isEmpty())
        return QRect();

    // RandR reports native pixels while Qt widget geometry is logical.
    const qreal pixelRatio = screen ? screen->devicePixelRatio() : 1.0;
    if (pixelRatio > 0.0) {
        geometry.setSize(QSize(qRound(geometry.width() / pixelRatio),
                               qRound(geometry.height() / pixelRatio)));
    }
    return geometry;
}

}

FullscreenBackground::FullscreenBackground(QWidget *parent)
    : QWidget(parent)
    , m_fadeOutAni(new QVariantAnimation(this))
    , m_focusBackground(new QLabel(this))
    , m_focusBlurEffect(new QGraphicsBlurEffect(m_focusBackground))
    , m_focusAnimation(new QVariantAnimation(this))
    , m_screenGeometryRefreshTimer(new QTimer(this))
    , m_x11GeometryPollTimer(new QTimer(this))
{
    Qt::WindowFlags flags = Qt::WindowStaysOnTopHint;
    if (isX11Platform())
        flags |= Qt::X11BypassWindowManagerHint;
    else
        flags |= Qt::FramelessWindowHint;
    setWindowFlags(flags);

    m_fadeOutAni->setEasingCurve(QEasingCurve::InOutCubic);
    m_fadeOutAni->setDuration(1000);
    m_fadeOutAni->setStartValue(1.0f);
    m_fadeOutAni->setEndValue(0.0f);

    connect(m_fadeOutAni, &QVariantAnimation::valueChanged, this, static_cast<void (FullscreenBackground::*)()>(&FullscreenBackground::update));

    m_focusBackground->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_focusBackground->setScaledContents(true);
    m_focusBackground->setGraphicsEffect(m_focusBlurEffect);
    m_focusBackground->hide();

    m_focusBlurEffect->setBlurHints(
        QGraphicsBlurEffect::PerformanceHint | QGraphicsBlurEffect::AnimationHint);
    m_focusBlurEffect->setBlurRadius(0.0);

    m_focusAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_focusAnimation, &QVariantAnimation::valueChanged, this,
        [this](const QVariant &value) {
            setBackgroundFocusProgress(value.toReal());
        });
    connect(m_focusAnimation, &QVariantAnimation::finished, this, [this] {
        if (qFuzzyIsNull(m_focusProgress))
            m_focusBackground->hide();
    });

    // Refer to gxde-dock
    m_screenGeometryRefreshTimer->setSingleShot(true);
    connect(m_screenGeometryRefreshTimer, &QTimer::timeout, this, [this] {
        refreshForCurrentSize();
        repaint();
    });

    m_x11GeometryPollTimer->setInterval(250);
    connect(m_x11GeometryPollTimer, &QTimer::timeout,
            this, &FullscreenBackground::applyScreenGeometry);
}

bool FullscreenBackground::contentVisible() const
{
    return m_content && m_content->isVisible();
}

void FullscreenBackground::updateBackground(const QPixmap &background)
{
    // show old background fade out
    m_fakeBackground = m_background;
    m_background = background;

    m_backgroundCache = pixmapHandle(m_background);
    m_fakeBackgroundCache = pixmapHandle(m_fakeBackground);
    updateFocusBackground();

    m_fadeOutAni->start();
}

void FullscreenBackground::updateBackground(const QString &file)
{
    const QUrl url(file);
    if (url.isLocalFile())
        return updateBackground(url.path());

    if (QFile::exists(file)) {
        m_bgPath = file;
    }
    else {
        m_bgPath = "/usr/share/wallpapers/deepin/desktop.jpg";
        if (!QFile::exists(m_bgPath)) {
            m_bgPath = "/usr/share/backgrounds/default_background.jpg";
        }
    }

    Q_ASSERT(QFileInfo(m_bgPath).isFile());

    QImageReader reader(m_bgPath);
    updateBackground(QPixmap::fromImageReader(&reader));
}

void FullscreenBackground::setScreen(QScreen *screen)
{
    // Wayland compositors place fullscreen surfaces by their associated output,
    // not by the X11-style global position of the widget. Bind the native
    // window before laying it out so its initial DPR belongs to this output.
    if (screen && !isX11Platform()) {
        winId();
        if (QWindow *window = windowHandle()) {
            if (window->screen() != screen)
                window->setScreen(screen);
        }
    }

    updateScreen(screen);
}

void FullscreenBackground::setContentVisible(bool contentVisible)
{
    if (this->contentVisible() == contentVisible)
        return;

    if (!m_content)
        return;

    if (!isVisible() && !contentVisible)
        return;

    m_content->setVisible(contentVisible);

    emit contentVisibleChanged(contentVisible);
}

void FullscreenBackground::setBackgroundFocused(bool focused)
{
    if (m_backgroundFocused == focused
        && m_focusAnimation->state() == QAbstractAnimation::Running) {
        return;
    }

    m_backgroundFocused = focused;

#ifdef DISABLE_ANIMATIONS
    setBackgroundFocusedImmediately(focused);
#else
    const qreal target = focused ? 1.0 : 0.0;
    if (qFuzzyCompare(m_focusProgress + 1.0, target + 1.0)) {
        setBackgroundFocusProgress(target);
        return;
    }

    m_focusAnimation->stop();
    m_focusAnimation->setDuration(qMax(1, qRound(
        kBackgroundFocusDuration * std::abs(target - m_focusProgress))));
    m_focusAnimation->setStartValue(m_focusProgress);
    m_focusAnimation->setEndValue(target);
    m_focusBackground->show();
    m_focusBackground->lower();
    m_focusAnimation->start();
#endif
}

void FullscreenBackground::setBackgroundFocusedImmediately(bool focused)
{
    m_backgroundFocused = focused;
    m_focusAnimation->stop();
    setBackgroundFocusProgress(focused ? 1.0 : 0.0);
}

void FullscreenBackground::setContent(QWidget * const w)
{
    Q_ASSERT(m_content.isNull());

    m_content = w;
    m_content->setParent(this);
    m_content->raise();
    m_content->move(0, 0);
}

void FullscreenBackground::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const float current_ani_value = m_fadeOutAni->currentValue().toFloat();

    // 绘制纯黑背景，避免出现壁纸设置错误导致控件与白色背景融为一体的问题
    painter.fillRect(rect(), QColor(0, 0, 0));

    if (!m_background.isNull())
        painter.drawPixmap(rect(), m_backgroundCache);

    if (!m_fakeBackground.isNull()) {
        // draw background
        painter.setOpacity(current_ani_value);
        painter.drawPixmap(rect(), m_fakeBackgroundCache);
        painter.setOpacity(1);
    }
}

void FullscreenBackground::enterEvent(QEnterEvent *event)
{
    m_content->show();
    emit contentVisibleChanged(true);

    return QWidget::enterEvent(event);
}

void FullscreenBackground::leaveEvent(QEvent *event)
{
    return QWidget::leaveEvent(event);
}

void FullscreenBackground::resizeEvent(QResizeEvent *event)
{
    refreshForCurrentSize();
    m_screenGeometryRefreshTimer->start();

    return QWidget::resizeEvent(event);
}

void FullscreenBackground::showEvent(QShowEvent *event)
{
    if (QWindow *w = windowHandle()) {
        connect(w, &QWindow::screenChanged, this, &FullscreenBackground::updateScreen, Qt::UniqueConnection);

        if (m_screen) {
            if (w->screen() != m_screen) {
                w->setScreen(m_screen);
            }

            // 更新窗口位置和大小
            updateGeometry();
        } else {
            updateScreen(w->screen());
        }
    }

    if (isX11Platform())
        m_x11GeometryPollTimer->start();

    return QWidget::showEvent(event);
}

void FullscreenBackground::hideEvent(QHideEvent *event)
{
    m_x11GeometryPollTimer->stop();
    return QWidget::hideEvent(event);
}

const QPixmap FullscreenBackground::pixmapHandle(const QPixmap &pixmap)
{
    const qreal pixelRatio = m_screen
        ? m_screen->devicePixelRatio()
        : devicePixelRatioF();
    const QSize trueSize = (QSizeF(size()) * pixelRatio).toSize();
    QPixmap pix = pixmap.scaled(trueSize,
                                Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);

    pix = pix.copy(QRect((pix.width() - trueSize.width()) / 2,
                         (pix.height() - trueSize.height()) / 2,
                         trueSize.width(),
                         trueSize.height()));

    // Preserve the target output's native pixel density while painting in logical coordinates.
    pix.setDevicePixelRatio(pixelRatio);

    return pix;
}

void FullscreenBackground::setBackgroundFocusProgress(qreal progress)
{
    m_focusProgress = qBound(0.0, progress, 1.0);
    m_focusBlurEffect->setBlurRadius(
        kBackgroundFocusBlurRadius * m_focusProgress);
    updateFocusBackground();
}

void FullscreenBackground::updateFocusBackground()
{
    if (m_backgroundCache.isNull() || qFuzzyIsNull(m_focusProgress)) {
        m_focusBackground->hide();
        return;
    }

    m_focusBackground->setPixmap(m_backgroundCache);

    const qreal scale = 1.0 + kBackgroundFocusZoom * m_focusProgress;
    const QSize focusSize = (QSizeF(size()) * scale).toSize();
    m_focusBackground->setGeometry(
        (width() - focusSize.width()) / 2,
        (height() - focusSize.height()) / 2,
        focusSize.width(),
        focusSize.height());
    m_focusBackground->show();
    m_focusBackground->lower();
}

void FullscreenBackground::refreshForCurrentSize()
{
    if (m_content)
        m_content->resize(size());
    m_backgroundCache = pixmapHandle(m_background);
    m_fakeBackgroundCache = pixmapHandle(m_fakeBackground);
    updateFocusBackground();
    update();
}

void FullscreenBackground::updateScreen(QScreen *screen)
{
    if (screen == m_screen)
        return;

    if (m_screen) {
        disconnect(m_screen, &QScreen::geometryChanged, this, &FullscreenBackground::updateGeometry);
        disconnect(m_screen, &QScreen::logicalDotsPerInchChanged, this, &FullscreenBackground::updateGeometry);
        disconnect(m_screen, &QScreen::physicalDotsPerInchChanged, this, &FullscreenBackground::updateGeometry);
    }

    if (screen) {
        connect(screen, &QScreen::geometryChanged, this, &FullscreenBackground::updateGeometry);
        connect(screen, &QScreen::logicalDotsPerInchChanged, this, &FullscreenBackground::updateGeometry);
        connect(screen, &QScreen::physicalDotsPerInchChanged, this, &FullscreenBackground::updateGeometry);
    }

    m_screen = screen;

    if (m_screen)
        updateGeometry();
}

void FullscreenBackground::updateGeometry()
{
    if (!m_screen)
        return;

    applyScreenGeometry();
    refreshForCurrentSize();
}

void FullscreenBackground::applyScreenGeometry()
{
    if (!m_screen)
        return;

    if (isX11Platform()) {
        const QRect nativeGeometry = x11OutputGeometry(m_screen);
        const QRect targetGeometry = nativeGeometry.isEmpty()
            ? m_screen->geometry() : nativeGeometry;
        if (geometry() != targetGeometry)
            setGeometry(targetGeometry);
    } else if (!isVisible()) {
        // Once mapped, the Wayland compositor's configure/Resize size is
        // authoritative. QScreen may still contain the previous VM mode.
        resize(m_screen->geometry().size());
    }
}
