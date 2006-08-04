#include <stdio.h>
#include <stdlib.h>

#include <QTextStream>

#include <kbookmarkimporter.h>
#include <kicon.h>
#include <kmimetype.h>
#include <kmenu.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
//#include <kbookmarkmenu.h>

#include "konsole_mnu.h"
#include "konsolebookmarkmenu.h"
#include "konsolebookmarkhandler.h"

#include <QFile>

#include <kactionmenu.h>
#include <klocale.h>


KonsoleBookmarkMenu::KonsoleBookmarkMenu( KBookmarkManager* mgr,
                     KonsoleBookmarkHandler * _owner, KMenu * _parentMenu,
                     KActionCollection *collec, bool _isRoot, bool _add,
                     const QString & parentAddress )
: KBookmarkMenu( mgr, _owner, _parentMenu, collec, _isRoot, _add,
                 parentAddress),
  m_kOwner(_owner)
{
    /*
     * First, we disconnect KBookmarkMenu::slotAboutToShow()
     * Then, we connect    KonsoleBookmarkMenu::slotAboutToShow().
     * They are named differently because the SLOT() macro thinks we want
     * KonsoleBookmarkMenu::KBookmarkMenu::slotAboutToShow()
     * Could this be solved if slotAboutToShow() is virtual in KBookmarMenu?
     */
    disconnect( _parentMenu, SIGNAL( aboutToShow() ), this,
                SLOT( slotAboutToShow() ) );
    connect( _parentMenu, SIGNAL( aboutToShow() ),
             SLOT( slotAboutToShow2() ) );
}

/*
 * Duplicate this exactly because KBookmarkMenu::slotBookmarkSelected can't
 * be overrided.  I would have preferred to NOT have to do this.
 *
 * Why did I do this?
 *   - when KBookmarkMenu::fillbBookmarkMenu() creates sub-KBookmarkMenus.
 *   - when ... adds KActions, it uses KBookmarkMenu::slotBookmarkSelected()
 *     instead of KonsoleBookmarkMenu::slotBookmarkSelected().
 */
void KonsoleBookmarkMenu::slotAboutToShow2()
{
  // Did the bookmarks change since the last time we showed them ?
  if ( m_bDirty )
  {
    m_bDirty = false;
    refill();
  }
}

void KonsoleBookmarkMenu::refill()
{
  //kDebug(1203) << "KBookmarkMenu::refill()" << endl;
  m_lstSubMenus.clear();

  foreach (KAction* action, m_actions)
  {
    m_parentMenu->removeAction(action);
  }

  m_parentMenu->clear();
  m_actions.clear();
  fillBookmarkMenu();
  m_parentMenu->adjustSize();
}

void KonsoleBookmarkMenu::fillBookmarkMenu()
{
  if ( m_bIsRoot )
  {
    if ( m_bAddBookmark )
      addAddBookmark();

    addEditBookmarks();

    if ( m_bAddBookmark )
      addNewFolder();

    if ( m_pManager->showNSBookmarks()
         && QFile::exists( KNSBookmarkImporter::netscapeBookmarksFile() ) )
    {
      m_parentMenu->addSeparator();

      KActionMenu * actionMenu = new KActionMenu( KIcon("netscape"),
                                                  i18n("Netscape Bookmarks"),
                                                  m_actionCollection, 0L );
      m_parentMenu->addAction( actionMenu );
      m_actions.append( actionMenu );
      KonsoleBookmarkMenu *subMenu = new KonsoleBookmarkMenu( m_pManager,
                                         m_kOwner, actionMenu->menu(),
                                         m_actionCollection, false,
					 m_bAddBookmark, QString() );
      m_lstSubMenus.append(subMenu);
      connect( actionMenu->menu(), SIGNAL(aboutToShow()), subMenu,
               SLOT(slotNSLoad()));
    }
  }

  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  bool separatorInserted = false;
  for ( KBookmark bm = parentBookmark.first(); !bm.isNull();
        bm = parentBookmark.next(bm) )
  {
    QString text = bm.text();
    text.replace( '&', "&&" );
    if ( !separatorInserted && m_bIsRoot) { // inserted before the first konq bookmark, to avoid the separator if no konq bookmark
      m_parentMenu->addSeparator();
      separatorInserted = true;
    }
    if ( !bm.isGroup() )
    {
      if ( bm.isSeparator() )
      {
        m_parentMenu->addSeparator();
      }
      else
      {
        // kDebug(1203) << "Creating URL bookmark menu item for " << bm.text() << endl;
        // create a normal URL item, with ID as a name
        KAction *action = new KAction(KIcon(bm.icon()),  text, m_actionCollection, bm.url().url() );
        connect(action, SIGNAL(triggered(bool) ), SLOT( slotBookmarkSelected() ));

        action->setToolTip(bm.url().prettyUrl());
	m_parentMenu->addAction( action );

        m_actions.append( action );
      }
    }
    else
    {
      // kDebug(1203) << "Creating bookmark submenu named " << bm.text() << endl;
      KActionMenu * actionMenu = new KActionMenu( KIcon(bm.icon()), text,
                                                  m_actionCollection, 0L );
      m_parentMenu->addAction( actionMenu );
      m_actions.append( actionMenu );
      KonsoleBookmarkMenu *subMenu = new KonsoleBookmarkMenu( m_pManager,
                                         m_kOwner, actionMenu->menu(),
                                         m_actionCollection, false,
                                         m_bAddBookmark, bm.address() );
      m_lstSubMenus.append( subMenu );
    }
  }

  if ( !m_bIsRoot && m_bAddBookmark )
  {
    m_parentMenu->addSeparator();
    addAddBookmark();
    addNewFolder();
  }
}

void KonsoleBookmarkMenu::slotBookmarkSelected()
{
    KAction * a;
    QString b;

    if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
    a = (KAction*)sender();
    b = a->text();
    m_kOwner->openBookmarkURL( sender()->objectName(), /* URL */
                               ( (KAction *)sender() )->text() /* Title */ );
}

void KonsoleBookmarkMenu::slotNSBookmarkSelected()
{
    KAction *a;
    QString b;

    QString link(sender()->objectName()+8);
    a = (KAction*)sender();
    b = a->text();
    m_kOwner->openBookmarkURL( link, /*URL */
                               ( (KAction *)sender() )->text()  /* Title */ );
}

#include "konsolebookmarkmenu.moc"
