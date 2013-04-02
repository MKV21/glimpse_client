#include "discovery.h"

#include <QHostAddress>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#ifdef HAVE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#endif // HAVE_UPNP

#ifdef HAVE_STUN
# ifndef Q_OS_WIN
#  include <unistd.h>
# endif // Q_OS_WIN
#include "stund/udp.h"
#include "stund/stun.h"
#endif // HAVE_STUN

class Discovery::Private : public QObject
{
    Q_OBJECT

public:
    Private(Discovery* q)
    : q(q)
    {
        connect(&watcher, SIGNAL(finished()), this, SLOT(discoveryFinished()));
    }

    Discovery* q;

    // Properties
    QFutureWatcher<Discovery::DiscoveryHash> watcher;
    Discovery::DiscoveryHash data;

    // Functions
    static Discovery::DiscoveryHash discover();
    static void upnpDiscover(Discovery::DiscoveryHash& hash);
    static void stunDiscover(Discovery::DiscoveryHash& hash);

public slots:
    void discoveryFinished();
};

Discovery::DiscoveryHash Discovery::Private::discover()
{
    Discovery::DiscoveryHash hash;
    upnpDiscover(hash);
    stunDiscover(hash);
    return hash;
}

void Discovery::Private::upnpDiscover(Discovery::DiscoveryHash &hash)
{
#ifdef HAVE_UPNP
    int error = 0;

    UPNPDev* devlist = ::upnpDiscover(2000, NULL, NULL, FALSE, FALSE, &error);
    if (devlist) {
        UPNPUrls urls;
        IGDdatas data;
        char lanaddr[64];

        int code = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
        if (code == 1) {
            hash.insert(LanIpAddress, QLatin1String(lanaddr));

            char externalIP[40];
            if (UPNPCOMMAND_SUCCESS == UPNP_GetExternalIPAddress(urls.controlURL,
                                                                 data.first.servicetype,
                                                                 externalIP)) {
                hash.insert(ExternalIpAddress, QLatin1String(externalIP));
            }

            char connectionType[64];
            if(UPNPCOMMAND_SUCCESS == UPNP_GetConnectionTypeInfo(urls.controlURL,
                                                                 data.first.servicetype,
                                                                 connectionType)) {
                hash.insert(ConnectionType, QLatin1String(connectionType));
            }

            quint32 uplink, downlink;
            if (UPNPCOMMAND_SUCCESS == UPNP_GetLinkLayerMaxBitRates(urls.controlURL_CIF,
                                                                    data.CIF.servicetype,
                                                                    &downlink, &uplink)) {
                hash.insert(LinkLayerMaxDownload, downlink);
                hash.insert(LinkLayerMaxUpload, uplink);
            }
        }

        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
    }
#endif // HAVE_UPNP
}

void Discovery::Private::stunDiscover(Discovery::DiscoveryHash &hash)
{
#ifdef HAVE_STUN
    StunAddress4 stunServerAddr;
    stunParseServerName("stunserver.org", stunServerAddr);

    bool preserverPort = false;
    bool hairpin = false;

    ::NatType stype = stunNatType(stunServerAddr, false, &preserverPort, &hairpin, stunRandomPort(), NULL);
    hash.insert(Discovery::NatType, stype);
#endif // HAVE_STUN
}

void Discovery::Private::discoveryFinished()
{
    data = watcher.result();
    emit q->finished();
}

Discovery::Discovery(QObject *parent)
: QObject(parent)
, d(new Private(this))
{
}

Discovery::~Discovery()
{
    delete d;
}

Discovery::DiscoveryHash Discovery::data() const
{
    return d->data;
}

void Discovery::discover()
{
    d->data.clear();
    d->watcher.setFuture( QtConcurrent::run(Discovery::Private::discover) );
}

#include "discovery.moc"