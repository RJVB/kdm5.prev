#include <QFile>
#include <QMap>
#include <QRegExp>

#include <QTextStream>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <k3process.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>

struct AppletInfo
{
    double freeSpace;
    QString configFile;
    QString desktopFile;
};
typedef QMap<QString, AppletInfo> AppletInfoMap;

int main(int argc, char** argv)
{
    // We must disguise as Kicker in order to obtain the correct reverseLayout setting.
    KCmdLineArgs::init(argc, argv, "kicker", "", "", "", false);
    KApplication app(false);

    QStringList stretchableApplets;
    stretchableApplets << "taskbarapplet.desktop";

    QTextStream in (stdin, QIODevice::ReadOnly);
    QTextStream out(stdout, QIODevice::WriteOnly);

    QStringList appletIds;
    AppletInfoMap applets;

    QRegExp rxGroup("^\\[(.+)\\]$");
    QRegExp rxKeyValue("([^=]+)=[ \t]*([^\n]+)");
    QString currentGroup;

    QString line;
    while (!(line = in.readLine()).isNull())
    {
        if (rxGroup.indexIn(line) != -1)
        {
            currentGroup = rxGroup.cap(1);
            continue;
        }

        if (rxKeyValue.indexIn(line) != -1)
        {
            QString key   = rxKeyValue.cap(1);
            QString value = rxKeyValue.cap(2);

            if (key == "Applets")
            {
                appletIds = value.split(",", QString::SkipEmptyParts);
            }
            else if (key == "FreeSpace")
            {
                applets[currentGroup].freeSpace = value.toDouble();
            }
            else if (key == "ConfigFile")
            {
                applets[currentGroup].configFile = value;
            }
            else if (key == "DesktopFile")
            {
                applets[currentGroup].desktopFile = value;
            }
        }
    }

    if (QApplication::isRightToLeft())
    {
        // Reverse appletIds
        QStringList appletIdsRev;
        QStringList::ConstIterator it;
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            appletIdsRev.prepend(*it);
        }
        appletIds = appletIdsRev;

        // Adjust the FreeSpace values
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            applets[*it].freeSpace = 1 - applets[*it].freeSpace;

            // Take care of stretchable applets.
            if (stretchableApplets.contains(applets[*it].desktopFile))
            {
                if (it != appletIds.begin())
                {
                    applets[*it].freeSpace = applets[*(--it)].freeSpace; 
                    ++it;
                }
                else
                {
                    applets[*it].freeSpace = 0;
                }
            }
        }
    }

    // Write the changed entries to stdout.
    if (!appletIds.empty())
    {
        out << "[General]" << endl;
        out << "Applets2=" << appletIds.join(",") << endl;
        QStringList::ConstIterator it;
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            out << "[" << *it << "]" << endl;
            out << "FreeSpace2=" << applets[*it].freeSpace << endl;
        }
    }

    // Build a list of childpanel config files.
    QStringList childPanelConfigFiles;
    AppletInfoMap::ConstIterator it2;
    QStringList::ConstIterator it;
    for (it2 = applets.begin(); it2 != applets.end(); ++it2)
    {
        if (it2.value().desktopFile == "childpanelextension.desktop")
        {
            childPanelConfigFiles << it2.value().configFile;
        }
    }

    if (!childPanelConfigFiles.isEmpty())
    {
        // Create a temporary kconf_update .upd file for updating the childpanels
        KTemporaryFile tempFile;
        tempFile.setSuffix(".upd");
        tempFile.open();
        QTextStream upd ( &tempFile );
        for (it = childPanelConfigFiles.begin(); it != childPanelConfigFiles.end(); ++it)
        {
            upd << "Id=kde_3.4_reverseLayout" << endl;
            upd << "File=" << *it << endl;
            upd << "Script=kicker-3.4-reverseLayout" << endl;
            upd << endl;
        }
        upd.flush();

        // Run kconf_update on the childpanel config files.
        K3Process kconf_update;
        kconf_update << KStandardDirs::findExe("kconf_update") << tempFile.fileName();
        kconf_update.start(K3Process::Block);
    }
}
