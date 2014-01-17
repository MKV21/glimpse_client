#include "httpgetrequest.h"

#include "../../log/logger.h"

LOGGER(HTTPGetRequest);


HTTPGetRequest::HTTPGetRequest(QObject *parent)
: Measurement(parent)
, currentStatus(HTTPGetRequest::Unknown)
, nam(NULL)
, reply(NULL)
{
}

HTTPGetRequest::~HTTPGetRequest()
{
}

bool HTTPGetRequest::prepare(NetworkManager *networkManager, const MeasurementDefinitionPtr &measurementDefinition)
{
    Q_UNUSED(networkManager);
    definition = measurementDefinition.dynamicCast<HTTPGetRequestDefinition>();
    if ( definition.isNull() ) {
        LOG_WARNING("Definition is empty");
        return false;
    }

    currentStatus = HTTPGetRequest::Unknown;

    // get network access manager
    nam = new QNetworkAccessManager(this);
    QObject::connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));

    // prepare request
    request = QNetworkRequest(definition->url);

    return true;
}

bool HTTPGetRequest::start()
{
    // TODO start timer

    // start
    reply = nam->get(request);

    setStatus(HTTPGetRequest::Running);

    return true;
}

void HTTPGetRequest::setStatus(Status status)
{
    if (currentStatus != status) {
        currentStatus = status;
        emit statusChanged(status);
    }
}

