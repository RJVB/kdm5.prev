/*****************************************************************

Copyright (c) 2000-2001 Matthias Ettrich <ettrich@kde.org>
              2000-2001 Matthias Elter   <elter@kde.org>
              2001      Carsten Pfeiffer <pfeiffer@kde.org>
              2001      Martijn Klingens <mklingens@yahoo.com>
              2004      Aaron J. Seigo   <aseigo@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <QCursor>
#include <QTimer>
#include <QList>
#include <QResizeEvent>
#include <QByteArray>

#include <QListWidget>
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <krun.h>
#include <kwm.h>
#include <kdialog.h>
#include <kactionselector.h>
#include <kiconloader.h>

#include "simplebutton.h"

#include "systemtrayapplet.h"
#include "systemtrayapplet.moc"

#include <X11/Xlib.h>
#include <QX11Info>

extern "C"
{
    KDE_EXPORT KPanelApplet* init(QWidget *parent, const QString& configFile)
    {
        KGlobal::locale()->insertCatalog("ksystemtrayapplet");
        return new SystemTrayApplet(configFile, Plasma::Normal,
                                    Plasma::Preferences, parent);
    }
}

SystemTrayApplet::SystemTrayApplet(const QString& configFile, Plasma::Type type, int actions,
                                   QWidget *parent)
  : KPanelApplet(configFile, type, actions, parent),
    m_showFrame(false),
    m_showHidden(false),
    m_expandButton(0),
    m_settingsDialog(0),
    m_iconSelector(0),
    m_autoRetractTimer(0),
    m_autoRetract(false)
{
    loadSettings();

    setBackgroundOrigin(AncestorOrigin);

    // kApplication notifies us of settings changes. added to support
    // disabling of frame effect on mouse hover
#ifdef __GNUC__
#warning "kde4: dcop port it"
#endif
    //kapp->dcopClient()->setNotifications(true);
    //connectDCOPSignal("kicker", "kicker", "configurationChanged()", "loadSettings()", false);

    QTimer::singleShot(0, this, SLOT(initialize()));
}

void SystemTrayApplet::initialize()
{
    bool existing = false;
#if 0
    // register existing tray windows
    const QList<WId> systemTrayWindows = kwin_module->systemTrayWindows();
    for (QList<WId>::ConstIterator it = systemTrayWindows.begin();
         it != systemTrayWindows.end(); ++it )
    {
        embedWindow(*it, true);
        existing = true;
    }
#endif

    showExpandButton(!m_hiddenWins.isEmpty());

    if (existing)
    {
        updateVisibleWins();
        layoutTray();
    }

#if 0
    // the KWinModule notifies us when tray windows are added or removed
    connect( kwin_module, SIGNAL( systemTrayWindowAdded(WId) ),
             this, SLOT( systemTrayWindowAdded(WId) ) );
    connect( kwin_module, SIGNAL( systemTrayWindowRemoved(WId) ),
             this, SLOT( updateTrayWindows() ) );
#endif

    QByteArray screenstr;
	QX11Info info;
    screenstr.setNum(info.screen());
    QByteArray trayatom = "_NET_SYSTEM_TRAY_S" + screenstr;

    Display *display = QX11Info::display();

    net_system_tray_selection = XInternAtom(display, trayatom, false);
    net_system_tray_opcode = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", false);

    // Acquire system tray
    XSetSelectionOwner(display,
                       net_system_tray_selection,
                       winId(),
                       CurrentTime);

    WId root = QX11Info::appRootWindow();

    if (XGetSelectionOwner (display, net_system_tray_selection) == winId())
    {
        XClientMessageEvent xev;

        xev.type = ClientMessage;
        xev.window = root;

        xev.message_type = XInternAtom (display, "MANAGER", False);
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = net_system_tray_selection;
        xev.data.l[2] = winId();
        xev.data.l[3] = 0;        /* manager specific data */
        xev.data.l[4] = 0;        /* manager specific data */

        XSendEvent (display, root, False, StructureNotifyMask, (XEvent *)&xev);
    }
}

SystemTrayApplet::~SystemTrayApplet()
{
    for (TrayEmbedList::const_iterator it = m_hiddenWins.constBegin();
         it != m_hiddenWins.constEnd();
         ++it)
    {
        delete *it;
    }

    for (TrayEmbedList::const_iterator it = m_shownWins.constBegin();
         it != m_shownWins.constEnd();
         ++it)
    {
        delete *it;
    }

    KGlobal::locale()->removeCatalog("ksystemtrayapplet");
}

bool SystemTrayApplet::x11Event( XEvent *e )
{
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
    if ( e->type == ClientMessage ) {
        if ( e->xclient.message_type == net_system_tray_opcode &&
             e->xclient.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
            if( isWinManaged( (WId)e->xclient.data.l[2] ) ) // we already manage it
                return true;
            embedWindow( e->xclient.data.l[2], false );
            updateVisibleWins();
            layoutTray();
            emit updateLayout();
            return true;
        }
    }
    return KPanelApplet::x11Event( e ) ;
}

void SystemTrayApplet::preferences()
{
    if (m_settingsDialog)
    {
        m_settingsDialog->show();
        m_settingsDialog->raise();
        return;
    }

    m_settingsDialog = new KDialog( 0 );
    m_settingsDialog->setObjectName( "systrayconfig" );
    m_settingsDialog->setCaption( i18n("Configure System Tray") );
    m_settingsDialog->setButtons( KDialog::Ok |KDialog::Apply| KDialog::Cancel );
    m_settingsDialog->showButtonSeparator( true );
    m_settingsDialog->resize(450, 400);

    connect(m_settingsDialog, SIGNAL(applyClicked()), this, SLOT(applySettings()));
    connect(m_settingsDialog, SIGNAL(okClicked()), this, SLOT(applySettings()));
    connect(m_settingsDialog, SIGNAL(finished()), this, SLOT(settingsDialogFinished()));

    m_iconSelector = new KActionSelector(m_settingsDialog);
    m_iconSelector->setAvailableLabel(i18n("Hidden icons:"));
    m_iconSelector->setSelectedLabel(i18n("Visible icons:"));
    m_settingsDialog->setMainWidget(m_iconSelector);

    QListWidget *hiddenListWidget = m_iconSelector->availableListWidget();
    QListWidget *shownListWidget = m_iconSelector->selectedListWidget();

    TrayEmbedList::const_iterator it = m_shownWins.begin();
    TrayEmbedList::const_iterator itEnd = m_shownWins.end();
    for (; it != itEnd; ++it)
    {
        QString name = KWM::windowInfo((*it)->clientWinId(), NET::WMName).name();
	QList<QListWidgetItem *> itemlist = shownListWidget->findItems(name, Qt::MatchExactly | Qt::MatchCaseSensitive);
        if(itemlist.isEmpty() )
        {
            new QListWidgetItem(QIcon(KWM::icon((*it)->clientWinId(), 22, 22, true)), name, shownListWidget, 0);
        }
    }

    it = m_hiddenWins.begin();
    itEnd = m_hiddenWins.end();
    for (; it != itEnd; ++it)
    {
        QString name = KWM::windowInfo((*it)->clientWinId(), NET::WMName).name();
	QList<QListWidgetItem *> itemlist = hiddenListWidget->findItems(name, Qt::MatchExactly | Qt::MatchCaseSensitive);
        if(itemlist.isEmpty())
        {
            new QListWidgetItem(QIcon(KWM::icon((*it)->clientWinId(), 22, 22, true)), name, hiddenListWidget, 0);
        }
    }

    m_settingsDialog->show();
}

void SystemTrayApplet::settingsDialogFinished()
{
    m_settingsDialog->delayedDestruct();
    m_settingsDialog = 0;
    m_iconSelector = 0;
}

void SystemTrayApplet::applySettings()
{
    if (!m_iconSelector)
    {
        return;
    }

    // Save the sort order and hidden status using the window class (WM_CLASS) rather
    // than window name (caption) - window name is i18n-ed, so it's for example
    // not possible to create default settings.
    // For backwards compatibility, name is kept as it is, class is preceded by '!'.
    QMap< QString, QString > windowNameToClass;
    for( TrayEmbedList::ConstIterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it ) {
        KWindowInfo info = KWM::windowInfo( (*it)->clientWinId(), NET::WMName, NET::WM2WindowClass);
        windowNameToClass[ info.name() ] = '!' + info.windowClassClass();
    }
    for( TrayEmbedList::ConstIterator it = m_hiddenWins.begin();
         it != m_hiddenWins.end();
         ++it ) {
        KWindowInfo info = KWM::windowInfo( (*it)->clientWinId(), NET::WMName, NET::WM2WindowClass);
        windowNameToClass[ info.name() ] = '!' + info.windowClassClass();
    }

    KConfigGroup group = config()->group("SortedTrayIcons");
    m_sortOrderIconList.clear();
    QList<QListWidgetItem*> list = m_iconSelector->availableListWidget()->findItems(QString("*"), Qt::MatchRegExp);
    foreach (QListWidgetItem* item, list)
    {
        if( windowNameToClass.contains(item->text()))
            m_sortOrderIconList.append(windowNameToClass[item->text()]);
        else
            m_sortOrderIconList.append(item->text());
    }
    group.writeEntry("SortOrder", m_sortOrderIconList);

    group = config()->group("HiddenTrayIcons");
    m_hiddenIconList.clear();
    list = m_iconSelector->availableListWidget()->findItems(QString("*"), Qt::MatchRegExp);
    foreach (QListWidgetItem* item, list)
    {
        if( windowNameToClass.contains(item->text()))
            m_hiddenIconList.append(windowNameToClass[item->text()]);
        else
            m_hiddenIconList.append(item->text());
    }

    group.writeEntry("Hidden", m_hiddenIconList);
    group.sync();

    TrayEmbedList::iterator it = m_shownWins.begin();
    while (it != m_shownWins.end())
    {
        if (shouldHide((*it)->clientWinId()))
        {
            m_hiddenWins.append(*it);
            it = m_shownWins.erase(it);
        }
        else
        {
            ++it;
        }
    }

    it = m_hiddenWins.begin();
    while (it != m_hiddenWins.end())
    {
        if (!shouldHide((*it)->clientWinId()))
        {
            m_shownWins.append(*it);
            it = m_hiddenWins.erase(it);
        }
        else
        {
            ++it;
        }
    }

    showExpandButton(!m_hiddenWins.isEmpty());

    updateVisibleWins();
    layoutTray();
    emit updateLayout();
}

void SystemTrayApplet::checkAutoRetract()
{
    if (!m_autoRetractTimer)
    {
        return;
    }

    if (!geometry().contains(mapFromGlobal(QCursor::pos())))
    {
        m_autoRetractTimer->stop();
        if (m_autoRetract)
        {
            m_autoRetract = false;

            if (m_showHidden)
            {
                retract();
            }
        }
        else
        {
            m_autoRetract = true;
            m_autoRetractTimer->start(2000);
        }

    }
    else
    {
        m_autoRetract = false;
        m_autoRetractTimer->start(250);
    }
}

void SystemTrayApplet::showExpandButton(bool show)
{
    if (show)
    {
        if (!m_expandButton)
        {
            m_expandButton = new SimpleButton(this);
            m_expandButton->setObjectName("expandButton");
            refreshExpandButton();

            if (orientation() == Qt::Vertical)
            {
                m_expandButton->setFixedSize(width() - 4,
                                             m_expandButton->sizeHint()
                                                            .height());
            }
            else
            {
                m_expandButton->setFixedSize(m_expandButton->sizeHint()
                                                            .width(),
                                             height() - 4);
            }
            connect(m_expandButton, SIGNAL(clicked()),
                    this, SLOT(toggleExpanded()));

            m_autoRetractTimer = new QTimer(this);
            m_autoRetractTimer->setSingleShot(true);
            connect(m_autoRetractTimer, SIGNAL(timeout()),
                    this, SLOT(checkAutoRetract()));
        }
        else
        {
            refreshExpandButton();
        }

        m_expandButton->show();
    }
    else if (m_expandButton)
    {
        m_expandButton->hide();
    }
}

void SystemTrayApplet::orientationChange( Qt::Orientation /*orientation*/ )
{
    refreshExpandButton();
}

void SystemTrayApplet::loadSettings()
{
    // set our defaults
    setFrameStyle(NoFrame);
    m_showFrame = false;

    KConfigGroup conf(config(), "General");

    if (conf.readEntry("ShowPanelFrame", false))
    {
        setFrameStyle(Panel | Sunken);
    }

    conf.changeGroup("HiddenTrayIcons");
    m_hiddenIconList = conf.readEntry("Hidden", QStringList() );
    conf.changeGroup("SortedTrayIcons");
    m_sortOrderIconList = conf.readEntry("SortOrder", QStringList());
}

void SystemTrayApplet::systemTrayWindowAdded( WId w )
{
    if (isWinManaged(w))
    {
        // we already manage it
        return;
    }

    embedWindow(w, true);
    updateVisibleWins();
    layoutTray();
    emit updateLayout();

    if (m_showFrame && frameStyle() == NoFrame)
    {
        setFrameStyle(Panel|Sunken);
    }
}

void SystemTrayApplet::embedWindow( WId w, bool kde_tray )
{
    TrayEmbed* emb = new TrayEmbed(kde_tray, this);
    //emb->setAutoDelete(false);
    emb->setBackgroundOrigin(AncestorOrigin);
    emb->setBackgroundRole( QPalette::NoRole );
    emb->setForegroundRole( QPalette::NoRole );

    if (kde_tray)
    {
        static Atom hack_atom = XInternAtom( QX11Info::display(), "_KDE_SYSTEM_TRAY_EMBEDDING", False );
        XChangeProperty( QX11Info::display(), w, hack_atom, hack_atom, 32, PropModeReplace, NULL, 0 );
        emb->embedClient(w);
        XDeleteProperty( QX11Info::display(), w, hack_atom );
    }
    else
    {
        emb->embedClient(w);
    }
    if (emb->clientWinId() == 0)  // error embedding
    {
        delete emb;
        return;
    }

    connect(emb, SIGNAL(clientClosed()), SLOT(updateTrayWindows()));
    emb->resize(24, 24);
    if (shouldHide(w))
    {
        emb->hide();
        m_hiddenWins.append(emb);
        showExpandButton(true);
    }
    else
    {
        emb->show();
        m_shownWins.append(emb);
    }
}

bool SystemTrayApplet::isWinManaged(WId w)
{
    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
    {
        if ((*emb)->clientWinId() == w) // we already manage it
        {
            return true;
        }
    }

    lastEmb = m_hiddenWins.end();
    for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
    {
        if ((*emb)->clientWinId() == w) // we already manage it
        {
            return true;
        }
    }

    return false;
}

bool SystemTrayApplet::shouldHide(WId w)
{
    return m_hiddenIconList.contains(KWM::windowInfo(w,NET::WMName).name());
    return m_hiddenIconList.contains(KWM::windowInfo(w,NET::WMName).name())
        || m_hiddenIconList.contains('!'+KWM::windowInfo(w,0,NET::WM2WindowClass).windowClassClass());
}

void SystemTrayApplet::updateVisibleWins()
{
    TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
    TrayEmbedList::const_iterator emb = m_hiddenWins.begin();

    if (m_showHidden)
    {
        for (; emb != lastEmb; ++emb)
        {
            (*emb)->show();
        }
    }
    else
    {
        for (; emb != lastEmb; ++emb)
        {
            (*emb)->hide();
        }
    }

    QMap< TrayEmbed*, QString > names; // cache names and classes
    QMap< TrayEmbed*, QString > classes;
    for( TrayEmbedList::const_iterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it ) {
        KWindowInfo info = KWM::windowInfo((*it)->clientWinId(),NET::WMName,NET::WM2WindowClass);
        names[ *it ] = info.name();
        classes[ *it ] = '!'+info.windowClassClass();
    }
    TrayEmbedList newList;
    for( QStringList::const_iterator it1 = m_sortOrderIconList.begin();
         it1 != m_sortOrderIconList.end();
         ++it1 ) {
        for( TrayEmbedList::iterator it2 = m_shownWins.begin();
             it2 != m_shownWins.end();
             ) {
            if( (*it1).startsWith("!") ? classes[ *it2 ] == *it1 : names[ *it2 ] == *it1 ) {
                newList.append( *it2 ); // don't bail out, there may be multiple ones
                it2 = m_shownWins.erase( it2 );
            } else
                ++it2;
        }
    }
    for( TrayEmbedList::const_iterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it )
        newList.append( *it ); // append unsorted items
    m_shownWins = newList;
}

void SystemTrayApplet::toggleExpanded()
{
    if (m_showHidden)
    {
        retract();
    }
    else
    {
        expand();
    }
}

void SystemTrayApplet::refreshExpandButton()
{
    if (!m_expandButton)
    {
        return;
    }

    Qt::Orientation o = orientation();
    m_expandButton->setOrientation(o);

    if (orientation() == Qt::Vertical)
    {
        m_expandButton->setPixmap(m_showHidden ?
            KIconLoader::global()->loadIcon("arrow-down", K3Icon::Panel, 16) :
            KIconLoader::global()->loadIcon("arrow-up", K3Icon::Panel, 16));
    }
    else
    {
        m_expandButton->setPixmap((m_showHidden ^ kapp->layoutDirection() == Qt::RightToLeft) ?
            KIconLoader::global()->loadIcon("arrow-right", K3Icon::Panel, 16) :
            KIconLoader::global()->loadIcon("arrow-left", K3Icon::Panel, 16));
    }
}

void SystemTrayApplet::expand()
{
    m_showHidden = true;
    refreshExpandButton();

    updateVisibleWins();
    layoutTray();
    emit updateLayout();

    if (m_autoRetractTimer)
    {
        m_autoRetractTimer->start(250);
    }
}

void SystemTrayApplet::retract()
{
    if (m_autoRetractTimer)
    {
        m_autoRetractTimer->stop();
    }

    m_showHidden = false;
    refreshExpandButton();

    updateVisibleWins();
    layoutTray();
    emit updateLayout();
}

void SystemTrayApplet::updateTrayWindows()
{
    TrayEmbedList::iterator emb = m_shownWins.begin();
    while (emb != m_shownWins.end())
    {
        WId wid = (*emb)->clientWinId();
        if ((wid == 0) ||
            ((*emb)->kdeTray())) // && !kwin_module->systemTrayWindows().contains(wid)))
        {
            (*emb)->deleteLater();
            emb = m_shownWins.erase(emb);
        }
        else
        {
            ++emb;
        }
    }

    emb = m_hiddenWins.begin();
    while (emb != m_hiddenWins.end())
    {
        WId wid = (*emb)->clientWinId();
        if ((wid == 0) ||
            ((*emb)->kdeTray())) // && !kwin_module->systemTrayWindows().contains(wid)))
        {
            (*emb)->deleteLater();
            emb = m_hiddenWins.erase(emb);
        }
        else
        {
            ++emb;
        }
    }

    showExpandButton(!m_hiddenWins.isEmpty());

    updateVisibleWins();
    layoutTray();
    emit updateLayout();
}

int SystemTrayApplet::maxIconWidth() const
{
    int largest = 24;

    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
    {
        if (*emb == 0)
        {
            continue;
        }

        int width = (*emb)->sizeHint().width();
        if (width > largest)
        {
            largest = width;
        }
    }

    if (m_showHidden)
    {
        lastEmb = m_hiddenWins.end();
        for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
        {
            int width = (*emb)->sizeHint().width();
            if (width > largest)
            {
                largest = width;
            }
        }
    }

    return largest;
}

int SystemTrayApplet::maxIconHeight() const
{
    int largest = 24;

    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != m_shownWins.end(); ++emb)
    {
        if (*emb == 0)
        {
            continue;
        }

        int height = (*emb)->sizeHint().height();
        if (height > largest)
        {
            largest = height;
        }
    }

    if (m_showHidden)
    {
        lastEmb = m_hiddenWins.end();
        for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != m_hiddenWins.end(); ++emb)
        {
            if (*emb == 0)
            {
                continue;
            }

            int height = (*emb)->sizeHint().height();
            if (height > largest)
            {
                largest = height;
            }
        }
    }

    return largest;
}

int SystemTrayApplet::widthForHeight(int h) const
{
    int iconWidth = maxIconWidth(), iconHeight = maxIconHeight();
    int iconCount = m_shownWins.count();

    if (m_showHidden)
    {
        iconCount += m_hiddenWins.count();
    }

    if (h < iconHeight)
    {
        // avoid div by 0 later
        h = iconHeight;
    }

    int ret = 0;

    if (iconCount > 0)
    {
        ret = (((iconCount - 1) / (h / iconHeight)) + 1) * iconWidth + 4;

        if (ret < iconWidth + 4)
        {
            ret = 0;
        }
    }

    if (m_expandButton && m_expandButton->isVisibleTo(const_cast<SystemTrayApplet*>(this)))
    {
        ret += m_expandButton->width() + 2;
    }

    return ret;
}

int SystemTrayApplet::heightForWidth(int w) const
{
    int iconWidth = maxIconWidth(), iconHeight = maxIconHeight();
    int iconCount = m_shownWins.count();

    if (m_showHidden)
    {
        iconCount += m_hiddenWins.count();
    }

    if (w < iconWidth)
    {
        // avoid div by 0 later
        w = iconWidth;
    }
    int ret = (((iconCount - 1) / (w / iconWidth)) + 1) * iconHeight + 4;

    if (ret < iconHeight + 4)
    {
        ret = 0;
    }

    if (m_expandButton && m_expandButton->isVisibleTo(const_cast<SystemTrayApplet*>(this)))
    {
        ret += m_expandButton->height() + 2;
    }

    return ret;
}

void SystemTrayApplet::resizeEvent( QResizeEvent* )
{
    if (m_expandButton)
    {
        if (orientation() == Qt::Vertical)
        {
            m_expandButton->setFixedSize(width() - 4,
                                         m_expandButton->sizeHint().height());
        }
        else
        {
            m_expandButton->setFixedSize(m_expandButton->sizeHint().width(),
                                         height() - 4);
        }
    }

    layoutTray();
}

void SystemTrayApplet::layoutTray()
{
    int iconCount = m_shownWins.count();

    if (m_showHidden)
    {
        iconCount += m_hiddenWins.count();
    }

    if (iconCount == 0)
    {
        updateGeometry();
        return;
    }

    /* heightWidth = height or width in pixels (depends on orientation())
     * nbrOfLines = number of rows or cols (depends on orientation())
     * spacing = size of spacing in pixels between lines (rows or cols)
     * line = what line to draw an icon in */
    int i = 0, line, spacing, nbrOfLines, heightWidth;
    int iconWidth = maxIconWidth(), iconHeight = maxIconHeight();

    // col = column or row, depends on orientation(),
    // the opposite direction of line
    int col = 0;

    if (m_expandButton && m_expandButton->isVisibleTo(this) && !kapp->layoutDirection() == Qt::RightToLeft)
    {
        m_expandButton->move(2, 2);
        if (orientation() == Qt::Vertical)
            col += m_expandButton->height() + 2;
        else
            col += m_expandButton->width() + 2;
    }

    if (orientation() == Qt::Vertical)
    {
        heightWidth = width();
        // to avoid nbrOfLines=0 we ensure heightWidth >= iconWidth!
        heightWidth = heightWidth < iconWidth ? iconWidth : heightWidth;
        nbrOfLines = heightWidth / iconWidth;
        spacing = (heightWidth - iconWidth*nbrOfLines) / (nbrOfLines + 1);

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
            {
                line = i % nbrOfLines;
                (*emb)->move(spacing*(line+1) + line*iconWidth, 2 + col);
                if (line + 1 == nbrOfLines)
                {
                    col += iconHeight;
                }
                i++;
            }
        }

        TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
        for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
        {
            line = i % nbrOfLines;
            (*emb)->move(spacing*(line+1) + line*iconWidth, 2 + col);
            if (line + 1 == nbrOfLines)
            {
                col += iconHeight;
            }
            i++;
        }
    }
    else
    {
        heightWidth = height();
        heightWidth = heightWidth < iconHeight ? iconHeight : heightWidth; // to avoid nbrOfLines=0
        nbrOfLines = heightWidth / iconHeight;
        spacing = (heightWidth - iconHeight*nbrOfLines) / (nbrOfLines + 1);

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
            {
                line = i % nbrOfLines;
                (*emb)->move(2 + col, spacing*(line+1) + line*iconHeight);
                if (line + 1 == nbrOfLines)
                {
                    col += iconWidth;
                }
                i++;
            }
        }

        TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
        for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
        {
            line = i % nbrOfLines;
            (*emb)->move(2 + col, spacing*(line+1) + line*iconHeight);

            if (line + 1 == nbrOfLines)
            {
                col += iconWidth;
            }

            i++;
        }
    }

    if (m_expandButton && m_expandButton->isVisibleTo(this) && kapp->layoutDirection() == Qt::RightToLeft)
    {
        m_expandButton->move(width() - m_expandButton->width() - 2, 2);
        col++;
    }

    updateGeometry();
}

TrayEmbed::TrayEmbed( bool kdeTray, QWidget* parent )
    : QX11EmbedContainer( parent ), kde_tray( kdeTray )
{
//    if( kde_tray ) // after QXEmbed reparents windows to the root window as unmapped.
//        setMapAfterRelease( true ); // systray one will have to be made visible somehow
}
