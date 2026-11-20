// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Checks that a hovered folder button in Dolphin's path bar doesn't paint over
// the bar's sunken border. KIO isn't needed: the two classes below use the same
// class names as KIO's and draw the same way KIO 6.32 does
// (KUrlNavigator::paintEvent, KUrlNavigatorButtonBase::drawHoverBackground).
// Usage: pathbartest <style-name>
#include <QApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QPainter>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <cstdio>

class KUrlNavigator : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        QStyleOptionFrame option;
        option.initFrom(this);
        option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
        option.midLineWidth = 0;
        option.frameShape = QFrame::StyledPanel;
        option.palette.setColor(QPalette::Base, palette().alternateBase().color());
        option.state |= QStyle::State_Enabled;
        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);
    }
};

// Same namespace as KIO's class, so the style sees the same full class name
// ("KDEPrivate::KUrlNavigatorButtonBase") as in real Dolphin.
namespace KDEPrivate
{
class KUrlNavigatorButtonBase : public QPushButton
{
    Q_OBJECT
public:
    using QPushButton::QPushButton;
    bool hovered = false;

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!hovered)
            return;
        QPainter painter(this);
        QStyleOptionButton buttonOption;
        buttonOption.initFrom(this);
        buttonOption.rect = QRect(0, 0, width(), height());
        buttonOption.features = QStyleOptionButton::Flat;
        style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &buttonOption, &painter, this);
    }
};
} // namespace KDEPrivate
using KDEPrivate::KUrlNavigatorButtonBase;

static QImage renderBar(QWidget *bar)
{
    QImage img(bar->size(), QImage::Format_ARGB32);
    img.fill(Qt::magenta);
    bar->render(&img);
    return img;
}

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

    KUrlNavigator bar;
    auto *layout = new QHBoxLayout(&bar);
    layout->setContentsMargins(0, 0, 0, 0); // as in KIO: buttons span the border too
    layout->setSpacing(0);
    auto *home = new KUrlNavigatorButtonBase(QStringLiteral("Home"), &bar);
    auto *docs = new KUrlNavigatorButtonBase(QStringLiteral("Documents"), &bar);
    home->setFixedSize(60, 26);
    docs->setFixedSize(90, 26);
    layout->addWidget(home);
    layout->addWidget(docs);
    layout->addStretch(1);
    bar.resize(300, 26);
    bar.ensurePolished();
    layout->activate();

    const QImage before = renderBar(&bar);
    home->hovered = docs->hovered = true;
    const QImage after = renderBar(&bar);
    after.save(QStringLiteral("pathbar-%1.png").arg(QString::fromLocal8Bit(argv[1])));

    // Every pixel of the bar's border must look the same with or without hover.
    const int frame = bar.style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, &bar);
    const QRect inner = bar.rect().adjusted(frame, frame, -frame, -frame);
    int changed = 0;
    for (int y = 0; y < before.height(); ++y)
        for (int x = 0; x < before.width(); ++x)
            if (!inner.contains(x, y) && before.pixel(x, y) != after.pixel(x, y))
                ++changed;
    // And the hover box must still show up inside the border.
    const bool hoverVisible = before.pixel(30, 13) != after.pixel(30, 13);

    std::printf("%s: %d border pixels covered by hover, hover box %s\n", argv[1], changed,
                hoverVisible ? "visible" : "MISSING");
    return (changed == 0 && hoverVisible) ? 0 : 1;
}

#include "pathbartest.moc"
