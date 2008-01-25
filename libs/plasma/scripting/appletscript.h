/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_APPLETSCRIPT_H
#define PLASMA_APPLETSCRIPT_H

#include <plasma/plasma_export.h>
#include <plasma/scripting/scriptengine.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSizeF>

class QPainter;
class QStyleOptionGraphicsItem;

namespace Plasma
{

class PLASMA_EXPORT AppletScript : public ScriptEngine
{
    Q_OBJECT

public:
    /**
     * Default constructor for an AppletScript.
     * Subclasses should not attempt to access the Plasma::Applet
     * associated with this AppletScript in the constructor. All
     * such set up that requires the Applet itself should be done
     * in the init() method.
     */
    explicit AppletScript(QObject *parent = 0);
    ~AppletScript();

    /**
     * Sets the applet associated with this AppletScript
     */
    void setApplet(Plasma::Applet *applet);

    /**
     * Returns the Plasma::Applet associated with this script component
     */
    Plasma::Applet* applet() const;

    /**
     * Called when the script should paint the applet
     *
     * @param painter the QPainter to use
     * @param option the style option containing such flags as selection, level of detail, etc
     **/
    virtual void paintInterface(QPainter* painter,
                                const QStyleOptionGraphicsItem* option,
                                const QRect &contentsRect);

    Q_INVOKABLE QSizeF size() const;

protected:
    /**
     * @arg engine name of the engine
     * @return a data engine associated with this plasmoid
     */
    Q_INVOKABLE DataEngine* dataEngine(const QString &engine) const;

    /**
     * @return absolute path to the main script file for this plasmoid
     **/
    QString mainScript() const;

    /**
     * @return the Package associated with this plasmoid which can
     *         be used to request resources, such as images and
     *         interface files.
     */
    const Package* package() const;

private:
    class Private;
    Private * const d;
};

#define K_EXPORT_PLASMA_APPLETSCRIPTENGINE(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_appletscriptengine_" #libname))

} //Plasma namespace

#endif