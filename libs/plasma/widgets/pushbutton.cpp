/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#include "pushbutton.h"

#include <QPushButton>
#include <QPainter>

#include <KMimeType>

#include "theme.h"
#include "svg.h"

namespace Plasma
{

class PushButton::Private
{
public:
    Private()
        : svg(0)
    {
    }

    ~Private()
    {
        delete svg;
    }

    void setPixmap(PushButton *q)
    {
        if (imagePath.isEmpty()) {
            return;
        }

        KMimeType::Ptr mime = KMimeType::findByPath(absImagePath);
        QPixmap pm(q->size().toSize());

        if (mime->is("image/svg+xml")) {
            svg = new Svg();
            QPainter p(&pm);
            svg->paint(&p, pm.rect());
        } else {
            pm = QPixmap(absImagePath);
        }

        static_cast<QPushButton*>(q->widget())->setIcon(QIcon(pm));
    }

    QString imagePath;
    QString absImagePath;
    Svg *svg;
};

PushButton::PushButton(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new Private)
{
    QPushButton* native = new QPushButton;
    connect(native, SIGNAL(clicked()), this, SIGNAL(clicked()));
    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);
}

PushButton::~PushButton()
{
    delete d;
}

void PushButton::setText(const QString &text)
{
    static_cast<QPushButton*>(widget())->setText(text);
}

QString PushButton::text() const
{
    return static_cast<QPushButton*>(widget())->text();
}

void PushButton::setImage(const QString &path)
{
    if (d->imagePath == path) {
        return;
    }

    delete d->svg;
    d->svg = 0;
    d->imagePath = path;

    bool absolutePath = !path.isEmpty() && 
                        #ifdef Q_WS_WIN
                            !QDir::isRelativePath(path)
                        #else
                            path[0] == '/'
                        #endif
        ;

    if (absolutePath) {
        d->absImagePath = path;
    } else {
        //TODO: package support
        d->absImagePath = Theme::defaultTheme()->imagePath(path);
    }

    d->setPixmap(this);
}

QString PushButton::image() const
{
    return d->imagePath;
}

void PushButton::setStylesheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString PushButton::stylesheet()
{
    return widget()->styleSheet();
}

QPushButton* PushButton::nativeWidget() const
{
    return static_cast<QPushButton*>(widget());
}

void PushButton::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->setPixmap(this);
    QGraphicsProxyWidget::resizeEvent(event);
}

} // namespace Plasma

#include <pushbutton.moc>
