/*
 *   Copyright (C) 2007 by Aaron Seigo aseigo@kde.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "icon.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

//#define PROVE_IT_CAN_BE_DONE

#ifdef PROVE_IT_CAN_BE_DONE
#include <private/qwindowsurface_p.h>
#endif

#include <KIcon>
#include <KImageEffect>
#include <KUrl>
#include <KRun>
#include <KMimeType>
#include <KDebug>

#include "phase.h"
#include "svg.h"
#include "effects/blur.cpp"

namespace Plasma
{

class Icon::Private
{
    public:
        Private()
            : size(128*1.1, 128*1.1),
              iconSize(128, 128),
              state(Private::NoState),
              svg("widgets/iconbutton"),
              svgElements(0),
              button1AnimId(0)
        {
            svg.setContentType(Plasma::Svg::ImageSet);
            svg.resize(size);

            //TODO: recheck when svg changes
            checkSvgElements();
        }

        void checkSvgElements()
        {
            if (svg.elementExists("background")) {
                svgElements |= SvgBackground;
            }

            if (svg.elementExists("background-hover")) {
                svgElements |= SvgBackgroundHover;
            }

            if (svg.elementExists("background-pressed")) {
                svgElements |= SvgBackgroundPressed;
            }

            if (svg.elementExists("foreground")) {
                svgElements |= SvgForeground;
            }

            if (svg.elementExists("foreground-hover")) {
                svgElements |= SvgForegroundHover;
            }

            if (svg.elementExists("foreground-pressed")) {
                svgElements |= SvgForegroundPressed;
            }

            if (svg.elementExists("minibutton")) {
                svgElements |= SvgMinibutton;
            }

            if (svg.elementExists("minibutton-hover")) {
                svgElements |= SvgMinibuttonHover;
            }

            if (svg.elementExists("minibutton-pressed")) {
                svgElements |= SvgMinibuttonPressed;
            }

            button1Pressed = false;
            button1Hovered = false;
        }

        ~Private() {}
        enum ButtonState
        {
            NoState,
            HoverState,
            PressedState
        };


        enum { NoSvg = 0,
               SvgBackground = 1,
               SvgBackgroundHover = 2,
               SvgBackgroundPressed = 4,
               SvgForeground = 8,
               SvgForegroundHover = 16,
               SvgForegroundPressed = 32,
               SvgMinibutton = 64,
               SvgMinibuttonHover = 256,
               SvgMinibuttonPressed = 128};

        KUrl url;
        QString text;
        QSizeF size;
        QSizeF iconSize;
        QIcon icon;
        ButtonState state;
        Svg svg;
        int svgElements;
        //TODO: create a proper state int for this, as we do with ButtonState
        //      for each possible button
        bool button1Hovered;
        bool button1Pressed;
        Phase::AnimId button1AnimId;
};

Icon::Icon(QGraphicsItem *parent)
    : QObject(),
      QGraphicsItem(parent),
      d(new Private)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptsHoverEvents(true);
    setEnabled(true);
    setPos(QPointF(0.0,0.0));
}

Icon::~Icon()
{
    delete d;
}

QRectF Icon::boundingRect() const
{
    return QRectF(QPointF(0, 0), d->size);
}

void Icon::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    painter->fillRect(boundingRect(), Qt::transparent);

#ifdef PROVE_IT_CAN_BE_DONE
     if (d->state == Private::HoverState && scene()) {
         QList<QGraphicsView*> views = scene()->views();
         if (views.count() > 0) {
             QPixmap* pix = static_cast<QPixmap*>(views[0]->windowSurface()->paintDevice());
             QImage image(boundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
             {
                 QPainter p(&image);
                 p.drawPixmap(image.rect(), *pix, sceneBoundingRect());
             }
             expblur<16,7>(image, 8);
             painter->save();
             painter->drawImage(0, 0, image);
             painter->restore();
         }
     }
#endif

    QString element;
    if (d->svgElements & Private::SvgBackground) {
        element = "background";
    }

    switch (d->state) {
        case Private::NoState:
            break;
        case Private::HoverState:
            if (d->svgElements & Private::SvgBackgroundHover) {
                element = "background-hover";
            }
            break;
        case Private::PressedState:
            if (d->svgElements & Private::SvgBackgroundPressed) {
                element = "background-pressed";
            } else if (d->svgElements & Private::SvgBackgroundHover) {
                element = "background-hover";
            }
            break;
    }

    if (!element.isEmpty()) {
        d->svg.paint(painter, 0, 0, element);
        element = QString();
    }

    if (!d->icon.isNull()) {
        qreal iw = d->iconSize.width();
        qreal ih = d->iconSize.height();
        qreal deltaX = (d->size.width() - iw) / 2;
        qreal deltaY = (d->size.height() - ih) / 2 ;
        if (d->state == Private::PressedState) {
            painter->save();
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->scale(0.95, 0.95);
            deltaY *= 0.95;
            deltaX *= 0.95;
            painter->drawPixmap((int)(deltaX + (iw * .025)), (int)(deltaY + (ih * .025)),
                                d->icon.pixmap(d->iconSize.toSize()));
            painter->restore();
        } else {
            painter->drawPixmap((int)deltaX, (int)deltaY, d->icon.pixmap(d->iconSize.toSize()));
        }
    }

    //TODO: draw text

    // Make it default
    if (d->svgElements & Private::SvgForeground) {
        element = "foreground";
    }

    switch (d->state) {
        case Private::NoState:
            break;
        case Private::HoverState:
            if (d->svgElements & Private::SvgForegroundHover) {
                element = "foreground-hover";
            }
            break;
        case Private::PressedState:
            if (d->svgElements & Private::SvgForegroundPressed) {
                element = "foreground-pressed";
            } else if (d->svgElements & Private::SvgForegroundHover) {
                element = "foreground-hover";
            }
            break;
    }

    if (!element.isEmpty()) {
        //kDebug() << "painting " << element << endl;
        d->svg.paint(painter, 0, 0, element);
        element.clear();
    }

    // Draw top-left button
    if (d->button1AnimId) {
        painter->drawPixmap(6, 6, Phase::self()->animationResult(d->button1AnimId));
    }
}

void Icon::setText(const QString& text)
{
    d->text = text;
//TODO: implement this puppy calculateSize();
}

QString Icon::text() const
{
    return d->text;
}

void Icon::setIcon(const QString& icon)
{
    kDebug() << "Icon set: " << icon << endl;
    if (icon.isEmpty()) {
        setIcon(QIcon());
        return;
    }

    setIcon(KIcon(icon));
}

void Icon::setIcon(const QIcon& icon)
{
    d->icon = icon;
    update();
}

void Icon::setUrl(const KUrl& url)
{
    d->url = url;
}

KUrl Icon::url()
{
    return d->url;
}

QSizeF Icon::size() const
{
    return d->size;
}

QSizeF Icon::iconSize() const
{
    return d->iconSize;
}

void Icon::setSize(const QSizeF& s)
{
    prepareGeometryChange();
    d->iconSize = s;
    d->size = s * 1.1;
    d->svg.resize(d->size);
    update();
}

void Icon::setSize(int w, int h)
{
    setSize(QSizeF(w, h));
}

bool Icon::isDown()
{
    return d->state == Private::PressedState;
}

void Icon::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QRectF button1(6, 6, 32, 32); // The top-left circle
    d->button1Pressed = button1.contains(event->pos());
    if (!d->button1Pressed) {
        d->state = Private::PressedState;
        QGraphicsItem::mousePressEvent(event);
        update();
        d->button1Pressed = false;
    }
}

void Icon::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    bool inside = boundingRect().contains(event->pos());
    bool wasClicked = d->state == Private::PressedState && inside;

    if (inside) {
        d->state = Private::HoverState;

        QRectF button1(6, 6, 32, 32); // The top-left circle
        d->button1Hovered = button1.contains(event->pos());
        if (d->button1Hovered &&
            d->button1Pressed && d->url.isValid()) {
            KRun::runUrl(d->url, KMimeType::findByUrl(d->url)->name(), 0);
            wasClicked = false;
        }
    } else {
        d->state = Private::NoState;
    }

    if (wasClicked) {
        emit pressed(false);
        emit clicked();
    }

    d->button1Pressed = false;
    QGraphicsItem::mouseReleaseEvent(event);
    update();
}

QPixmap Icon::buttonPixmap()
{
    //TODO this is just full of assumptions such as sizes and icon names. ugh!
    QPixmap alpha(26, 26);
    {
        QPainter painter(&alpha);
        painter.fillRect(alpha.rect(), Qt::black);
    }

    QPixmap pix(26, 26);
    pix.setAlphaChannel(alpha);
    QPainter painter(&pix);
    painter.fillRect(pix.rect(), Qt::transparent);
    QString element;

    if (d->svgElements & Private::SvgMinibutton) {
        element = "minibutton";
    }

    if (d->button1Pressed) {
        if (d->svgElements & Private::SvgMinibuttonPressed) {
            element = "minibutton-pressed";
        } else if (d->svgElements & Private::SvgMinibuttonHover) {
            element = "minibutton-hover";
        }
    } else if (d->button1Hovered) {
        if (d->svgElements & Private::SvgMinibuttonHover) {
            element = "minibutton-hover";
        }
    }

    //paint foreground element
    if (!element.isEmpty()) {
        //kDebug() << "painting " << element << endl;
        d->svg.resize(pix.size());
        d->svg.paint(&painter, 0, 0, element);
        d->svg.resize(boundingRect().size());
    }

    KIcon exec("exec");
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.drawPixmap(2, 2, exec.pixmap(22,22));

    return pix;
}

void Icon::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    d->state = Private::HoverState;
    d->button1Pressed = false;
    QRectF button1(6, 6, 32, 32); // The top-left circle
    d->button1Hovered = button1.contains(event->pos());

    if (d->button1AnimId) {
        Phase::self()->stopElementAnimation(d->button1AnimId);
    }

    d->button1AnimId = Phase::self()->startElementAnimation(this, Phase::ElementAppear);
    Phase::self()->setAnimationPixmap(d->button1AnimId, buttonPixmap());
    QGraphicsItem::hoverEnterEvent(event);
}

void Icon::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    d->button1Pressed = false;
    d->button1Hovered = false;

    if (d->button1AnimId) {
        Phase::self()->stopElementAnimation(d->button1AnimId);
    }

    d->button1AnimId = Phase::self()->startElementAnimation(this, Phase::ElementDisappear);
    Phase::self()->setAnimationPixmap(d->button1AnimId, buttonPixmap());

    d->state = Private::NoState;
    QGraphicsItem::hoverLeaveEvent(event);
}

void Icon::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->button1Pressed) {
        d->button1Pressed = false;
    }

    if (d->state == Private::PressedState) {
        d->state = Private::HoverState;
    }

    QGraphicsItem::mouseMoveEvent(event);
}

QSizeF Icon::sizeHint() const
{
    return d->size;
}

QSizeF Icon::minimumSize() const
{
    return d->size; // probably should be more flexible on this =)
}

QSizeF Icon::maximumSize() const
{
    return d->size;
}

Qt::Orientations Icon::expandingDirections() const
{
    return Qt::Horizontal;
}

bool Icon::hasHeightForWidth() const
{
    return true;
}

qreal Icon::heightForWidth(qreal w) const
{
    return w; //FIXME: we shouldn't assume squareness but actually calculate based on text and what not
}

bool Icon::hasWidthForHeight() const
{
    return true;
}

qreal Icon::widthForHeight(qreal h) const
{
    return h; //FIXME: we shouldn't assume squareness but actually calculate based on text and what not
}

QRectF Icon::geometry() const
{
    return boundingRect().toRect();
}

void Icon::setGeometry(const QRectF &r)
{
    setSize(r.size());
    setPos(r.x(),r.y());
}

int Icon::boundsForIconSize(int iconSize)
{
    return iconSize * 1.1;
}

} // namespace Plasma

#include "icon.moc"
