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

// The Dolphin/KDE path bar (KUrlNavigator) that contains `widget`, if any.
static const QWidget *urlNavigatorOf(const QWidget *widget)
{
    for (const QWidget *w = widget ? widget->parentWidget() : nullptr; w; w = w->parentWidget()) {
        if (w->inherits("KUrlNavigator"))
            return w;
        if (w->isWindow())
            break;
    }
    return nullptr;
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

    // Dolphin's path bar (KIO's KUrlNavigator):
    //
    // The bar paints a white field with the 9X sunken 2px border
    // (PE_PanelLineEdit). The folder buttons inside it are as tall as the
    // whole bar, border included, and on hover they paint PE_PanelButtonCommand
    // over their full rectangle. With the Windows style that's a solid gray
    // box, so it covered the bar's sunken border. Keep the hover box inside
    // the border instead.
    // KIO keeps the button class in a namespace, and inherits() needs the
    // full name; the plain name is checked too in case that ever changes.
    if (element == PE_PanelButtonCommand && widget
        && (widget->inherits("KDEPrivate::KUrlNavigatorButtonBase") || widget->inherits("KUrlNavigatorButtonBase"))) {
        if (const QWidget *bar = urlNavigatorOf(widget)) {
            const int frame = QProxyStyle::pixelMetric(PM_DefaultFrameWidth, nullptr, bar);
            const QRect inner = bar->rect().adjusted(frame, frame, -frame, -frame);
            const QRect innerInButton(widget->mapFrom(bar, inner.topLeft()), inner.size());
            if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                QStyleOptionButton inset(*button);
                inset.rect = button->rect.intersected(innerInButton);
                if (!inset.rect.isEmpty())
                    QProxyStyle::drawPrimitive(element, &inset, painter, widget);
                return;
            }
        }
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
    //
    // Apps can set the same property themselves. Plasma's system tray does:
    // it sets the anchor to the tray icon, then opens the menu at the
    // screen's top-left corner and relies on the anchor to move it. So we
    // only ever touch an anchor we set ourselves (marked with our own
    // property), and leave any other one alone.
    static const char anchorProperty[] = "_q_waylandPopupAnchorRect";
    static const char ownAnchorProperty[] = "_win9xplus_setPopupAnchorRect";
    if (event->type() == QEvent::Show) {
        auto *menu = qobject_cast<QMenu *>(watched);
        QWindow *window = menu ? menu->windowHandle() : nullptr;
        if (window) {
            const bool ours = window->property(ownAnchorProperty).toBool();
            const bool setByApp = !ours && window->property(anchorProperty).isValid();
            QMenu *parent = parentMenuOf(menu);
            if (parent && !setByApp && !menu->actions().isEmpty()) {
                QRect anchor = parent->actionGeometry(menu->menuAction());
                if (menu->isRightToLeft())
                    anchor.setLeft(0);
                else
                    anchor.setRight(parent->width() - 1);
                anchor.translate(0, -menu->actionGeometry(menu->actions().first()).top());
                window->setProperty(anchorProperty, anchor);
                window->setProperty(ownAnchorProperty, true);
            } else if (!parent && ours) {
                // We placed this menu as a submenu before; now it's opening
                // on its own, so remove our anchor and let Qt place it.
                window->setProperty(anchorProperty, QVariant());
                window->setProperty(ownAnchorProperty, QVariant());
            }
        }
    }
    return QProxyStyle::eventFilter(watched, event);
}
