/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef SCREENLOCKER_KSLDAPP_H
#define SCREENLOCKER_KSLDAPP_H

#include <KDE/KUniqueApplication>

#include <QtCore/QProcess>

// forward declarations
class KActionCollection;

namespace ScreenLocker
{

class LockWindow;

class KSldApp : public KUniqueApplication
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ksld.App")

public:
    static KSldApp* self();
    ~KSldApp();

    // The action collection of the active widget
    KActionCollection* actionCollection();

public Q_SLOTS:
    Q_SCRIPTABLE void lock();
     void lockProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private Q_SLOTS:
    void cleanUp();
    void releaseGrab();

private:
    KSldApp();
    void initialize();
    bool establishGrab();
    bool grabKeyboard();
    bool grabMouse();
    void startLockProcess();
    void showLockWindow();
    void hideLockWindow();

    KActionCollection *m_actionCollection;
    bool m_locked;
    QProcess *m_lockProcess;
    LockWindow *m_lockWindow;
};
} // namespace

#endif // SCREENLOCKER_KSLDAPP_H
