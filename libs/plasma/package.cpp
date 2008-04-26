/******************************************************************************
*   Copyright 2007 by Aaron Seigo <aseigo@kde.org>                        *
*   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>               *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#include "package.h"

#include <QDir>
#include <QFile>

#include <KArchiveDirectory>
#include <KArchiveEntry>
#include <KComponentData>
#include <KDesktopFile>
#include <KIO/FileCopyJob>
#include <KIO/Job>
#include <KPluginInfo>
#include <KStandardDirs>
#include <KTempDir>
#include <KTemporaryFile>
#include <KZip>
#include <KDebug>

#include "packagemetadata.h"

namespace Plasma
{

class Package::Private
{
public:
    Private(const PackageStructure::Ptr st, const QString& p)
        : structure(st),
          basePath(p),
          valid(QFile::exists(basePath)),
          metadata(0)
    {
        if (valid && basePath[basePath.length() - 1] != '/') {
            basePath.append('/');
        }
    }

    ~Private()
    {
        delete metadata;
    }

    PackageStructure::Ptr structure;
    QString basePath;
    bool valid;
    PackageMetadata *metadata;
};

Package::Package(const QString& packageRoot, const QString& package, PackageStructure::Ptr structure)
    : d(new Private(structure, packageRoot + '/' + package))
{
    structure->setPath(d->basePath);
}

Package::Package(const QString &packagePath, PackageStructure::Ptr structure)
    : d(new Private(structure, packagePath))
{
    structure->setPath(d->basePath);
}

Package::~Package()
{
    delete d;
}

bool Package::isValid() const
{
    if (!d->valid) {
        return false;
    }

    foreach (const char *dir, d->structure->requiredDirectories()) {
        if (!QFile::exists(d->basePath + d->structure->contentsPrefix() + d->structure->path(dir))) {
            kWarning(505) << "Could not find required directory" << dir;
            d->valid = false;
            return false;
        }
    }

    foreach (const char *file, d->structure->requiredFiles()) {
        if (!QFile::exists(d->basePath + d->structure->contentsPrefix() + d->structure->path(file))) {
            kWarning(505) << "Could not find required file" << file << ", look in"
                          << d->basePath + d->structure->contentsPrefix() + d->structure->path(file) << endl;
            d->valid = false;
            return false;
        }
    }

    return true;
}

QString Package::filePath(const char* fileType, const QString& filename) const
{
    if (!d->valid) {
        kDebug() << "package is not valid";
        return QString();
    }

    QString path = d->structure->path(fileType);

    if (path.isEmpty()) {
        kDebug() << "no matching path came of it";
        return QString();
    }

    path.prepend(d->basePath + d->structure->contentsPrefix());

    if (!filename.isEmpty()) {
        path.append("/").append(filename);
    }

    if (QFile::exists(path)) {
        return path;
    }

    kDebug() << path << "does not exist";
    return QString();
}

QString Package::filePath(const char* fileType) const
{
    return filePath(fileType, QString());
}

QStringList Package::entryList(const char* fileType) const
{
    if (!d->valid) {
        return QStringList();
    }

    QString path = d->structure->path(fileType);
    if (path.isEmpty()) {
        return QStringList();
    }

    QDir dir(d->basePath + d->structure->contentsPrefix() + path);

    if (!dir.exists()) {
        return QStringList();
    }

    return dir.entryList(QDir::Files | QDir::Readable);
}

const PackageMetadata* Package::metadata() const
{
    //FIXME: this only works for native plasma packges; should fall back to... PackageStructure?
    if (!d->metadata) {
        d->metadata = new PackageMetadata(d->basePath + "metadata.desktop");
    }
    return d->metadata;
}

const QString Package::path() const
{
    return d->basePath;
}

const PackageStructure::Ptr Package::structure() const
{
    return d->structure;
}

//TODO: provide a version of this that allows one to ask for certain types of packages, etc?
//      should we be using KService here instead/as well?
QStringList Package::knownPackages(const QString& packageRoot) // static
{
    QDir dir(packageRoot);

    if (!dir.exists()) {
        return QStringList();
    }

    QStringList packages;

    foreach (const QString& sdir, dir.entryList(QDir::AllDirs | QDir::Readable)) {
        QString metadata = packageRoot + '/' + sdir + "/metadata.desktop";
        if (QFile::exists(metadata)) {
            PackageMetadata m(metadata);
            packages << m.pluginName();
        }
    }

    return packages;
}

bool Package::installPackage(const QString& package,
                             const QString& packageRoot) // static
{
    //TODO: report *what* failed if something does fail
    QDir root(packageRoot);

    if (!root.exists()) {
        KStandardDirs::makeDir(packageRoot);
        if (!root.exists()) {
            kWarning(505) << "Could not create package root directory:" << packageRoot;
            return false;
        }
    }

    QFileInfo fileInfo(package);
    if (!fileInfo.exists()) {
        kWarning(505) << "No such file:" << package;
        return false;
    }

    QString path;
    KTempDir tempdir;
    bool archivedPackage = false;

    if (fileInfo.isDir()) {
        // we have a directory, so let's just install what is in there
        path = package;

        // make sure we end in a slash!
        if (path[path.size() - 1] != '/') {
            path.append('/');
        }
    } else {
        KZip archive(package);
        if (!archive.open(QIODevice::ReadOnly)) {
            kWarning(505) << "Could not open package file:" << package;
            return false;
        }

        archivedPackage = true;
        const KArchiveDirectory* source = archive.directory();
        const KArchiveEntry* metadata = source->entry("metadata.desktop");

        if (!metadata) {
            kWarning(505) << "No metadata file in package" << package;
            return false;
        }

        path = tempdir.name();
        source->copyTo(path);
    }

    QString metadataPath = path + "metadata.desktop";
    if (!QFile::exists(metadataPath)) {
        kWarning(505) << "No metadata file in package" << package;
        return false;
    }

    PackageMetadata meta(metadataPath);
    QString targetName = meta.pluginName();

    if (targetName.isEmpty()) {
        kWarning(505) << "Package plugin name not specified";
        return false;
    }

    targetName = packageRoot + '/' + targetName;
    if (QFile::exists(targetName)) {
        kWarning(505) << targetName << "already exists";
        return false;
    }

    KIO::FileCopyJob* job(0);

    if (archivedPackage) {
        // it's in a temp dir, so just move it over.
        job = KIO::file_move(path, targetName, -1, KIO::HideProgressInfo);
    } else {
        // it's a directory containing the stuff, so copy the contents rather
        // than move them
        job = KIO::file_copy(path, targetName, -1, KIO::HideProgressInfo);
    }

    if (!job->exec()) {
        kWarning(505) << "Could not move package to destination:" << targetName;
        return false;
    }

    if (archivedPackage) {
        // no need to remove the temp dir (which has been successfully moved if it's an archive)
        tempdir.setAutoRemove(false);
    }

    // and now we register it as a service =)
    targetName.append("/metadata.desktop");
    KConfigGroup cg = KDesktopFile(targetName).desktopGroup();

    // should not installing it as a service disqualify it?
    // i don't think so since KServiceTypeTrader may not be
    // used by the installing app in any case, and the
    // package is properly installed - aseigo

    //TODO: reduce code duplication with registerPackage below

    QFile icon(packageRoot + cg.readEntry("Icon"));
    if (icon.exists()) {
        QString installedIcon("plasma_applet_" + meta.pluginName() + cg.readEntry("Icon"));
        meta.write(targetName, installedIcon);
        installedIcon = KStandardDirs::locateLocal("icon", installedIcon);
        job = KIO::file_copy(icon.fileName(), installedIcon, -1, KIO::HideProgressInfo);
        job->exec();
    }

    QString serviceName(KGlobal::mainComponent().componentName() + "_plasma_applet_" + meta.pluginName());
    QString service = KStandardDirs::locateLocal("services", serviceName + ".desktop");
    job = KIO::file_copy(targetName, service, -1, KIO::HideProgressInfo);
    job->exec();

    return true;
}

bool Package::registerPackage(const PackageMetadata &data, const QString &iconPath)
{
    QString serviceName("plasma-applet-" + data.pluginName());
    QString service = KStandardDirs::locateLocal("services", serviceName + ".desktop");

    if (data.pluginName().isEmpty()) {
        return false;
    }

    data.write(service);

    KDesktopFile config(service);
    KConfigGroup cg = config.desktopGroup();
    cg.writeEntry("Type", "Service");
    //TODO do we really like to just install all packages as applet/containment? Probably
    //it would make sense to let the packages themself decide what they are.
    cg.writeEntry("X-KDE-ServiceTypes", "Plasma/Applet,Plasma/Containment");
    cg.writeEntry("X-KDE-PluginInfo-EnabledByDefault", true);

    QFile icon(iconPath);
    if (icon.exists()) {
        //FIXME: the '/' search will break on non-UNIX. do we care?
        QString installedIcon("plasma_applet_" + data.pluginName() +
                              iconPath.right(iconPath.length() - iconPath.lastIndexOf("/")));
        cg.writeEntry("Icon", installedIcon);
        installedIcon = KStandardDirs::locateLocal("icon", installedIcon);
        KIO::FileCopyJob *job = KIO::file_copy(iconPath, installedIcon, -1, KIO::HideProgressInfo);
        job->exec();
    }

    return true;
}

bool Package::createPackage(const PackageMetadata &metadata,
                            const QString &source,
                            const QString &destination,
                            const QString &icon) // static
{
    if (!metadata.isValid()) {
        kWarning(550) << "Metadata file is not complete";
        return false;
    }

    // write metadata in a temporary file
    KTemporaryFile metadataFile;
    if (!metadataFile.open()) {
        return false;
    }
    metadata.write(metadataFile.fileName(), icon);

    // put everything into a zip archive
    KZip creation(destination);
    creation.setCompression(KZip::NoCompression);
    if (!creation.open(QIODevice::WriteOnly)) {
        return false;
    }

    creation.addLocalFile(metadataFile.fileName(), "metadata.desktop");
    creation.addLocalDirectory(source, "contents");
    creation.close();
    return true;
}


} // Namespace
