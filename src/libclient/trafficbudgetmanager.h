#ifndef TRAFFIC_BUDGET_MANAGER_H
#define TRAFFIC_BUDGET_MANAGER_H

#include <QObject>

#include "export.h"

class CLIENT_API TrafficBudgetManager : public QObject
{
    Q_OBJECT

public:
    TrafficBudgetManager();
    ~TrafficBudgetManager();

    void setAvailableTraffic(quint32 traffic);
    quint32 availableTraffic() const;

    bool addUsedTraffic(quint32 traffic);
    quint32 usedTraffic() const;

protected:
    class Private;
    Private *d;
};

#endif