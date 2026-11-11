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

#include "neoarrowrectangle.h"
#include <X11/extensions/shape.h>
#include <QScreen>
#include <QApplication>
#include <dgraphicsgloweffect.h>

DWIDGET_USE_NAMESPACE

namespace {

QRect movementBounds(const QWidget *widget, const QPoint &anchor)
{
    if (widget->parentWidget())
        return widget->parentWidget()->rect();

    QScreen *targetScreen = QGuiApplication::screenAt(anchor);
    if (!targetScreen)
        targetScreen = widget->screen();
    if (!targetScreen)
        targetScreen = QGuiApplication::primaryScreen();

    return targetScreen ? targetScreen->geometry() : QRect();
}

}

NeoArrowRectangle::NeoArrowRectangle(ArrowDirection direction, QWidget * parent) :
    QWidget(parent),m_arrowDirection(direction)
{
    DGraphicsGlowEffect *glowEffect = new DGraphicsGlowEffect(this);
    glowEffect->setBlurRadius(shadowBlurRadius());
    glowEffect->setDistance(shadowDistance());
    glowEffect->setXOffset(shadowXOffset());
    glowEffect->setYOffset(shadowYOffset());
    setGraphicsEffect(glowEffect);
}

void NeoArrowRectangle::show(int x, int y)
{
    m_lastPos = QPoint(x, y);
    move(x, y);//Overload function
    if (isHidden())
        QWidget::show();

    resizeWithContent();
    repaint();
}

void NeoArrowRectangle::setContent(QWidget *content)
{
    if (!content)
        return;
    if (m_content)
        m_content->setParent(NULL);

    m_content = content;
    m_content->setParent(this);
    m_content->show();

    qreal delta = shadowBlurRadius() + shadowDistance() + margin();

    resizeWithContent();

    switch(m_arrowDirection)
    {
    case ArrowLeft:
        m_content->move(m_arrowHeight + delta, delta);
        break;
    case ArrowRight:
        m_content->move(delta, delta);
        break;
    case ArrowTop:
        m_content->move(delta, delta + m_arrowHeight);
        break;
    case ArrowBottom:
        m_content->move(delta, delta);
        break;
    }

    repaint();
}

void NeoArrowRectangle::resizeWithContent()
{
    setFixedSize(getFixedSize());

    repaint();

    //Shadow Transparent For MouseEvents
    qreal delta = shadowBlurRadius() + shadowDistance();
    Q_UNUSED(delta);
//    XRectangle m_contentXRect;
//    m_contentXRect.x = 0;
//    m_contentXRect.y = 0;
//    m_contentXRect.width = width() - delta * 2;
//    m_contentXRect.height = height() - delta * 2;
//    XShapeCombineRectangles(QX11Info::display(), winId(), ShapeInput,
//                            delta + shadowXOffset(),
//                            delta + shadowYOffset(),
//                            &m_contentXRect, 1, ShapeSet, YXBanded);
}

QSize NeoArrowRectangle::getFixedSize()
{
    if (m_content)
    {
        qreal delta = shadowBlurRadius() + shadowDistance() + margin();

        switch(m_arrowDirection)
        {
        case ArrowLeft:
        case ArrowRight:
            return QSize(m_content->width() + delta * 2 + m_arrowHeight, m_content->height() + delta * 2);
        case ArrowTop:
        case ArrowBottom:
            return QSize(m_content->width() + delta * 2, m_content->height() + delta * 2 + m_arrowHeight);
        }
    }

    return QSize(0, 0);
}

void NeoArrowRectangle::move(int x, int y)
{
    switch (m_arrowDirection)
    {
    case ArrowLeft:
    case ArrowRight:
        verticalMove(x, y);
        break;
    case ArrowTop:
    case ArrowBottom:
        horizontalMove(x, y);
        break;
    default:
        QWidget::move(x, y);
        break;
    }
}

// override methods
void NeoArrowRectangle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath border;

    switch (m_arrowDirection)
    {
    case NeoArrowRectangle::ArrowLeft:
        border = getLeftCornerPath();
        break;
    case NeoArrowRectangle::ArrowRight:
        border = getRightCornerPath();
        break;
    case NeoArrowRectangle::ArrowTop:
        border = getTopCornerPath();
        break;
    case NeoArrowRectangle::ArrowBottom:
        border = getBottomCornerPath();
        break;
    default:
        border = getRightCornerPath();
    }

    painter.setClipPath(border);
    painter.fillPath(border, QBrush(m_backgroundColor));

    QPen strokePen;
    strokePen.setColor(m_borderColor);
    strokePen.setWidth(m_borderWidth);
    painter.strokePath(border, strokePen);
}
qreal NeoArrowRectangle::shadowYOffset() const
{
    return m_shadowYOffset;
}

void NeoArrowRectangle::setShadowYOffset(const qreal &shadowYOffset)
{
    m_shadowYOffset = shadowYOffset;
}

qreal NeoArrowRectangle::shadowXOffset() const
{
    return m_shadowXOffset;
}

void NeoArrowRectangle::setShadowXOffset(const qreal &shadowXOffset)
{
    m_shadowXOffset = shadowXOffset;
}

qreal NeoArrowRectangle::shadowDistance() const
{
    return m_shadowDistance;
}

void NeoArrowRectangle::setShadowDistance(const qreal &shadowDistance)
{
    m_shadowDistance = shadowDistance;
}

qreal NeoArrowRectangle::shadowBlurRadius() const
{
    return m_shadowBlurRadius;
}

void NeoArrowRectangle::setShadowBlurRadius(const qreal &shadowBlurRadius)
{
    m_shadowBlurRadius = shadowBlurRadius;
}

QColor NeoArrowRectangle::borderColor() const
{
    return m_borderColor;
}

void NeoArrowRectangle::setBorderColor(const QColor &borderColor)
{
    m_borderColor = borderColor;
}

int NeoArrowRectangle::borderWidth() const
{
    return m_borderWidth;
}

void NeoArrowRectangle::setBorderWidth(int borderWidth)
{
    m_borderWidth = borderWidth;
}

QColor NeoArrowRectangle::backgroundColor() const
{
    return m_backgroundColor;
}

NeoArrowRectangle::ArrowDirection NeoArrowRectangle::arrowDirection() const
{
    return m_arrowDirection;
}

void NeoArrowRectangle::setBackgroundColor(const QColor &backgroundColor)
{
    m_backgroundColor = backgroundColor;
}

int NeoArrowRectangle::radius() const
{
    return this->m_radius;
}

int NeoArrowRectangle::arrowHeight() const
{
    return this->m_arrowHeight;
}

int NeoArrowRectangle::arrowWidth() const
{
    return this->m_arrowWidth;
}

int NeoArrowRectangle::arrowX() const
{
    return this->m_arrowX;
}

int NeoArrowRectangle::arrowY() const
{
    return this->m_arrowY;
}

int NeoArrowRectangle::margin() const
{
    return this->m_margin;
}

void NeoArrowRectangle::setArrowDirection(ArrowDirection value)
{
    m_arrowDirection = value;
}

void NeoArrowRectangle::setWidth(int value)
{
    this->setFixedWidth(value);
}

void NeoArrowRectangle::setHeight(int value)
{
    this->setFixedHeight(value);
}

void NeoArrowRectangle::setRadius(int value)
{
    this->m_radius = value;
}

void NeoArrowRectangle::setArrowHeight(int value)
{
    this->m_arrowHeight = value;
}

void NeoArrowRectangle::setArrowWidth(int value)
{
    this->m_arrowWidth = value;
}

void NeoArrowRectangle::setArrowX(int value)
{
    this->m_arrowX = value;
}

void NeoArrowRectangle::setArrowY(int value)
{
    this->m_arrowY = value;
}

void NeoArrowRectangle::setMargin(int value)
{
    this->m_margin = value;
}

QPainterPath NeoArrowRectangle::getLeftCornerPath()
{
    qreal delta = shadowBlurRadius() + shadowDistance();

    QRect rect = this->rect().marginsRemoved(QMargins(delta, delta, delta, delta));

    QPoint cornerPoint(rect.x(), rect.y() + (m_arrowY > 0 ? m_arrowY : (rect.height() / 2)));
    QPoint topLeft(rect.x() + m_arrowHeight, rect.y());
    QPoint topRight(rect.x() + rect.width(), rect.y());
    QPoint bottomRight(rect.x() + rect.width(), rect.y() + rect.height());
    QPoint bottomLeft(rect.x() + m_arrowHeight, rect.y() + rect.height());
    int radius = this->m_radius > (rect.height() / 2) ? (rect.height() / 2) : this->m_radius;

    QPainterPath border;
    border.moveTo(topLeft.x() - radius,topLeft.y());
    border.lineTo(topRight.x() - radius, topRight.y());
    border.arcTo(topRight.x() - 2 * radius, topRight.y(), 2 * radius, 2 * radius, 90, -90);
    border.lineTo(bottomRight.x(), bottomRight.y() - radius);
    border.arcTo(bottomRight.x() - 2 * radius, bottomRight.y() - 2 * radius, 2 * radius, 2 * radius, 0, -90);
    border.lineTo(bottomLeft.x() - radius,bottomLeft.y());
    border.arcTo(bottomLeft.x(),bottomLeft.y() - 2 * radius,2 * radius,2 * radius,-90,-90);
    border.lineTo(cornerPoint.x() + m_arrowHeight,cornerPoint.y() + m_arrowWidth / 2);
    border.lineTo(cornerPoint);
    border.lineTo(cornerPoint.x() + m_arrowHeight,cornerPoint.y() - m_arrowWidth / 2);
    border.lineTo(topLeft.x(),topLeft.y() + radius);
    border.arcTo(topLeft.x(),topLeft.y(),2 * radius,2 * radius,-180,-90);
    border.lineTo(topLeft.x() - radius,topLeft.y());

    return border;
}

QPainterPath NeoArrowRectangle::getRightCornerPath()
{
    qreal delta = shadowBlurRadius() + shadowDistance();

    QRect rect = this->rect().marginsRemoved(QMargins(delta, delta, delta, delta));

    QPoint cornerPoint(rect.x() + rect.width(), rect.y() + (m_arrowY > 0 ? m_arrowY : rect.height() / 2));
    QPoint topLeft(rect.x(), rect.y());
    QPoint topRight(rect.x() + rect.width() - m_arrowHeight, rect.y());
    QPoint bottomRight(rect.x() + rect.width() - m_arrowHeight, rect.y() + rect.height());
    QPoint bottomLeft(rect.x(), rect.y() + rect.height());
    int radius = this->m_radius > (rect.height() / 2) ? rect.height() / 2 : this->m_radius;

    QPainterPath border;
    border.moveTo(topLeft.x() + radius, topLeft.y());
    border.lineTo(topRight.x() - radius,topRight.y());
    border.arcTo(topRight.x() - 2 * radius,topRight.y(),2 * radius,2 * radius,90,-90);
    border.lineTo(cornerPoint.x() - m_arrowHeight,cornerPoint.y() - m_arrowWidth / 2);
    border.lineTo(cornerPoint);
    border.lineTo(cornerPoint.x() - m_arrowHeight,cornerPoint.y() + m_arrowWidth / 2);
    border.lineTo(bottomRight.x(),bottomRight.y() - radius);
    border.arcTo(bottomRight.x() - 2 * radius,bottomRight.y() - 2 * radius,2 * radius,2 * radius,0,-90);
    border.lineTo(bottomLeft.x() + radius, bottomLeft.y());
    border.arcTo(bottomLeft.x(), bottomLeft.y() - 2 * radius, 2 * radius, 2 * radius, -90, -90);
    border.lineTo(topLeft.x(), topLeft.y() + radius);
    border.arcTo(topLeft.x(), topLeft.y(), 2 * radius, 2 * radius, 180, -90);

    return border;
}

QPainterPath NeoArrowRectangle::getTopCornerPath()
{
    qreal delta = shadowBlurRadius() + shadowDistance();

    QRect rect = this->rect().marginsRemoved(QMargins(delta, delta, delta, delta));

    QPoint cornerPoint = m_cornerPoint.isNull() ? QPoint(rect.x() + (m_arrowX > 0 ? m_arrowX : rect.width() / 2), rect.y()) : m_cornerPoint;

    QPoint topLeft(rect.x(), rect.y() + m_arrowHeight);
    QPoint topRight(rect.x() + rect.width(), rect.y() + m_arrowHeight);
    QPoint bottomRight(rect.x() + rect.width(), rect.y() + rect.height());
    QPoint bottomLeft(rect.x(), rect.y() + rect.height());
    int radius = this->m_radius > (rect.height() / 2 - m_arrowHeight) ? rect.height() / 2 -m_arrowHeight : this->m_radius;

    QPainterPath border;
    border.moveTo(topLeft.x() + radius, topLeft.y());
    border.lineTo(cornerPoint.x() - m_arrowWidth / 2, cornerPoint.y() + m_arrowHeight);
    border.lineTo(cornerPoint);
    border.lineTo(cornerPoint.x() + m_arrowWidth / 2, cornerPoint.y() + m_arrowHeight);
    border.lineTo(topRight.x() - radius, topRight.y());
    border.arcTo(topRight.x() - 2 * radius, topRight.y(), 2 * radius, 2 * radius, 90, -90);
    border.lineTo(bottomRight.x(), bottomRight.y() - radius);
    border.arcTo(bottomRight.x() - 2 * radius, bottomRight.y() - 2 * radius, 2 * radius, 2 * radius, 0, -90);
    border.lineTo(bottomLeft.x() + radius, bottomLeft.y());
    border.arcTo(bottomLeft.x(), bottomLeft.y() - 2 * radius, 2 * radius, 2 * radius, - 90, -90);
    border.lineTo(topLeft.x(), topLeft.y() + radius);
    border.arcTo(topLeft.x(), topLeft.y(), 2 * radius, 2 * radius, 180, -90);

    return border;
}

QPainterPath NeoArrowRectangle::getBottomCornerPath()
{
    qreal delta = shadowBlurRadius() + shadowDistance();

    QRect rect = this->rect().marginsRemoved(QMargins(delta, delta, delta, delta));

    QPoint cornerPoint(rect.x() + (m_arrowX > 0 ? m_arrowX : rect.width() / 2), rect.y()  + rect.height());
    QPoint topLeft(rect.x(), rect.y());
    QPoint topRight(rect.x() + rect.width(), rect.y());
    QPoint bottomRight(rect.x() + rect.width(), rect.y() + rect.height() - m_arrowHeight);
    QPoint bottomLeft(rect.x(), rect.y() + rect.height() - m_arrowHeight);
    int radius = this->m_radius > (rect.height() / 2 - m_arrowHeight) ? rect.height() / 2 -m_arrowHeight : this->m_radius;

    QPainterPath border;
    border.moveTo(topLeft.x() + radius, topLeft.y());
    border.lineTo(topRight.x() - radius, topRight.y());
    border.arcTo(topRight.x() - 2 * radius, topRight.y(), 2 * radius, 2 * radius, 90, -90);
    border.lineTo(bottomRight.x(), bottomRight.y() - radius);
    border.arcTo(bottomRight.x() - 2 * radius, bottomRight.y() - 2 * radius, 2 * radius, 2 * radius, 0, -90);
    border.lineTo(cornerPoint.x() + m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight);
    border.lineTo(cornerPoint);
    border.lineTo(cornerPoint.x() - m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight);
    border.lineTo(bottomLeft.x() + radius, bottomLeft.y());
    border.arcTo(bottomLeft.x(), bottomLeft.y() - 2 * radius, 2 * radius, 2 * radius, -90, -90);
    border.lineTo(topLeft.x(), topLeft.y() + radius);
    border.arcTo(topLeft.x(), topLeft.y(), 2 * radius, 2 * radius, 180, -90);

    return border;
}

void NeoArrowRectangle::verticalMove(int x, int y)
{
    const QRect dRect = movementBounds(this, QPoint(x, y));
    qreal delta = shadowBlurRadius() - shadowDistance();

    int lRelativeY = y - dRect.y() - (height() - delta) / 2;
    int rRelativeY = y - dRect.y() + (height() - delta) / 2 - dRect.height();
    int absoluteY = 0;

    if (lRelativeY < 0)//out of screen in top side
    {
        //arrowY use relative coordinates
        setArrowY(height() / 2 - delta + lRelativeY);
        absoluteY = dRect.y() - delta;
    }
    else if(rRelativeY > 0)//out of screen in bottom side
    {
        setArrowY(height() / 2 - delta * 2 + rRelativeY);
        absoluteY = dRect.y() + dRect.height() - height() + delta;
    }
    else
        absoluteY = y - height() / 2;

    switch (m_arrowDirection)
    {
    case ArrowLeft:
        QWidget::move(x, absoluteY);
        break;
    case ArrowRight:
        QWidget::move(x - width(), absoluteY);
        break;
    default:
        break;
    }
}

void NeoArrowRectangle::horizontalMove(int x, int y)
{
    const QRect dRect = movementBounds(this, QPoint(x, y));
    qreal delta = shadowBlurRadius() - shadowDistance();

    int lRelativeX = x - dRect.x() - (width() - delta) / 2;
    int rRelativeX = x - dRect.x() + (width() - delta) / 2 - dRect.width();
    int absoluteX = 0;

    if (lRelativeX < 0)//out of screen in left side
    {
        //arrowX use relative coordinates
        setArrowX(width() / 2 - delta + lRelativeX);
        absoluteX = dRect.x() - delta;
    }
    else if(rRelativeX > 0)//out of screen in right side
    {
        setArrowX(width() / 2 - delta * 2 + rRelativeX);
        absoluteX = dRect.x() + dRect.width() - width() + delta;
    }
    else
        absoluteX = x - width() / 2;

    switch (m_arrowDirection)
    {
    case ArrowTop:
        QWidget::move(absoluteX, y);
        break;
    case ArrowBottom:
        QWidget::move(absoluteX, y - height());
        break;
    default:
        break;
    }
}

void NeoArrowRectangle::setCornerPoint(const QPoint &cornerPoint)
{
    m_cornerPoint = cornerPoint;
}

void NeoArrowRectangle::moveToPos(const QPoint &topleft)
{
    QWidget::move(topleft);
}

NeoArrowRectangle::~NeoArrowRectangle()
{

}

