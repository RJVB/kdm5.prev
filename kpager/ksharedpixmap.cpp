/* vi: ts=8 sts=4 sw=4
 *
 *
 * This file is part of the KDE libraries.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * Shared pixmap client for KDE.
 */
#include "config.h"

#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QCharRef>
#include <QtGui/QPixmap>
#include <QtGui/QWidgetList>
#include <QtGui/QWidget>

#include "ksharedpixmap.h"

#ifdef Q_WS_X11
#include <qx11info_x11.h>

#include <kdebug.h>
#include <stdlib.h> // for abs
#include <stdio.h>

#include <X11/Xlib.h>

// Make sure to include all this X-based shit before we clean up the mess.
// Needed for --enable-final. Not needed by this file itself!
#include <X11/Xutil.h>

#include <netwm.h>

#include <fixx11h.h>

/**
 * KSharedPixmap
 */

class KSharedPixmap::KSharedPixmapPrivate
{
public:
  Atom pixmap;
  Atom target;
  Atom selection;
  QRect rect;
  QPixmap pixmap_data;
};

KSharedPixmap::KSharedPixmap()
    : QWidget( 0 ),
      d( new KSharedPixmapPrivate )
{
    setObjectName("shpixmap comm window");
    init();
}


KSharedPixmap::~KSharedPixmap()
{
    delete d;
    d = 0;
}


void KSharedPixmap::init()
{
    char pixmapName[] = "PIXMAP";
    char targetName[50];
    snprintf(targetName, 49, "target prop for window %lx",
             static_cast<unsigned long>(winId()));
    char *names[2] = { pixmapName, targetName };

    Atom atoms[2];
    XInternAtoms(QX11Info::display(), names, 2, False, atoms);
    d->pixmap = atoms[0];
    d->target = atoms[1];
    d->selection = None;
}


bool KSharedPixmap::isAvailable(const QString & name) const
{
    QString str = QString("KDESHPIXMAP:%1").arg(name);
    Atom sel = XInternAtom(QX11Info::display(), str.toLatin1(), true);
    if (sel == None)
	return false;
    return XGetSelectionOwner(QX11Info::display(), sel) != None;
}

QPixmap KSharedPixmap::pixmap() const
{
    return d->pixmap_data;
}

bool KSharedPixmap::loadFromShared(const QString & name, const QRect & rect)
{
    d->rect = rect;
    if (d->selection != None)
	// already active
	return false;

    d->pixmap_data = QPixmap(); // invalidate

    QString str = QString("KDESHPIXMAP:%1").arg(name);
    d->selection = XInternAtom(QX11Info::display(), str.toLatin1(), true);
    if (d->selection == None)
	return false;
    if (XGetSelectionOwner(QX11Info::display(), d->selection) == None)
    {
	d->selection = None;
	return false;
    }

    XConvertSelection(QX11Info::display(), d->selection, d->pixmap, d->target,
	    winId(), CurrentTime);
    return true;
}


bool KSharedPixmap::x11Event(XEvent *event)
{
    if (event->type != SelectionNotify)
	return false;

    XSelectionEvent *ev = &event->xselection;
    if (ev->selection != d->selection)
	return false;

    if ((ev->target != d->pixmap) || (ev->property == None))
    {
	kWarning(270) << k_funcinfo << "illegal selection notify event.\n";
	d->selection = None;
	emit done(false);
	return true;
    }

    // Read pixmap handle from ev->property

    int dummy, format;
    unsigned long nitems, ldummy;
    unsigned char *pixmap_id = 0;
    Atom type;

    XGetWindowProperty(QX11Info::display(), winId(), ev->property, 0, 1, false,
	    d->pixmap, &type, &format, &nitems, &ldummy,
	    &pixmap_id);

    if (nitems != 1 || !pixmap_id)
    {
	kWarning(270) << k_funcinfo << "could not read property, nitems = " << nitems << "\n";
	emit done(false);
	return true;
    }

    Window root;
    unsigned int width, height, udummy;
    void *drawable_id = (void *) pixmap_id;
    Drawable pixmap = *(Drawable*) drawable_id;

    XGetGeometry(QX11Info::display(), pixmap, &root, &dummy, &dummy, &width,
	    &height, &udummy, &udummy);

    QX11Info inf;

    if (d->rect.isEmpty())
    {
	d->pixmap_data = QPixmap(width, height);
        GC gc = XCreateGC(QX11Info::display(), pixmap, 0, NULL );
	XCopyArea(QX11Info::display(), pixmap, d->pixmap_data.handle(), gc,
		0, 0, width, height, 0, 0);

        XFree(pixmap_id);
        XFreeGC(QX11Info::display(), gc);
	XDeleteProperty(QX11Info::display(), winId(), ev->property);
	d->selection = None;
	emit done(true);
	return true;
    }

    // Do some more processing here: Generate a tile that can be used as a
    // background tile for the rectangle "rect".

    //Check for origin off screen
    QPoint origin(0, 0);
    if(  d->rect.topLeft().x() < 0 ||  d->rect.topLeft().y() < 0 ) {
        //Save the offset and relocate the rect corners
        QPoint tl = d->rect.topLeft();
        QPoint br = d->rect.bottomRight();
        if( tl.x() < 0 ) {
            origin.setX( abs( tl.x() ) );
            tl.setX( 0 );
        }
        if( tl.y() < 0 ) {
            origin.setY( abs( tl.y() ) );
            tl.setY( 0 );
        }
        QRect adjustedRect( tl, br );
        d->rect = adjustedRect;
    }

    unsigned w = d->rect.width(), h = d->rect.height();
    unsigned tw = qMin(width, w), th = qMin(height, h);
    unsigned xa = d->rect.x() % width, ya = d->rect.y() % height;
    unsigned t1w = qMin(width-xa,tw), t1h = qMin(height-ya,th);

    d->pixmap_data = QPixmap( tw+origin.x(), th+origin.y() );

    GC gc = XCreateGC(QX11Info::display(), pixmap, 0, NULL );
    XCopyArea(QX11Info::display(), pixmap, d->pixmap_data.handle(), gc,
            xa, ya, t1w+origin.x(), t1h+origin.y(), origin.x(), origin.y() );
    XCopyArea(QX11Info::display(), pixmap, d->pixmap_data.handle(), gc,
	    0, ya, tw-t1w, t1h, t1w, 0);
    XCopyArea(QX11Info::display(), pixmap, d->pixmap_data.handle(), gc,
	    xa, 0, t1w, th-t1h, 0, t1h);
    XCopyArea(QX11Info::display(), pixmap, d->pixmap_data.handle(), gc,
	    0, 0, tw-t1w, th-t1h, t1w, t1h);

    XFree(pixmap_id);
    XFreeGC(QX11Info::display(), gc);

    d->selection = None;
    XDeleteProperty(QX11Info::display(), winId(), ev->property);
    emit done(true);
    return true;
}


#else
KSharedPixmap::KSharedPixmap()
    : QWidget( 0 )
{
}

KSharedPixmap::~KSharedPixmap()
{
}

QPixmap KSharedPixmap::pixmap() const
{
    return QPixmap();
}

bool KSharedPixmap::isAvailable(const QString & name) const
{
    return false;
}

bool KSharedPixmap::loadFromShared(const QString & name, const QRect & rect)
{
    return false;
}
#endif

#include "ksharedpixmap.moc"
