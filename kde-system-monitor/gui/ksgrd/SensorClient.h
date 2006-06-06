/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

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

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#ifndef KSG_SENSORCLIENT_H
#define KSG_SENSORCLIENT_H

#include <QString>

namespace KSGRD {

/**
  Every object that should act as a client to a sensor must inherit from
  this class. A pointer to the client object is passed as SensorClient*
  to the SensorAgent. When the requested information is available or a
  problem occurred one of the member functions is called.
 */
class SensorClient
{
  public:
    SensorClient() { }
    virtual ~SensorClient() { }

    /**
      This function is called whenever the information form the sensor has
      been received by the sensor agent. This function must be reimplemented
      by the sensor client to receive and process this information.
     */
    virtual void answerReceived( int, const QStringList& ) { }

    /**
      In case of an unexpected fatal problem with the sensor the sensor
      agent will call this function to notify the client about it.
     */
    virtual void sensorLost( int ) { }
};

/**
  Every object that has a SensorClient as a child must inherit from
  this class to support the advanced update interval settings.
 */
class SensorBoard
{
  public:
    SensorBoard() { }
    virtual ~SensorBoard() { }

    void updateInterval( int interval ) { mUpdateInterval = interval; }

    int updateInterval() { return mUpdateInterval; }

  private:
    int mUpdateInterval;
};

/**
  The following classes are utility classes that provide a
  convenient way to retrieve pieces of information from the sensor
  answers. For each type of answer there is a separate class.
 */
class SensorTokenizer
{
  public:
    SensorTokenizer( const QString &info, QChar separator )
    {
      if ( separator == '/' ) {
        //This is a special case where we assume that info is a '\' escaped string

        int i=0;
        int lastTokenAt = -1;

        for( ; i < info.length(); ++i ) {
          if( info[i] == '\\' ) {
            ++i;
          }
          else if ( info[i] == separator ) {
            mTokens.append( unEscapeString( info.mid( lastTokenAt + 1, i - lastTokenAt - 1 ) ) );
            lastTokenAt = i;
          }
        }

        //Add everything after the last token
        mTokens.append( unEscapeString( info.mid( lastTokenAt + 1, i - lastTokenAt - 1 ) ) );
      }
      else {
        mTokens = info.split( separator );
      }
    }

    ~SensorTokenizer() { }

    const QString& operator[]( unsigned idx )
    {
      Q_ASSERT(idx < (unsigned)(mTokens.count()));
      return mTokens[ idx ];
    }

    uint count()
    {
      return mTokens.count();
    }

  private:
    QStringList mTokens;

    QString unEscapeString( const QString &string ) {

      int i=0;
      QString result = string;

      for( ; i < result.length(); ++i ) {
        if( result[i] == '\\' ) {
          result.remove( i, 1 );
          ++i;
        }
      }

      return result;
    }
};

/**
  An integer info contains 4 fields separated by TABS, a description
  (name), the minimum and the maximum values and the unit.
  e.g. Swap Memory	0	133885952	KB
 */
class SensorIntegerInfo : public SensorTokenizer
{
  public:
    SensorIntegerInfo( const QString &info )
      : SensorTokenizer( info, '\t' ) { }

    ~SensorIntegerInfo() { }

    const QString &name()
    {
      return (*this)[ 0 ];
    }

    long min()
    {
      return (*this)[ 1 ].toLong();
    }

    long max()
    {
      return (*this)[ 2 ].toLong();
    }

    const QString &unit()
    {
      return (*this)[ 3 ];
    }
};

/**
  An float info contains 4 fields separated by TABS, a description
  (name), the minimum and the maximum values and the unit.
  e.g. CPU Voltage 0.0	5.0	V
 */
class SensorFloatInfo : public SensorTokenizer
{
  public:
    SensorFloatInfo( const QString &info )
      : SensorTokenizer( info, '\t' ) { }

    ~SensorFloatInfo() { }

    const QString &name()
    {
      return (*this)[ 0 ];
    }

    double min()
    {
      return (*this)[ 1 ].toDouble();
    }

    double max()
    {
      return (*this)[ 2 ].toDouble();
    }

    const QString &unit()
    {
      return (*this)[ 3 ];
    }
};

/**
  A PS line consists of information about a process. Each piece of 
  information is separated by a TAB. The first 4 fields are process name,
  PID, PPID and real user ID. Those fields are mandatory.
 */
class SensorPSLine : public SensorTokenizer
{
  public:
    SensorPSLine( const QString &line )
      : SensorTokenizer( line, '\t' ) { }

    ~SensorPSLine() { }

    const QString& name()
    {
      return (*this)[ 0 ];
    }

    long pid()
    {
      return (*this)[ 1 ].toLong();
    }

    long ppid()
    {
      return (*this)[ 2 ].toLong();
    }

    long uid()
    {
      return (*this)[ 3 ].toLong();
    }
};

}

#endif
