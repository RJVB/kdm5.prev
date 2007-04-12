/* vi: ts=8 sts=4 sw=4
 *
 * This file is part of the KPager.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef KSHAREDPIXMAP_H
#define KSHAREDPIXMAP_H

#include <QtGui/QWidget>

class QPixmap;
class QString;

/**
 * Shared pixmap client.
 *
 * A shared pixmap is a pixmap that resides on the X server, is referenced
 * by a global id string, and can be accessed by all X clients.
 *
 * This class is a client class to shared pixmaps in KDE. You can use it
 * to copy (a part of) a shared pixmap into. KSharedPixmap provides a
 * member of type QPixmap (@see pixmap()) for that purpose.
 *
 * The server part of shared pixmaps is not implemented here. 
 * That part is provided by KPixmapServer, in the source file:
 * kdebase/kdesktop/pixmapserver.cc.
 *
 * An example: copy from a shared pixmap:
 * \code
 *   KSharedPixmap *pm = new KSharedPixmap;
 *   connect(pm, SIGNAL(done(bool)), SLOT(slotDone(bool)));
 *   pm->loadFromShared("My Pixmap");
 *
 *   // do something with the pixmap
 *   pm->pixmap() ...
 * \endcode
 *
 * @author Geert Jansen <jansen@kde.org>
 */
class KSharedPixmap : 
    public QWidget
{
    Q_OBJECT

public:

    /**
     * Construct an empty pixmap.
     */
    KSharedPixmap();

    /**
     * Destroys the pixmap.
     */
    ~KSharedPixmap();

    /**
     * Load from a shared pixmap reference. The signal done() is emitted
     * when the operation has finished.
     *
     * @param name The shared pixmap name.
     * @param rect If you pass a nonzero rectangle, a tile is generated which 
     * is able to fill up the specified rectangle completely. This is solely 
     * for optimization: in some cases the tile will be much smaller than the 
     * original pixmap. It reflects KSharedPixmap's original use: sharing
     * of the desktop background to achieve pseudo transparency.
     * @return True if the shared pixmap exists and loading has started
     * successfully, false otherwise.
     */
    bool loadFromShared(const QString & name, const QRect & rect=QRect());

    /**
     * Check whether a shared pixmap is available.
     *
     * @param name The shared pixmap name.
     * @return True if the shared pixmap is available, false otherwise.
     */
    bool isAvailable(const QString & name) const;

    /**
     * Gives access to the shared pixmap data.
     */
    QPixmap pixmap() const;

Q_SIGNALS:
    /** 
     * This signal is raised when a pixmap load operation has finished.
     *
     * @param success True if successful, false otherwise.
     */
    void done(bool success);

#ifdef Q_WS_X11
protected:
    bool x11Event(XEvent *);
#endif    

private:
    bool copy(const QString & id, const QRect & rect);
    void init();

    class KSharedPixmapPrivate;
    KSharedPixmapPrivate *d;
};

#endif

