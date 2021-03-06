#include "settings.h"
#include "deviceinfo.h"
#include "log/logger.h"
#include "types.h"

#include <QSettings>
#include <QCryptographicHash>

static QLatin1String CONFIG_SERVER("supervisor.measure-it.net");
static QLatin1String SUPERVISOR_SERVER("supervisor.measure-it.net");

LOGGER(Settings);

class Settings::Private
{
public:
    void sync();

    GetConfigResponse config;
    QSettings settings;
};

void Settings::Private::sync()
{
    settings.setValue("config", config.toVariant());
    settings.sync();
}

Settings::Settings(QObject *parent)
: QObject(parent)
, d(new Private)
{
}

Settings::~Settings()
{
    d->sync();
    delete d;
}

Settings::StorageType Settings::init()
{
    bool newSettings = deviceId().isNull();

    DeviceInfo info;

    if (deviceId().isEmpty())
    {
        QString generatedDeviceId = info.deviceId();

        if (generatedDeviceId.isEmpty())
        {
            QCryptographicHash hash(QCryptographicHash::Sha224);
            hash.addData(QUuid::createUuid().toByteArray());

            setDeviceId(QString::fromLatin1(hash.result().toHex()));

            LOG_INFO("Generated fallback device ID");
        }
        else
        {
            setDeviceId(generatedDeviceId);
        }
    }

    LOG_INFO(QString("Device ID: %1").arg(deviceId()));

    // this is an emergency fix, we can remove this later
    if (!userId().isEmpty())
    {
        if (!d->settings.contains("hashed-user-id"))
        {
            d->settings.setValue("hashed-user-id", userIdToHash(userId()));
        }
    }

    // Create new settings
    if (newSettings)
    {
        d->config.setConfigAddress(CONFIG_SERVER);
        d->config.setSupervisorAdress(SUPERVISOR_SERVER);
        setUsedTraffic(0);
        setUsedMobileTraffic(0);
        LOG_DEBUG("Created new settings for this device");

        return NewSettings;
    }
    else
    {
        d->config.fillFromVariant(qvariant_cast<QVariantMap>(d->settings.value("config")));

        // Always set the controller address if we have none
        if (d->config.configAddress().isEmpty())
        {
            LOG_WARNING("Config controller address lost, setting back default one");
            d->config.setConfigAddress(SUPERVISOR_SERVER);
        }

        if (d->config.supervisorAddress().isEmpty())
        {
            LOG_WARNING("Supervisor address lost, setting back default one");
            d->config.setConfigAddress(CONFIG_SERVER);
        }

        LOG_DEBUG("Loaded existing settings for this device");
        return ExistingSettings;
    }
}

bool Settings::hasLoginData() const
{
    return !userId().isEmpty() && !apiKey().isEmpty();
}

void Settings::setDeviceName(const QString &deviceName)
{
    if (this->deviceName() != deviceName)
    {
        d->settings.setValue("device-name", deviceName);
        emit deviceNameChanged(deviceName);
    }
}

QString Settings::deviceName() const
{
    return d->settings.value("device-name").toString();
}

void Settings::setDeviceId(const QString &deviceId)
{
    if (this->deviceId() != deviceId)
    {
        d->settings.setValue("device-id", deviceId);
        emit deviceIdChanged(deviceId);
    }
}

QString Settings::deviceId() const
{
    return d->settings.value("device-id").toString();
}

void Settings::setUserId(const QString &userId)
{
    if (this->userId() != userId)
    {
        d->settings.setValue("user-id", userId);
        QString hash = userIdToHash(userId);
        d->settings.setValue("hashed-user-id", hash);
        emit userIdChanged(userId);
        emit hashedUserIdChanged(hash);
    }
}

QString Settings::userId() const
{
    return d->settings.value("user-id").toString();
}

QString Settings::hashedUserId() const
{
    return d->settings.value("hashed-user-id").toString();
}

void Settings::setApiKey(const QString &apiKey)
{
    if (this->apiKey() != apiKey)
    {
        d->settings.setValue("api-key", apiKey);
        emit apiKeyChanged(apiKey);
    }
}

QString Settings::apiKey() const
{
    return d->settings.value("api-key").toString();
}

bool Settings::isPassive() const
{
    return d->settings.value("passive").toBool();
}

void Settings::setPassive(bool passive)
{
    if (this->isPassive() != passive)
    {
        d->settings.setValue("passive", passive);
        emit passiveChanged(passive);
    }
}

void Settings::setAllowedTraffic(quint32 traffic)
{
    if (this->allowedTraffic() != traffic)
    {
        d->settings.setValue("allowed-traffic", traffic);
        emit allowedTrafficChanged(traffic);
    }
}

quint32 Settings::allowedTraffic() const
{
    return d->settings.value("allowed-traffic", 524288000).toUInt(); // defaults to 500 MB
}

void Settings::setAllowedMobileTraffic(quint32 traffic)
{
    if (this->allowedMobileTraffic() != traffic)
    {
        d->settings.setValue("allowed-mobile-traffic", traffic);
        emit allowedMobileTrafficChanged(traffic);
    }
}

quint32 Settings::allowedMobileTraffic() const
{
    return d->settings.value("allowed-mobile-traffic", 31457280).toUInt(); // defaults to 30 MB
}

void Settings::setUsedTraffic(quint32 traffic)
{
    if (this->usedTraffic() != traffic)
    {
        d->settings.setValue("used-traffic", traffic);
        emit usedTrafficChanged(traffic);
    }
}

quint32 Settings::usedTraffic() const
{
    return d->settings.value("used-traffic", 0).toUInt();
}

void Settings::setUsedMobileTraffic(quint32 traffic)
{
    if (this->usedMobileTraffic() != traffic)
    {
        d->settings.setValue("used-mobile-traffic", traffic);
        emit usedMobileTrafficChanged(traffic);
    }
}

quint32 Settings::usedMobileTraffic() const
{
    return d->settings.value("used-mobile-traffic", 0).toUInt();
}

void Settings::setTrafficBudgetManagerActive(bool active)
{
    if (this->trafficBudgetManagerActive() != active)
    {
        d->settings.setValue("traffic-budget-manager-active", active);
        emit trafficBudgetManagerActiveChanged(active);
    }
}

bool Settings::trafficBudgetManagerActive() const
{
    return d->settings.value("traffic-budget-manager-active", false).toBool();
}

void Settings::setMobileMeasurementsActive(bool active)
{
    if (this->mobileMeasurementsActive() != active)
    {
        d->settings.setValue("mobile-measurements-active", active);
        emit mobileMeasurementsActiveChanged(active);
    }
}

bool Settings::mobileMeasurementsActive() const
{
    return d->settings.value("mobile-measurements-active", false).toBool();
}

void Settings::setBacklog(quint32 backlog)
{
    if (this->backlog() != backlog)
    {
        d->settings.setValue("backlog", backlog);
        emit backlogChanged(backlog);
    }
}

quint32 Settings::backlog() const
{
    return d->settings.value("backlog", 10).toUInt();
}

void Settings::setGoogleAnalyticsActive(bool active)
{
    if (this->googleAnalyticsActive() != active)
    {
        d->settings.setValue("google-analytics-active", active);
        emit googleAnalyticsActiveChanged(active);
    }
}

bool Settings::googleAnalyticsActive() const
{
    return d->settings.value("google-analytics-active", true).toBool();
}

GetConfigResponse *Settings::config() const
{
    return &d->config;
}

void Settings::clear()
{
    d->settings.clear();
    d->settings.sync();
}

void Settings::sync()
{
    d->sync();
}
