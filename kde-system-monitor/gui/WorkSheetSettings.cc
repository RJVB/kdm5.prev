/*
    kde-system-monitor, the KDE System Monitor

    Copyright (c) 1999 - 2002 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <kacceleratormanager.h>
#include <klineedit.h>
#include <knuminput.h>
#include <klocale.h>

#include <q3groupbox.h>
#include <QLabel>
#include <QSpinBox>
#include <QLayout>
#include <QToolTip>

//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>

#include "WorkSheetSettings.h"

WorkSheetSettings::WorkSheetSettings( QWidget* parent, const char* name )
  : KDialogBase( parent, name, true, QString(), Ok|Cancel, Ok, true )
{
  setCaption( i18n( "Worksheet Properties" ) );

  QWidget *page = new QWidget( this );
  setMainWidget( page );

  QVBoxLayout *topLayout = new QVBoxLayout( page );
  topLayout->setMargin( 0 );
  topLayout->setSpacing( spacingHint() );

  Q3GroupBox *group = new Q3GroupBox( 0, Qt::Vertical, i18n( "Title" ), page );
  group->layout()->setMargin( marginHint() );
  group->layout()->setSpacing( spacingHint() );

  QGridLayout *groupLayout = new QGridLayout(  );
  group->layout()->addItem( groupLayout );
  groupLayout->setAlignment( Qt::AlignTop );

  mSheetTitle = new KLineEdit( group );
  groupLayout->addWidget( mSheetTitle, 0, 0 );

  topLayout->addWidget( group );

  group = new Q3GroupBox( 0, Qt::Vertical, i18n( "Properties" ), page );
  group->layout()->setMargin( marginHint() );
  group->layout()->setSpacing( spacingHint() );

  groupLayout = new QGridLayout();
  group->layout()->addItem( groupLayout );
  groupLayout->setAlignment( Qt::AlignTop );

  QLabel *label = new QLabel( i18n( "Rows:" ), group );
  groupLayout->addWidget( label, 0, 0 );

  mRows = new KIntNumInput( 1, group );
  mRows->setMaximum( 42 );
  mRows->setMinimum( 1 );
  groupLayout->addWidget( mRows, 0, 1 );
  label->setBuddy( mRows );

  label = new QLabel( i18n( "Columns:" ), group );
  groupLayout->addWidget( label, 1, 0 );

  mColumns = new KIntNumInput( 1, group );
  mColumns->setMaximum( 42 );
  mColumns->setMinimum( 1 );
  groupLayout->addWidget( mColumns, 1, 1 );
  label->setBuddy( mColumns );

  label = new QLabel( i18n( "Update interval:" ), group );
  groupLayout->addWidget( label, 2, 0 );

  mInterval = new KIntNumInput( 2, group );
  mInterval->setMaximum( 300 );
  mInterval->setMinimum( 1 );
  mInterval->setSuffix( i18n( " sec" ) );
  groupLayout->addWidget( mInterval, 2, 1 );
  label->setBuddy( mInterval );

  topLayout->addWidget( group );

  mRows->setWhatsThis( i18n( "Enter the number of rows the sheet should have." ) );
  mColumns->setWhatsThis( i18n( "Enter the number of columns the sheet should have." ) );
  mInterval->setWhatsThis( i18n( "All displays of the sheet are updated at the rate specified here." ) );
  mSheetTitle->setToolTip( i18n( "Enter the title of the worksheet here." ) );

  KAcceleratorManager::manage( page );

  mSheetTitle->setFocus();

  resize( QSize( 250, 230 ).expandedTo( minimumSizeHint() ) );
}

WorkSheetSettings::~WorkSheetSettings()
{
}

void WorkSheetSettings::setRows( int rows )
{
  mRows->setValue( rows );
}

int WorkSheetSettings::rows() const
{
  return mRows->value();
}

void WorkSheetSettings::setColumns( int columns )
{
  mColumns->setValue( columns );
}

int WorkSheetSettings::columns() const
{
  return mColumns->value();
}

void WorkSheetSettings::setInterval( int interval )
{
  mInterval->setValue( interval );
}

int WorkSheetSettings::interval() const
{
  return mInterval->value();
}

void WorkSheetSettings::setSheetTitle( const QString &title )
{
  mSheetTitle->setText( title );
}

QString WorkSheetSettings::sheetTitle() const
{
  return mSheetTitle->text();
}

#include "WorkSheetSettings.moc"
