// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "win9xplusstyle.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>
#include <QWindow>

// Wrap Qt's own "Windows" style so that everything looks exactly the same,
// except for the parts we override below.
Win9xPlusStyle::Win9xPlusStyle()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Windows")))
{
}

static bool isComboPopup(const QWidget *widget)
{
    return widget && widget->inherits("QComboBoxPrivateContainer");
}

void Win9xPlusStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const
{
    // The fix for the desktop right-click gap:
    //
    // QMenu paints PE_PanelMenu over its whole area first, then the menu
    // items, then the 3D border (PE_FrameMenu). The Windows style draws
    // nothing for PE_PanelMenu, and it leaves a 1px margin (PM_MenuHMargin /
    // PM_MenuVMargin) between the border and the items. In a normal window
    // that strip still looks gray because the window background shows
    // through. Plasma gives its desktop and panel menus a see-through
    // background (Qt::WA_TranslucentBackground), so the strip stays empty
    // and you see whatever is behind the menu.
    //
    // Filling the panel with the same Button color the menu items use
    // closes the gap and doesn't change how ordinary menus look.
    if (element == PE_PanelMenu && !isComboPopup(widget)) {
        painter->fillRect(option->rect, option->palette.brush(QPalette::Button));
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

int Win9xPlusStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                                const QWidget *widget) const
{
    // Submenu placement on X11 (Wayland is handled in eventFilter below).
    // QMenu puts a submenu at
    //   (right edge of the hovered item) + PM_SubMenuOverlap
    // and the Windows style uses -(border width), so with its 2px border
    // and 1px margin the submenu landed 5px on top of the parent menu,
    // covering its 3D border. Offsetting by (border + margin) instead makes
    // the two menus sit edge to edge without overlapping (checked by
    // tests/submenutest.cpp).
    if (metric == PM_SubMenuOverlap) {
        return QProxyStyle::pixelMetric(PM_MenuPanelWidth, option, widget)
             + QProxyStyle::pixelMetric(PM_MenuHMargin, option, widget);
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void Win9xPlusStyle::drawControl(ControlElement element, const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    // QMenu also asks for its leftover space (around and between items) to be
    // painted as CE_MenuEmptyArea, which the Windows style draws nothing for.
    // Fill it too so no see-through gap can appear there.
    if (element == CE_MenuEmptyArea && !isComboPopup(widget)) {
        painter->fillRect(option->rect, option->palette.brush(QPalette::Button));
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Win9xPlusStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (qobject_cast<QMenu *>(widget))
        widget->installEventFilter(this);
}

void Win9xPlusStyle::unpolish(QWidget *widget)
{
    if (qobject_cast<QMenu *>(widget))
        widget->removeEventFilter(this);
    QProxyStyle::unpolish(widget);
}

// The parent menu whose highlighted item opened `menu` as a submenu, if any.
static QMenu *parentMenuOf(QMenu *menu)
{
    QAction *action = menu->menuAction();
    const auto objects = action->associatedObjects();
    for (QObject *object : objects) {
        auto *parent = qobject_cast<QMenu *>(object);
        if (parent && parent != menu && parent->isVisible() && parent->activeAction() == action)
            return parent;
    }
    return nullptr;
}

bool Win9xPlusStyle::eventFilter(QObject *watched, QEvent *event)
{
    // Submenu placement on Wayland.
    //
    // Since Qt 6.10, Qt on Wayland ignores PM_SubMenuOverlap. It asks the
    // compositor to put the submenu's top-left corner at the top-right
    // corner of the hovered item. That item stops (border + margin) pixels
    // short of the menu's outer edge, so the submenu still covered the
    // parent's 3D border.
    //
    // Qt reads a window property, "_q_waylandPopupAnchorRect", when it
    // creates the popup, and uses it instead of the item's rectangle. The
    // show event arrives just before that, so we set it here:
    //  - stretched right to the parent's outer edge, so the menus touch
    //    without overlapping (left edge for right-to-left layouts);
    //  - raised by the submenu's own top border, so its first item lines up
    //    with the hovered item, like Windows 9x and like Qt does on X11.
    // Qt builds that don't support the property simply ignore it.
    if (event->type() == QEvent::Show) {
        auto *menu = qobject_cast<QMenu *>(watched);
        QWindow *window = menu ? menu->windowHandle() : nullptr;
        if (window) {
            QMenu *parent = parentMenuOf(menu);
            if (parent && !menu->actions().isEmpty()) {
                QRect anchor = parent->actionGeometry(menu->menuAction());
                if (menu->isRightToLeft())
                    anchor.setLeft(0);
                else
                    anchor.setRight(parent->width() - 1);
                anchor.translate(0, -menu->actionGeometry(menu->actions().first()).top());
                window->setProperty("_q_waylandPopupAnchorRect", anchor);
            } else {
                // Shown as a normal popup this time: let Qt place it.
                window->setProperty("_q_waylandPopupAnchorRect", QVariant());
            }
        }
    }
    return QProxyStyle::eventFilter(watched, event);
}
