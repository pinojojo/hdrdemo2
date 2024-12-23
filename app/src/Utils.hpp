#pragma once

#include <QtWidgets>

namespace utils
{
    inline QIcon createColoredIcon(const QString &iconPath, const QColor &color)
    {
        QIcon icon(iconPath);
        QPixmap pixmap = icon.pixmap(QSize(24, 24));
        QBitmap mask = pixmap.createMaskFromColor(Qt::transparent, Qt::MaskInColor);
        mask.fill(color);
        pixmap.setMask(mask);
        return QIcon(pixmap);
    }

}