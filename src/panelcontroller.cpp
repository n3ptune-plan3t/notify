#include "panelcontroller.h"
#include "notificationserver.h"
PanelController::PanelController(NotificationServer *server, QObject *parent)
    : QObject(parent), m_server(server) {}
void PanelController::TogglePanel() { emit toggleRequested(); }
void PanelController::ShowPanel() { emit showRequested(); }
void PanelController::HidePanel() { emit hideRequested(); }
void PanelController::ClearNotifications() { m_server->ClearNotifications(); }
void PanelController::ToggleDoNotDisturb() { m_server->ToggleDoNotDisturb(); }
int PanelController::GetNotificationCount() const { return m_server->notifications().size(); }
bool PanelController::GetDoNotDisturb() const { return m_server->doNotDisturb(); }
