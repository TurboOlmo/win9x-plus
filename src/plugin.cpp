// SPDX-FileCopyrightText: 2026 The Win9x Plus authors
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "win9xplusstyle.h"

#include <QStylePlugin>

class Win9xPlusStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QStyleFactoryInterface_iid FILE "win9xplus.json")
public:
    QStyle *create(const QString &key) override
    {
        if (key.compare(QLatin1String("win9xplus"), Qt::CaseInsensitive) == 0)
            return new Win9xPlusStyle;
        return nullptr;
    }
};

#include "plugin.moc"
