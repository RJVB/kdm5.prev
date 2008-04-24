/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *                 2007 Alexis Ménard <darktears31@gmail.com>
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

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <QtCore/QObject>
#include <QtGui/QRegion>
#include <QtGui/QPixmap>

#include <kgenericfactory.h>

#include <plasma/plasma_export.h>
#include <plasma/phase.h>

class QGraphicsItem;

namespace Plasma
{

class PLASMA_EXPORT Animator : public QObject
{
    Q_OBJECT

public:
    explicit Animator(QObject *parent = 0);
    ~Animator();

    // Parameter definitions
    virtual int animationFPS(Plasma::Phase::Animation) const;
    virtual int movementAnimationFPS(Plasma::Phase::Movement) const;
    virtual int elementAnimationFPS(Plasma::Phase::Animation) const;
    virtual int animationDuration(Plasma::Phase::Animation) const;
    virtual int movementAnimationDuration(Plasma::Phase::Movement) const;
    virtual int elementAnimationDuration(Plasma::Phase::Animation) const;
    virtual Phase::CurveShape animationCurve(Plasma::Phase::Animation) const;
    virtual Phase::CurveShape movementAnimationCurve(Plasma::Phase::Movement) const;
    virtual Phase::CurveShape elementAnimationCurve(Plasma::Phase::Animation) const;

    // Element animations
    virtual QPixmap elementAppear(qreal progress, const QPixmap& pixmap);
    virtual QPixmap elementDisappear(qreal progress, const QPixmap& pixmap);

    // Item animations
    virtual void itemAppear(qreal progress, QGraphicsItem* item);
    virtual void itemDisappear(qreal progress, QGraphicsItem* item);
    virtual void itemActivated(qreal progress, QGraphicsItem* item);

    // Item movements
    virtual void itemSlideIn(qreal progress, QGraphicsItem* item, const QPoint &start, const QPoint &destination);
    virtual void itemSlideOut(qreal progress, QGraphicsItem* item, const QPoint &start, const QPoint &destination);

private:
    class Private;
    Private * const d;
};

} // Plasma namespace

#define K_EXPORT_PLASMA_ANIMATOR(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_animator_" #libname))

#endif // multiple inclusion guard
