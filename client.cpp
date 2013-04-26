#include "client.h"
#include "discovery.h"
#include "requests.h"

#include <QUdpSocket>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QHostInfo>
#include <QDataStream>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QJsonDocument>
#include <QJsonArray>
#else
#ifdef Q_OS_WIN
#include <QJson/Parser>
#else
#include <qjson/parser.h>
#endif
#endif

#include <QDebug>

const QUrl masterUrl = QUrl("http://mplane.informatik.hs-augsburg.de:16001");

class Client::Private : public QObject
{
    Q_OBJECT

public:
    Private(Client* q)
    : q(q)
    , status(Client::Registered)
    , networkAccessManager(new QNetworkAccessManager(q))
    {
        connect(&discovery, SIGNAL(finished()), this, SLOT(onDiscoveryFinished()));
        connect(&managerSocket, SIGNAL(readyRead()), this, SLOT(onDatagramReady()));
        connect(&aliveTimer, SIGNAL(timeout()), this, SLOT(onAliveTimer()));

        aliveTimer.setInterval(120 * 1000);
        aliveTimer.start();
    }

    Client* q;

    // Properties
    Client::Status status;
    QNetworkAccessManager* networkAccessManager;
    Discovery discovery;
    QUdpSocket managerSocket;
    QTimer aliveTimer;
    QHostInfo aliveInfo;

    // Functions
    void setStatus(Client::Status status);

    void processDatagram(const QByteArray &datagram, const QHostAddress& host, quint16 port);
    void processClientInfoRequest();
    void processPeerRequest();

    void sendClientInfo();
    void sendPeerResponse(const QHostAddress &host, quint16 port);
    void sendPeerRequest(bool manual = false);

public slots:
    void onDatagramReady();
    void onLookupFinished(const QHostInfo &host);
    void onAliveTimer();
    void onDiscoveryFinished();

    void onRegisterFinished();
    void onRegisterError(QNetworkReply::NetworkError error);

    void onPeerRequestFinished();
    void onPeerRequestError(QNetworkReply::NetworkError error);
};

void Client::Private::setStatus(Client::Status status)
{
    if ( this->status == status )
        return;

    this->status = status;
    emit q->statusChanged();
}

void Client::Private::processDatagram(const QByteArray& datagram, const QHostAddress &host, quint16 port)
{
    const RequestType* type = (RequestType*)datagram.constData();

    qDebug() << Q_FUNC_INFO << *type;

    switch(*type) {
    case ClientInfoRequest:
        processClientInfoRequest();
        break;

    case PeerRequest:
        sendPeerRequest();
        break;

    case PeerResponse:
        // TODO: Code
        break;

    default:
        qDebug() << "Received unknown request from" << host.toString() << port;
    }
}

void Client::Private::processClientInfoRequest()
{
    qDebug() << Q_FUNC_INFO;

    // Get some information
    discovery.discover();
}

void Client::Private::processPeerRequest()
{
    qDebug() << Q_FUNC_INFO;
}

void Client::Private::sendClientInfo()
{
    ClientInfo info;
    QByteArray data = QJsonDocument::fromVariant(info.toVariant()).toJson();

    QUrl url = masterUrl;
    url.setPath("/info");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/json");

    QNetworkReply* reply = networkAccessManager->post(request, data);
    reply->ignoreSslErrors(); // TODO: Remove the evil
    connect(reply, SIGNAL(finished()), this, SLOT(onRegisterFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRegisterError(QNetworkReply::NetworkError)));
}

void Client::Private::sendPeerResponse(const QHostAddress &host, quint16 port)
{
    RequestType type = PeerResponse;

    managerSocket.writeDatagram((const char*)&type, sizeof(type), host, port);
}

void Client::Private::sendPeerRequest(bool manual)
{
    Request r;
    QByteArray data = QJsonDocument::fromVariant(r.toVariant()).toJson();

    QUrl url = masterUrl;
    if (manual)
        url.setPath("/manualrequest");
    else
        url.setPath("/request");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/json");

    QNetworkReply* reply = networkAccessManager->post(request, data);
    reply->ignoreSslErrors();
    connect(reply, SIGNAL(finished()), this, SLOT(onPeerRequestFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onPeerRequestError(QNetworkReply::NetworkError)));
}

void Client::Private::onDatagramReady()
{
    while (managerSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(managerSocket.pendingDatagramSize());

        QHostAddress host;
        quint16 port;
        managerSocket.readDatagram(datagram.data(), datagram.size(), &host, &port);

        // Process the datagram
        processDatagram(datagram, host, port);
    }
}

void Client::Private::onLookupFinished(const QHostInfo& host)
{
    aliveInfo = host;

    if ( !host.addresses().isEmpty() ) {
        qDebug() << "Alive host found:" << host.addresses().first();
        onAliveTimer();
    } else {
        // Wait some seconds before trying again
        QTimer::singleShot(5000, this, SLOT(onAliveTimer()));
    }
}

void Client::Private::onAliveTimer()
{
    if (aliveInfo.addresses().isEmpty()) {
        aliveInfo.lookupHost(masterUrl.host(), this, SLOT(onLookupFinished(QHostInfo)));
        qDebug() << "Looking up alive host";
    } else {
        managerSocket.writeDatagram(QByteArray(), aliveInfo.addresses().first(), 16000);
        qDebug() << "Alive packet sent";
    }
}

void Client::Private::onDiscoveryFinished()
{
   sendClientInfo();
}

void Client::Private::onRegisterFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError)
        setStatus(Client::Registered);
    else
        setStatus(Client::Unregistered);

    reply->deleteLater();
}

void Client::Private::onRegisterError(QNetworkReply::NetworkError error)
{
    setStatus(Client::Unregistered);

    qDebug() << "Registration error" << enumToString(QNetworkReply, "NetworkError", error);
}

void Client::Private::onPeerRequestFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);

        if ( error.error == QJsonParseError::NoError ) {
            QJsonArray root = document.array();

            foreach(QJsonValue value, root) {
                qDebug() << "Received peer:" << value.toString();
            }
        } else {
            qDebug() << Q_FUNC_INFO << "Json error:" << error.errorString();
        }
    } else {
        // Hmm?
    }

    reply->deleteLater();
}

void Client::Private::onPeerRequestError(QNetworkReply::NetworkError error)
{
    qDebug() << "Peer request error" << enumToString(QNetworkReply, "NetworkError", error);
}

Client::Client(QObject *parent)
: QObject(parent)
, d(new Private(this))
{
}

Client::~Client()
{
    delete d;
}

Client *Client::instance()
{
    static Client* ins = NULL;

    if ( !ins )
        ins = new Client();

    return ins;
}

bool Client::init()
{
    bool ok = false;
    ok = d->managerSocket.bind(1337);

    // Cheat ...
    d->onAliveTimer();

    return ok;
}

Client::Status Client::status() const
{
    return d->status;
}

QNetworkAccessManager *Client::networkAccessManager() const
{
    return d->networkAccessManager;
}

QAbstractSocket *Client::managerSocket() const
{
    return &d->managerSocket;
}

void Client::requestTest()
{
    d->sendPeerRequest(true);
}

#include "client.moc"
