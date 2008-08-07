/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#ifndef SERVICE_P_H
#define SERVICE_P_H

#include "servicejob.h"

#include <QGraphicsWidget>
#include <QMultiHash>
#include <QWidget>

#include <KTemporaryFile>

namespace Plasma
{

class ConfigXml;

class NullServiceJob : public ServiceJob
{
public:
    NullServiceJob(QObject *parent)
        : ServiceJob(QString(), QString(), QMap<QString, QVariant>(), parent)
    {
    }

    void start()
    {
        setErrorText(i18n("Invalid (null) service, can not perform any operations."));
        emitResult();
    }
};

class NullService : public Service
{
public:
    NullService(QObject *parent)
        : Service(parent)
    {
        setName("NullService");
    }

    ServiceJob *createJob(const QString &, QMap<QString, QVariant> &)
    {
        return new NullServiceJob(parent());
    }
};

class ServicePrivate
{
public:
    ServicePrivate(Service *service)
        : q(service),
          config(0),
          tempFile(0)
    {
    }
    ~ServicePrivate()
    {
        delete tempFile;
    }

    Service *q;
    QString destination;
    QString name;
    ConfigXml *config;
    KTemporaryFile *tempFile;
    QMultiHash<QWidget *, QString> associatedWidgets;
    QMultiHash<QGraphicsWidget *, QString> associatedGraphicsWidgets;

    void jobFinished(KJob* job)
    {
        emit q->finished(static_cast<ServiceJob*>(job));
    }

    void associatedWidgetDestroyed(QObject *obj)
    {
        associatedWidgets.remove(static_cast<QWidget*>(obj));
    }

    void associatedGraphicsWidgetDestroyed(QObject *obj)
    {
        associatedGraphicsWidgets.remove(static_cast<QGraphicsWidget*>(obj));
    }
};

} // namespace Plasma

#endif
