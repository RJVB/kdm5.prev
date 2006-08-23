/*****************************************************************

Copyright (c) 2000 Bill Nagel

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

#include "quickbutton.h"
#include "quickaddappsmenu.h"

#include <QPainter>
#include <qdrawutil.h>
#include <QMenu>
#include <QToolTip>
#include <QPixmap>
#include <QPaintEvent>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <ktoggleaction.h>

#include <kickertip.h>
#include <klocale.h>
#include <kdesktopfile.h>
#include <krun.h>
#include <kiconeffect.h>
#include <kglobalsettings.h>
#include <kcursor.h>
#include <kapplication.h>
#include <kipc.h>
#include <kiconloader.h>
#include <k3urldrag.h>
#include <kstandarddirs.h>
#include <kworkspace.h>

#include <math.h>
#include <algorithm>

#ifdef DEBUG
   #define DEBUGSTR kDebug()
#else
   #define DEBUGSTR kndDebug()
#endif

QuickURL::QuickURL(const QString &u)
{  DEBUGSTR<<"QuickURL::QuickURL("<<u<<")"<<endl<<flush;
   KService::Ptr _service;
   _menuId = u;
   if (_menuId.startsWith("file:") && _menuId.endsWith(".desktop")) {
      // this ensures that desktop entries are referenced by desktop name instead of by file name
      _menuId=KUrl(_menuId).path();
   }
   if (_menuId.startsWith("/")) {
      // Absolute path
      _kurl.setPath(_menuId);

      if (_menuId.endsWith(".desktop")) {
         // Strip path
         QString s = _menuId;
         s = s.mid(s.lastIndexOf('/')+1);
         s = s.left(s.length()-8);
         _service = KService::serviceByStorageId(s);
         if (!_service) {
            _service = new KService(_menuId);
         } else {
         }
      }
   } else if (!KUrl::isRelativeUrl(_menuId)) {
      // Full URL
      _kurl = _menuId;
   } else {
      // menu-id
      _service = KService::serviceByMenuId(_menuId);
   }
   DEBUGSTR << "QuickURL: _service='"<<_service<<" _kurl="<<_kurl<<" _menuId="<<_menuId<<endl<<flush;

   if (_service) {
      if (!_service->isValid()) {
         DEBUGSTR << "QuickURL: _service is not valid"<<endl<<flush;
         // _service is a KShared pointer, don't try to delete it!
         _service = 0;
      } else {
         DEBUGSTR << "QuickURL: _service='"<<_service<<"' _service->desktopEntryPath()="<<_service->desktopEntryPath()<<endl<<flush;
         if (_kurl.path().length() == 0)
         {
            _kurl.setPath(KStandardDirs::locate("apps", _service->desktopEntryPath()));
         }
         if (!_service->menuId().isEmpty())
            _menuId = _service->menuId();

         m_genericName = _service->genericName();
         m_name = _service->name();
      }
   } else {
      m_name = _kurl.prettyUrl();
   }
   DEBUGSTR<<"QuickURL::QuickURL("<<u<<") END"<<endl<<flush;
}

void QuickURL::run() const
{  KWorkSpace::propagateSessionManager();   // is this needed?
   if (_service)
      KRun::run(*(_service), KUrl::List(), 0);
   else
      new KRun(_kurl, 0, 0, _kurl.isLocalFile());
}

//similar to MimeType::pixmapForURL
QPixmap QuickURL::pixmap( mode_t _mode, K3Icon::Group _group,
                          int _force_size, int _state, QString *) const
{  // Load icon

   //QPixmap pxmap = KMimeType::pixmapForURL(_kurl, _mode, _group, _force_size, _state);
   QString iconName = KMimeType::iconNameForURL(_kurl, _mode);
   QPixmap pxmap = KGlobal::iconLoader()->loadIcon(iconName, _group, _force_size, _state);
   // Resize to fit button
   pxmap = pxmap.scaled(_force_size, _force_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
   return pxmap;
}


QuickButton::QuickButton(const QString &u, KAction* configAction,
                         KActionCollection* actionCollection,
                         QWidget *parent) :
     QAbstractButton(parent),
     m_flashCounter(0),
     m_sticky(false)
{
    installEventFilter(KickerTip::self());
    if (parent && ! parent->parent())
    {
        setBackgroundRole( QPalette::NoRole );
        setForegroundRole( QPalette::NoRole );
    }
    setMouseTracking(true);
    _highlight = false;
    _oldCursor = cursor();
    _qurl=new QuickURL(u);

    this->setToolTip( _qurl->name());
    resize(int(DEFAULT_ICON_DIM),int(DEFAULT_ICON_DIM));

    QuickAddAppsMenu *addAppsMenu = new QuickAddAppsMenu(
        parent, this, _qurl->url());
    _popup = new QMenu(this);
    _popup->insertItem(i18n("Add Application"), addAppsMenu);
    _popup->addAction( configAction );
        _popup->addSeparator();
    _popup->insertItem(SmallIcon("remove"), i18n("Remove"),
            this, SLOT(removeApp()));

    m_stickyAction = new KToggleAction(i18n("Never Remove Automatically"),
        KShortcut(), actionCollection);
    connect(m_stickyAction, SIGNAL(toggled(bool)),
        this, SLOT(slotStickyToggled(bool)));
    m_stickyAction->plug(_popup, 2);
    m_stickyId = _popup->idAt(2);

    settingsChanged(KApplication::SETTINGS_MOUSE);
    connect(kapp, SIGNAL(settingsChanged(int)), SLOT(settingsChanged(int)));
    connect(kapp, SIGNAL(iconChanged(int)), SLOT(iconChanged(int)));
    connect(this, SIGNAL(clicked()), SLOT(launch()));
    connect(this, SIGNAL(removeApp(QuickButton *)), parent,
        SLOT(removeAppManually(QuickButton *)));
    kapp->addKipcEventMask(KIPC::SettingsChanged);
    kapp->addKipcEventMask(KIPC::IconChanged);
}

QuickButton::~QuickButton()
{
    delete _qurl;
}


QString QuickButton::url() const
{
    return _qurl->url();
}


QString QuickButton::menuId() const
{  return _qurl->menuId();}

void QuickButton::loadIcon()
{  // Set Icon Dimension from size
   _iconDim=std::min(size().width(),size().height())-2*ICON_MARGIN;
   // Load icons
   _icon = _qurl->pixmap(0, K3Icon::Panel, _iconDim, K3Icon::DefaultState);
   _iconh = _qurl->pixmap(0, K3Icon::Panel, _iconDim, K3Icon::ActiveState);
}

void QuickButton::resizeEvent(QResizeEvent *)
{
   loadIcon();
}

void QuickButton::paintEvent(QPaintEvent *)
{
   QPainter p(this);
   drawButton(&p);
}

void QuickButton::drawButton(QPainter *p)
{
   if (isDown() || isChecked()) {
      p->fillRect(rect(), palette().brush(QPalette::Mid));
      qDrawWinButton(p, 0, 0, width(), height(), palette(), true);
   }

   drawButtonLabel(p);
}

void QuickButton::drawButtonLabel(QPainter *p)
{
   QPixmap *pix = &_icon;    //PIX
   if (_highlight) pix = &_iconh;
   QPoint offset=QPoint((width()-_iconDim)/2,(height()-_iconDim)/2);
   int d = ICON_MARGIN;
   if (isDown() || isChecked()) d += 1;

   if (m_flashCounter%500<250) p->drawPixmap(offset.x()+ d, offset.y() + d, *pix);
}

void QuickButton::enterEvent(QEvent*)
{
   if (_changeCursorOverItem)
      setCursor(KCursor().handCursor());
   _highlight = true;
   repaint();
}

void QuickButton::leaveEvent(QEvent*)
{
   if (_changeCursorOverItem)
      setCursor(_oldCursor);
   setDragging(false);
}

void QuickButton::mousePressEvent(QMouseEvent *e)
{
   if (e->button() == Qt::RightButton)
      _popup->popup(e->globalPos());
   else if (e->button() == Qt::LeftButton) {
      _dragPos = e->pos();
      QAbstractButton::mousePressEvent(e);
   }
}

void QuickButton::mouseMoveEvent(QMouseEvent *e)
{
   if ((e->buttons() & Qt::LeftButton) == 0) return;
   QPoint p(e->pos() - _dragPos);
   if (p.manhattanLength() <= KGlobalSettings::dndEventDelay())
      return;
   DEBUGSTR<<"dragstart"<<endl<<flush;
   setDown(false);
   if (_dragEnabled) {
       KUrl::List uris;
       uris.append(_qurl->kurl());
       DEBUGSTR<<"creating KURLDrag"<<endl<<flush;
       K3URLDrag *dd = new K3URLDrag(uris,this);
       dd->setPixmap(_icon); //PIX
       DEBUGSTR<<"ready to drag"<<endl<<flush;
       grabKeyboard();
       dd->drag();
       releaseKeyboard();
   } else {
       setCursor(Qt::ForbiddenCursor);
   }
}

void QuickButton::settingsChanged(int category)
{
   if (category != KApplication::SETTINGS_MOUSE) return;

   _changeCursorOverItem = KGlobalSettings::changeCursorOverIcon();

   if (!_changeCursorOverItem)
      setCursor(_oldCursor);
}

void QuickButton::iconChanged(int)
{
   loadIcon();
   repaint();
}

void QuickButton::launch()
{
   setDown(false);
   repaint();
   KIconEffect::visualActivate(this, rect());
   _qurl->run();
   emit executed(_qurl->menuId());
}

void QuickButton::setDragging(bool enable)
{
   setDown(enable);
   _highlight=enable;
   repaint();
}

void QuickButton::setEnableDrag(bool enable)
{
   _dragEnabled=enable;
}

void QuickButton::removeApp()
{
   emit removeApp(this);
}

void QuickButton::flash()
{
   m_flashCounter = 2000;
   QTimer::singleShot(0, this, SLOT(slotFlash()));
}

void QuickButton::slotFlash()
{
    static const int timeout = 500/4;
    if (m_flashCounter > 0)
    {
        m_flashCounter -= timeout;
        if (m_flashCounter < 0) m_flashCounter = 0;
        repaint();
        QTimer::singleShot(timeout, this, SLOT(slotFlash()));
    }
}

void QuickButton::slotStickyToggled(bool isSticky)
{
    m_sticky = isSticky;
    emit stickyToggled(isSticky);
}

void QuickButton::setSticky(bool sticky)
{
    m_stickyAction->setChecked(sticky);
    slotStickyToggled(sticky);
}

void QuickButton::updateTipData(KickerTip::Data &data)
{
    if (!_qurl)
    {
        return;
    }
    data.message = _qurl->name();
    data.direction = m_popupDirection;
    data.subtext = _qurl->genericName();
    if (data.subtext == QString())
    {
        data.subtext = data.message;
    }
//    data.icon = KMimeType::pixmapForURL(_qurl->kurl(), 0,
//        K3Icon::Panel, K3Icon::SizeHuge, K3Icon::DefaultState);
    QString iconName = KMimeType::iconNameForURL(_qurl->kurl());
    data.icon = KGlobal::iconLoader()->loadIcon(iconName, K3Icon::Panel, K3Icon::SizeHuge, K3Icon::DefaultState);
}

void QuickButton::setPopupDirection(Plasma::Position d)
{
    m_popupDirection = d;
}

void QuickButton::setDynamicModeEnabled(bool enabled)
{
    _popup->setItemVisible(m_stickyId, enabled);
}


#include "quickbutton.moc"
