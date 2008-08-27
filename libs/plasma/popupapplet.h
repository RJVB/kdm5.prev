/***************************************************************************
 *   Copyright (C) 2008 by Montel Laurent <montel@kde.org>                 *
 *   Copyright (C) 2008 by Alessandro Diaferia <alediaferia@gmail.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#ifndef POPUPAPPLET_H
#define POPUPAPPLET_H

#include <plasma/applet.h>
#include <plasma/plasma_export.h>

class QGraphicsProxyWidget;
class QGraphicsLinearLayout;

namespace Plasma
{

class Dialog;
class Icon;
class PopupAppletPrivate;

/**
 * Allows applets to automatically 'collapse' into an icon when put in an panel, and is a convenient
 * base class for any applet that wishes to use extenders.
 * Applets that subclass this class should implement either widget() or graphicsWidget() to return a
 * widget that will be displayed in the applet if the applet is in a Planar or MediaCenter form
 * factor. If the applet is put in a panel, an icon will be displayed instead, which shows the
 * widget in a popup when clicked.
 * If you use this class as a base class for your extender using applet, you should implement
 * graphicsWidget() to return extender(), and connect the extenders geometryChanged() signal to the
 * widgetGeometryChanged() slot.
 */

class PLASMA_EXPORT PopupApplet : public Plasma::Applet
{
    Q_OBJECT
public:
    PopupApplet(QObject *parent, const QVariantList &args);
    ~PopupApplet();

    /**
     * @arg icon the icon that has to be displayed when the applet is in a panel.
     */
    void setIcon(const QIcon &icon);

    /**
     * @arg icon the icon that has to be displayed when the applet is in a panel.
     */
    void setIcon(const QString &iconName);

    /**
     * @return the icon that is displayed when the applet is in a panel.
     */
    QIcon icon() const;

    /**
     * Implement either this function or graphicsWidget.
     * @return the widget that will get shown in either a layout in the applet, or in a Dialog,
     * depending on the form factor of the applet.
     */
    virtual QWidget *widget();

    /**
     * Implement either this function or widget.
     * @return the widget that will get shown in either a layout in the applet, or in a Dialog,
     * depending on the form factor of the applet.
     */
    virtual QGraphicsWidget *graphicsWidget();

    /**
     * Shows the dialog showing the widget if the applet is in a panel.
     * @arg displayTime the time in ms that the popup should be displayed, defaults to 0 which means
     * always (until the user closes it again, that is).
     */
    void showPopup(uint displayTime = 0);

    /**
     * Shows the applet always as an icon, even when put on the desktop.
     */
    void setAlwaysIconified(bool set);
 
    /**
     * @return true if the PopupApplet is set to be always shown as icon.
     */
    bool alwaysIconified();

public Q_SLOTS:
    /**
     * Hides the popup.
     */
    void hidePopup();

    /**
     * Notify PopupApplet that the geometry of the graphicsWidget has changed. When you use an
     * extender you'll want to connect the extender's geometryChanged() signal to this slot.
     */
    void widgetGeometryChanged();

protected:
    void constraintsEvent(Plasma::Constraints constraints);

private:
    Q_PRIVATE_SLOT(d, void togglePopup())
    Q_PRIVATE_SLOT(d, void hideTimedPopup())
    PopupAppletPrivate * const d;
};

} // Plasma namespace

#endif /* POPUPAPPLET_H */

