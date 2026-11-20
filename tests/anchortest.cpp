// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Checks that the style's Wayland submenu placement never removes a popup
// anchor that the app set itself, the way Plasma's system tray does for its
// icon menus, and that it does clean up its own anchor.
// Usage: anchortest <style-name>
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QStyleFactory>
#include <QTimer>
#include <QWindow>
#include <cstdio>

static const char anchorProperty[] = "_q_waylandPopupAnchorRect";

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc < 2)
        return 2;
    QStyle *style = QStyleFactory::create(QString::fromLocal8Bit(argv[1]));
    if (!style) {
        std::printf("style '%s' not found\n", argv[1]);
        return 2;
    }
    QApplication::setStyle(style);
    int failures = 0;

    // 1. A tray-style menu: the app sets the anchor, then pops up at (0,0).
    {
        QMenu tray;
        tray.addAction(QStringLiteral("Show"));
        tray.addAction(QStringLiteral("Quit"));
        tray.winId();
        const QRect trayIcon(1800, 1050, 24, 24);
        tray.windowHandle()->setProperty(anchorProperty, trayIcon);
        tray.popup(QPoint(0, 0));
        const bool kept = tray.windowHandle()->property(anchorProperty).toRect() == trayIcon;
        std::printf("%s: app-set anchor %s\n", argv[1], kept ? "kept" : "REMOVED");
        failures += kept ? 0 : 1;
        tray.hide();
    }

    // 2. A submenu gets our anchor, and loses it when later opened on its own.
    {
        QMenu menu;
        menu.addAction(QStringLiteral("Open"));
        QMenu *sub = menu.addMenu(QStringLiteral("Create New"));
        sub->addAction(QStringLiteral("Folder..."));

        bool setAsSubmenu = false;
        QObject::connect(sub, &QMenu::aboutToShow, &app, [&] {
            QTimer::singleShot(0, &app, [&] {
                setAsSubmenu = sub->windowHandle()->property(anchorProperty).isValid();
                app.quit();
            });
        });
        menu.popup(QPoint(100, 100));
        QTimer::singleShot(50, &app, [&] {
            menu.setActiveAction(sub->menuAction());
            QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QApplication::sendEvent(&menu, &right);
        });
        QTimer::singleShot(2000, &app, &QApplication::quit);
        app.exec();
        menu.hide();
        sub->hide();

        sub->popup(QPoint(300, 300));
        const bool cleared = !sub->windowHandle()->property(anchorProperty).isValid();
        sub->hide();

        // With a style that doesn't place submenus, there's nothing to set.
        const bool placesSubmenus = QString::fromLocal8Bit(argv[1]).compare(QLatin1String("win9xplus"), Qt::CaseInsensitive) == 0;
        std::printf("%s: submenu anchor %s, removed when opened alone: %s\n", argv[1],
                    setAsSubmenu ? "set" : "not set", cleared ? "yes" : "NO");
        if ((placesSubmenus && !setAsSubmenu) || !cleared)
            ++failures;
    }
    return failures ? 1 : 0;
}
