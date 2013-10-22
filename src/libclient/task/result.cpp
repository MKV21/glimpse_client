#include "result.h"

class Result::Private
{
public:
    QDateTime dateTime;
    QVariant probeResult;
    QVariant peerResult;
};

Result::Result(const QDateTime &dateTime, const QVariant &probeResult, const QVariant &peerResult)
: d(new Private)
{
    d->dateTime = dateTime;
    d->probeResult = probeResult;
    d->peerResult = peerResult;
}

Result::~Result()
{
    delete d;
}

ResultPtr Result::fromVariant(const QVariant &variant)
{
    QVariantMap map = variant.toMap();

    return ResultPtr(new Result(map.value("date_time").toDateTime(),
                                map.value("probe_result").toString(),
                                map.value("peer_result").toString()));
}

QDateTime Result::dateTime() const
{
    return d->dateTime;
}

QVariant Result::probeResult() const
{
    return d->probeResult;
}

QVariant Result::peerResult() const
{
    return d->peerResult;
}

QVariant Result::toVariant() const
{
    QVariantMap map;
    map.insert("date_time", d->dateTime);
    map.insert("probe_result", d->probeResult);
    map.insert("peer_result", d->peerResult);
    return map;
}
