# Qt Notify Center

A lightweight Qt 6 / C++20 notification daemon and notification-history panel. The interface uses **Qt Widgets**, not QML. There are no transient popup windows, no polling loops, and no periodic Waybar interval.

## X11 and Wayland

The panel uses a normal Qt top-level window, so it can run through Qt's `xcb` platform plugin on X11 and its `wayland` platform plugin on Wayland. The compositor manages placement and focus on Wayland.

Layer shell (`wlr-layer-shell`) is a Wayland-only protocol, not supported by X11 and not available on every Wayland compositor. This build deliberately avoids making it a hard dependency, preserving portability. A pinned-corner layer-shell panel would need an optional Wayland-specific backend.

## Features

- Freedesktop `org.freedesktop.Notifications` D-Bus service.
- Replacement by notification ID, close and clear actions.
- Bounded persistent JSON history.
- Do Not Disturb toggle.
- Notification action buttons.
- Notifications grouped by application in the panel.
- Plain INI configuration.
- Event-driven Waybar refresh via `SIGRTMIN+8`; no polling interval.
- No QML engine or Qt Quick libraries.

## Inline reply note

The standard Freedesktop notification API provides `ActionInvoked(notification_id, action_key)` but has no standard way to transmit arbitrary reply text. A generic inline reply box would not reliably deliver typed text to clients, so this implementation does not show a fake reply control. Client-specific reply support can be added for applications with a known protocol.

## Dependencies

CMake 3.21+, C++20 compiler, Qt 6.5+ Core/Gui/Widgets/DBus, session D-Bus, Ninja recommended.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/qt-notify-center
```

Start once per user session. Add `exec qt-notify-center` to the compositor startup config.

## Persistent popups

Each incoming notification creates a solid Qt Widgets popup. Popups do not expire
automatically: close them with **×** or use an action button. Closing a popup leaves
the notification in history; dismissing it in the history panel removes it. DND
suppresses new popup windows but still records notifications in history.

Popups use ordinary Qt top-level windows, without QML or a polling loop. Their
requested corner placement is generally honoured by X11; Wayland compositors control
top-level window placement and may reposition them. This is not a layer-shell surface.

## Configuration

On first run:

`~/.config/qt-notify-center/config.ini`

```ini
[ui]
panelWidth=410
panelHeight=640
background=#17191f
card=#242832
foreground=#f1f2f6
secondary=#aab0bf
border=#343843
urgentBackground=#3a252b
urgentBorder=#a84e60
```

Edit with any text editor; restart the daemon after changes.

## Waybar (event-driven)

```jsonc
"custom/notifications": {
  "exec": "qt-notify-centerctl --status",
  "return-type": "json",
  "format": "{}",
  "signal": 8,
  "on-click": "qt-notify-centerctl --toggle",
  "on-click-right": "qt-notify-centerctl --clear",
  "on-click-middle": "qt-notify-centerctl --dnd"
}
```

Do not add `interval`. The daemon sends `SIGRTMIN+8` when notification count or DND state changes.

## Commands

`qt-notify-centerctl --status`, `--toggle`, `--show`, `--hide`, `--clear`, `--dnd`.

## Data

- Config: `~/.config/qt-notify-center/config.ini`
- History: Qt application data directory, `notifications.json`
- DND: Qt settings storage


## GitHub Actions binary artifact

The workflow `.github/workflows/build.yml` builds the project on Ubuntu 24.04 and uploads `qt-notify-center-linux-x86_64` as a GitHub Actions artifact. Download it from the workflow run's **Artifacts** section. The binary is dynamically linked and requires compatible Qt 6 runtime libraries and a session D-Bus.
