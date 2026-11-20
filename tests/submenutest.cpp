// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Opens a menu and its submenu (like Dolphin's "Create New") offscreen and
// reports how far the submenu overlaps its parent. Usage: submenutest <style-name>
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>

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

    QMenu menu;
    menu.addAction(QStringLiteral("Open"));
    QMenu *sub = menu.addMenu(QStringLiteral("Create New"));
    sub->addAction(QStringLiteral("Folder..."));
    sub->addAction(QStringLiteral("Text File..."));
    menu.addAction(QStringLiteral("Properties"));

    int result = 2;
    QObject::connect(sub, &QMenu::aboutToShow, &app, [&] {
        QTimer::singleShot(0, &app, [&] {
            const QRect p = menu.geometry();
            const QRect s = sub->geometry();
            const int overlap = p.right() + 1 - s.left(); // >0 means submenu covers parent
            std::printf("%s: parent x=%d..%d, submenu starts at x=%d, overlap=%dpx\n",
                        argv[1], p.left(), p.right(), s.left(), overlap);
            result = overlap > 0 ? 1 : 0;
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
    return result;
}
