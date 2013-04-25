#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client.h"

#include <QPointer>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
    void setClient(Client* client);
    Client* client() const;

protected slots:
    void startClicked();
    void noConnectionClicked();
    void connectionSlowClicked();
    void statusChanged();

private:
    Ui::MainWindow *ui;
    QPointer<Client> m_client;
    QSystemTrayIcon m_tray;
    QMenu m_trayMenu;
};

#endif // MAINWINDOW_H
