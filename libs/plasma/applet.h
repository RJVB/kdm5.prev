/*
 *   Copyright 2006-2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
 *   Copyright 2008 by Ménard Alexis <darktears31@gmail.com>

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

#ifndef PLASMA_APPLET_H
#define PLASMA_APPLET_H

#include <QtGui/QGraphicsItem>
#include <QtGui/QWidget>

#include <KDE/KPluginInfo>
#include <KDE/KGenericFactory>

#include <plasma/configxml.h>
#include <plasma/packagestructure.h>
#include <plasma/plasma.h>
#include <plasma/widgets/widget.h>

class KConfigDialog;

namespace Plasma
{

class Containment;
class DataEngine;
class Package;

/**
 * @short The base Applet class
 *
 * Applet provides several important roles for add-ons widgets in Plasma.
 *
 * First, it is the base class for the plugin system and therefore is the
 * interface to applets for host applications. It also handles the life time
 * management of data engines (e.g. all data engines accessed via
 * Applet::dataEngine(const QString&) are properly deref'd on Applet
 * destruction), background painting (allowing for consistent and complex
 * look and feel in just one line of code for applets), loading and starting
 * of scripting support for each applet, providing access to the associated
 * plasmoid package (if any) and access to configuration data.
 *
 * See techbase.kde.org for tutorial on writing Applets using this class.
 */
class PLASMA_EXPORT Applet : public Widget
{
    Q_OBJECT
//    Q_PROPERTY( QRectF maxSizeHint READ maxSizeHint )
    Q_PROPERTY(bool hasConfigurationInterface READ hasConfigurationInterface WRITE setHasConfigurationInterface)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString category READ category)
    Q_PROPERTY(bool immutable READ isImmutable WRITE setImmutable)
    Q_PROPERTY(bool drawStandardBackground READ drawStandardBackground WRITE setDrawStandardBackground)
    Q_PROPERTY(bool failedToLaunch READ failedToLaunch WRITE setFailedToLaunch)
    Q_PROPERTY(bool needsConfiguring READ needsConfiguring WRITE setNeedsConfiguring)
    Q_PROPERTY(QRectF boundingRect READ boundingRect)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(bool shouldConserveResources READ shouldConserveResources)
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)

    public:
        typedef QList<Applet*> List;
        typedef QHash<QString, Applet*> Dict;

        /**
         * @param parent the QGraphicsItem this applet is parented to
         * @param serviceId the name of the .desktop file containing the
         *      information about the widget
         * @param appletId a unique id used to differentiate between multiple
         *      instances of the same Applet type
         */
        explicit Applet(QGraphicsItem* parent = 0,
                        const QString& serviceId = QString(),
                        uint appletId = 0);

        /**
         * This constructor is to be used with the plugin loading systems
         * found in KPluginInfo and KService. The argument list is expected
         * to have two elements: the KService service ID for the desktop entry
         * and an applet ID which must be a base 10 number.
         *
         * @param parent a QObject parent; you probably want to pass in 0
         * @param args a list of strings containing two entries: the service id
         *      and the applet id
         */
        Applet(QObject* parent, const QVariantList& args);

        ~Applet();

        /**
         * @return a package structure representing a Theme
         */
        static PackageStructure::Ptr packageStructure();

        /**
         * This method is called once the applet is loaded and added to a Corona.
         * If the applet requires a QGraphicsScene or has an particularly intensive
         * set of initialization routines to go through, consider implementing it
         * in this method instead of the constructor.
         **/
        virtual void init();

        /**
         * @return the id of this applet
         */
        uint id() const;

        /**
        * Returns the KConfigGroup to access the applets configuration.
        *
        * This config object will write to an instance
        * specific config file named \<appletname\>\<instanceid\>rc
        * in the Plasma appdata directory.
        **/
        KConfigGroup config() const;

        /**
         * Returns a config group with the name provided. This ensures
         * that the group name is properly namespaced to avoid collision
         * with other applets that may be sharing this config file
         *
         * @param group the name of the group to access
         **/
        KConfigGroup config(const QString &group) const;

        /**
         * Saves state information about this applet.
         **/
        void save(KConfigGroup* group) const;

        /**
         * Restores state information about this applet.
         **/
        void restore(KConfigGroup* group);

        /**
         * Returns a KConfigGroup object to be shared by all applets of this
         * type.
         *
         * This config object will write to an applet-specific config object
         * named plasma_\<appletname\>rc in the local config directory.
         */
        KConfigGroup globalConfig() const;

        /**
         * Returns the config skeleton object from this applet's package,
         * if any.
         *
         * @return config skeleton object, or 0 if none
         **/
        ConfigXml* configXml() const;

        /**
         * Loads the given DataEngine
         *
         * Tries to load the data engine given by @p name.  Each engine is
         * only loaded once, and that instance is re-used on all subsequent
         * requests.
         *
         * If the data engine was not found, an invalid data engine is returned
         * (see DataEngine::isValid()).
         *
         * Note that you should <em>not</em> delete the returned engine.
         *
         * @param name Name of the data engine to load
         * @return pointer to the data engine if it was loaded,
         *         or an invalid data engine if the requested engine
         *         could not be loaded
         */
        Q_INVOKABLE DataEngine* dataEngine(const QString& name) const;

        /**
         * Accessor for the associated Package object if any.
         * Generally, only Plasmoids come in a Package.
         *
         * @return the Package object, or 0 if none
         **/
        const Package* package() const;

        /**
         * Called when any of the geometry constraints have been updated.
         * This method calls constraintsUpdated, which may be reimplemented,
         * once the Applet has been prepared for updating the constraints.
         *
         * @param constraints the type of constraints that were updated
         */
        void updateConstraints(Plasma::Constraints constraints = Plasma::AllConstraints);

        /**
         * Called when any of the geometry constraints have been updated.
         *
         * This is always called prior to painting and should be used as an
         * opportunity to layout the widget, calculate sizings, etc.
         *
         * Do not call update() from this method; an update() will be triggered
         * at the appropriate time for the applet.
         *
         * @param constraints the type of constraints that were updated
         * @property constraint
         */
        virtual void constraintsUpdated(Plasma::Constraints constraints);

        /**
         * Returns the current form factor the applet is being displayed in.
         *
         * @see Plasma::FormFactor
         */
        virtual FormFactor formFactor() const;

        /**
         * Returns the location of the scene which is displaying applet.
         *
         * @see Plasma::Location
         */
        virtual Location location() const;
     
	/**
         * @return the prefered aspect ratio mode for placement and resizing
         */
        Qt::AspectRatioMode aspectRatioMode() const;

        /**
         * Sets the prefered aspect ratio mode for placement and resizing
         */
        void setAspectRatioMode(Qt::AspectRatioMode);

        /**
         * @return whether or not to keep this applet square.
         */
        bool remainSquare() const;

        /**
         * Sets whether or not this applet should be kept square.
         *
         * @param square true if the applet should always be square in shape.
         */
        void setRemainSquare(bool square);

        /**
         * Returns a list of all known applets.
         *
         * @param category Only applets matchin this category will be returned.
         *                 Useful in conjunction with knownCategories.
         *                 If "Misc" is passed in, then applets without a
         *                 Categories= entry are also returned.
         *                 If an empty string is passed in, all applets are
         *                 returned.
         * @param parentApp the application to filter applets on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin info.
         *                  The default value of QString() will result in a
         *                  list containing only applets not specifically
         *                  registered to an application.
         * @return list of applets
         **/
        static KPluginInfo::List knownApplets(const QString &category = QString(),
                                              const QString &parentApp = QString());

        /**
         * Returns a list of all known applets associated with a certain mimetype.
         *
         * @return list of applets
         **/
        static KPluginInfo::List knownAppletsForMimetype(const QString &mimetype);

        /**
         * Returns a list of all the categories used by
         * installed applets.
         *
         * @param parentApp the application to filter applets on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin info.
         *                  The default value of QString() will result in a
         *                  list containing only applets not specifically
         *                  registered to an application.
         * @return list of categories
         * @param visibleOnly true if it should only return applets that are marked as visible
         */
        static QStringList knownCategories(const QString &parentApp = QString(), bool visibleOnly = true);

        /**
         * Attempts to load an applet
         *
         * Returns a pointer to the applet if successful.
         * The caller takes responsibility for the applet, including
         * deleting it when no longer needed.
         *
         * @param name the plugin name, as returned by KPluginInfo::pluginName()
         * @param appletId unique ID to assign the applet, or zero to have one
         *        assigned automatically.
         * @param args to send the applet extra arguments
         * @return a pointer to the loaded applet, or 0 on load failure
         **/
        static Applet* load(const QString &name, uint appletId = 0,
                            const QVariantList& args = QVariantList());

        /**
         * Attempts to load an applet
         *
         * Returns a pointer to the applet if successful.
         * The caller takes responsibility for the applet, including
         * deleting it when no longer needed.
         *
         * @param info KPluginInfo object for the desired applet
         * @param appletId unique ID to assign the applet, or zero to have one
         *        assigned automatically.
         * @param args to send the applet extra arguments
         * @return a pointer to the loaded applet, or 0 on load failure
         **/
        static Applet* load(const KPluginInfo& info, uint appletId = 0,
                                  const QVariantList& args = QVariantList());

        /**
         * Get the category of the given applet
         *
         * @param applet a KPluginInfo object for the applet
         */
        static QString category(const KPluginInfo& applet);

        /**
         * Get the category of the given applet
         *
         * @param appletName the name of the applet
         */
        static QString category(const QString& appletName);

        /**
         * This method is called when the interface should be painted.
         *
         * @param painter the QPainter to use to do the paintiner
         * @param option the style options object
         * @param contentsRect the rect to paint within; automatically adjusted for
         *                     the background, if any
         **/
        virtual void paintInterface(QPainter *painter,
                                    const QStyleOptionGraphicsItem *option,
                                    const QRect& contentsRect);

        /**
         * Returns the user-visible name for the applet, as specified in the
         * .desktop file.
         *
         * @return the user-visible name for the applet.
         **/
        QString name() const;

        /**
         * Returns the plugin name for the applet
         */
        QString pluginName() const;

        /**
         * Whether the applet should conserve resources. If true, try to avoid doing stuff which
         * is computationally heavy. Try to conserve power and resources.
         *
         * @return true if it should conserve resources, false if it does not.
         */
        bool shouldConserveResources() const;

        /**
         * Returns the icon related to this applet
         **/
        QString icon() const;

        /**
         * Returns the category the applet is in, as specified in the
         * .desktop file.
         */
        QString category() const;

        /**
         * Returns the color corresponding to the applet's category.
         */
        QColor color() const;

        /**
         * @return true if this applet is immutable
         **/
        bool isImmutable() const;

        /**
         * @return true if this applet is immutable due to Kiosk settings
         */
        bool isKioskImmutable() const;

        /**
         * @return returns whether or not the applet is using the standard
         *         background
         **/
        bool drawStandardBackground() const;

        /**
         * Sets whether the applet should automatically draw the standard
         * background.
         *
         * Defaults to true
         **/
        void setDrawStandardBackground(bool drawBackground);

	void paintWindowFrame ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);

        /**
         * If for some reason, the applet fails to get up on its feet (the
         * library couldn't be loaded, necessary hardware support wasn't found,
         * etc..) this method returns true
         **/
        bool failedToLaunch() const;

        /**
         * Call this method when the applet fails to launch properly. An
         * optional reason can be provided.
         *
         * Not that all children items will be deleted when this method is
         * called. If you have pointers to these items, you will need to
         * reset them after calling this method.
         *
         * @param failed true when the applet failed, false when it succeeded
         * @param reason an optional reason to show the user why the applet
         *               failed to launch
         **/
        void setFailedToLaunch(bool failed, const QString& reason = QString());

        /**
         * @return true if the applet currently needs to be configured,
         *         otherwise, false
         */
        bool needsConfiguring() const;

        /**
         * When the applet needs to be configured before being usable, this
         * method can be called to show a standard interface prompting the user
         * to configure the applet
         *
         * Not that all children items will be deleted when this method is
         * called. If you have pointers to these items, you will need to
         * reset them after calling this method.
         *
         * @param needsConfiguring true if the applet needs to be configured,
         *                         or false if it doesn't
         */
        void setNeedsConfiguring(bool needsConfiguring);

        /**
         * @return true if this plasmoid provides a GUI configuration
         **/
        bool hasConfigurationInterface();

        /**
         * Sets whether or not this applet provides a user interface for
         * configuring the applet.
         *
         * It defaults to false, and if true is passed in you should
         * also reimplement showConfigurationInterface()
         *
         * @param hasInterface whether or not there is a user interface available
         **/
        void setHasConfigurationInterface(bool hasInterface);

        /**
         * Returns a list of context-related QAction instances.
         *
         * This is used e.g. within the \a DesktopView to display a
         * contextmenu.
         *
         * @return A list of actions. The default implementation returns an
         *         empty list.
         **/
        virtual QList<QAction*> contextActions();

        /**
         * Sets shadow for the given applet.
         */
        void setShadowShown(bool);

        /**
         * Returns true if the given item has a shadow shown.
         */
        bool isShadowShown() const;

        /**
         * Sets whether or not this Applet is acting as a Containment
         */
        void setIsContainment(bool isContainment);

        /**
         * @return true if this Applet is currently being used as a Containment, false otherwise
         */
        bool isContainment() const;

	/**
         * Sets the geometry of this Plasma::Applet. Should not be used directly by
         * applet subclasses.
         * @param geometry the geometry to apply to this Plasma::Applet.
         */
        void setGeometry(const QRectF &geometry);
  
        /**
         * Causes this applet to raise above all other applets.
         */
        void raise();

        /**
         * Reimplemented from Plasma::Widget
         */
        Qt::Orientations expandingDirections() const;

        /**
         * Reimplemented from QGraphicsItem
         **/
        int type() const;
        enum { Type = Plasma::AppletType };

	/**
         * Reimplemented from QGraphicsItem
         */
        QPainterPath shape() const;

        /**
         * @internal
         */
        void resetConfigurationObject();

        /**
         * @return the Containment, if any, this applet belongs to
         **/
        Containment* containment() const;

    Q_SIGNALS:
        /**
         * Emitted when the applet needs to take (or lose) keyboard focus.
         *
         * An applet should emit this signal to ensure that autohiding
         * elements stay unhidden and other bits of bookkeeping are
         * performed to ensure proper function.
         *
         * If you call watchForFocus on your applet, then this is handled for
         * the applet and it is not necessary to emit the signal directly.
         *
         * @param focus true if the applet is taking keyboard focus, false if
         *        it is giving it up
         **/
        void requestFocus( bool focus );

        /**
         * Emitted whenever the applet makes a geometry change, so that views
         * can coordinate themselves with these changes if they desire.
         */
        void geometryChanged();

        /**
         * Emitted when an applet has changed values in its configuration
         * and wishes for them to be saved at the next save point. As this implies
         * disk activity, this signal should be used with care.
         *
         * @note This does not need to be emitted from saveState by individual
         * applets.
         */
        void configNeedsSaving();

    public Q_SLOTS:
        /**
         * Sets whether or not this applet is immutable or not.
         *
         * @param immutable true if this applet should not be changeable
         **/
        void setImmutable(bool immutable);

        /**
         * Destroys the applet; it will be deleted and configurations reset.
         */
        void destroy();

        /**
         * Lets the user interact with the plasmoid options.
         * Called when the user selects the configure entry
         * from the context menu.
         *
         * Applets should NOT reimplement this method unless there is good
         * reason to do so, but instead reimplment createConfigurationInterface
         */
        virtual void showConfigurationInterface();

        /**
         * Reimplement this method so provide a configuration interface,
         * parented to the supplied widget. Ownership of the widgets is passed
         * to the parent widget.
         *
         * @param parent the dialog which is the parent of the configuration
         *               widgets
         */
        virtual void createConfigurationInterface(KConfigDialog *parent);

        /**
         * Sends all pending contraints updates to the applet. Will usually
         * be called automatically, but can also be called manually if needed.
         */
        void flushUpdatedConstraints();

        /**
         * This method should be invoked whenever an application launch or other window
         * creation of focussing event was triggered
         */
        void launchActivated();

        /**
         * called when the Plasma::Theme has changed
         */
        void themeChanged();

    protected:
        /**
         * Called when a request to save the state of the applet is made
         * during runtime
         **/
        virtual void saveState(KConfigGroup* config) const;

        /**
         * Returns the name of the applet.
         *
         * This will be the same for all instances of this applet.
         **/
        QString globalName() const;

        /**
         * Returns a name unique to the instance of this applet.
         *
         * Useful for being able to refer directly to a particular
         * applet. Combines the global name with the applet id
         **/
        QString instanceName() const;

        /**
        * Register widgets that can receive keyboard focus.
        *
        * Calling this results in an eventFilter being placed on the widget.
        *
        * @param widget the widget to watch for keyboard focus
        * @param watch whether to start watching the widget, or to stop doing so
        */
        void watchForFocus(QObject *widget, bool watch = true);

        /**
         * Register the widgets that manages mouse clicks but you still want
         * to be able to drag the applet around when holding the mouse pointer
         * on that widgets.
         *
         * Calling this results in an eventFilter being places on the widget.
         *
         * @param widget the widget to watch for mouse move
         * @param watch whether to start watching the widget, or to stop doing so
         */
        void watchForMouseMove(QGraphicsItem *widget, bool watch);

        /**
        * Call this whenever focus is needed or not needed.
        *
        * You do not have to call this method for widgets that have
        * been registered with watchForFocus
        *
        * @see watchForFocus
        * @param focus whether to or not to request focus
        */
        void needsFocus(bool focus);

        /**
         * @internal event filter; used for focus watching
         **/
        bool eventFilter( QObject *o, QEvent *e );

        /**
         * @internal scene event filter; used to manage applet dragging
         */
        bool sceneEventFilter ( QGraphicsItem * watched, QEvent * event );

        /**
         * @internal manage the mouse movement to drag the applet around
         */
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

        /**
         * Reimpmlemented from LayoutItem
         */
        QPointF topLeft() const;

        /**
         * Reimplemented from QGraphicsItem
         */
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    protected Q_SLOTS:
        /**
         * @internal used to check the immutability of the item in the config file
         */
        void checkImmutability();

    private:
        Q_DISABLE_COPY(Applet)

        /**
         * Reimplemented from QGraphicsItem
         **/
        void paintWidget(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

        class Private;
        Private* const d;
};

} // Plasma namespace

/**
 * Register an applet when it is contained in a loadable module
 */
#define K_EXPORT_PLASMA_APPLET(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_applet_" #libname))

#endif // multiple inclusion guard
