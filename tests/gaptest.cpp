// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Renders a see-through QMenu (like Plasma's desktop menu) offscreen and
// reports any fully transparent pixels. Usage: gaptest <style-name>
#include <QApplication>
#include <QImage>
#include <QMenu>
#include <QStyleFactory>
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
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.addAction(QStringLiteral("Add Widgets..."));
    menu.addSeparator();
    menu.addAction(QStringLiteral("Configure Desktop and Wallpaper..."));
    menu.adjustSize();

    QImage img(menu.size(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    menu.render(&img, QPoint(), QRegion(), QWidget::DrawChildren);
    img.save(QStringLiteral("menu-%1.png").arg(QString::fromLocal8Bit(argv[1])));

    int holes = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (qAlpha(img.pixel(x, y)) == 0)
                ++holes;
    std::printf("%s: %dx%d menu, %d see-through pixels\n", argv[1], img.width(), img.height(), holes);
    return holes ? 1 : 0;
}
