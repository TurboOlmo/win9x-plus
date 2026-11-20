// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Win9x Plus: Qt's built-in "Windows" (MS Windows 9X) style, with fixes for Plasma 6.
#pragma once

#include <QProxyStyle>

class Win9xPlusStyle : public QProxyStyle
{
    Q_OBJECT
public:
    Win9xPlusStyle();

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const override;

    using QProxyStyle::polish;
    using QProxyStyle::unpolish;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};
