#include "request.h"

class Request::Private
{
public:
    QString deviceId;
    QString sessionId;
    QString path;
};

Request::Request(QObject *parent)
: QObject(parent)
, d(new Private)
{
}

Request::~Request()
{
    delete d;
}

void Request::setDeviceId(const QString &deviceId)
{
    if (d->deviceId != deviceId)
    {
        d->deviceId = deviceId;
        emit deviceIdChanged(deviceId);
    }
}

QString Request::deviceId() const
{
    return d->deviceId;
}

void Request::setSessionId(const QString &sessionId)
{
    if (d->sessionId != sessionId)
    {
        d->sessionId = sessionId;
        emit sessionIdChanged(sessionId);
    }
}

QString Request::sessionId() const
{
    return d->sessionId;
}

void Request::setPath(const QString &path)
{
    if (d->path != path)
    {
        d->path = path;
        emit pathChanged(path);
    }
}

QString Request::path() const
{
    return d->path;
}
