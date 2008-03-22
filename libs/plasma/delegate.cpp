/*
    Copyright 2007 Robert Knight <robertknight@gmail.com>
    Copyright 2007 Kevin Ottens <ervin@kde.org>
    Copyright 2008 Marco Martin <notmart@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Own
#include "delegate.h"

#include <cmath>

// Qt
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

// KDE
#include <KColorUtils>
#include <KDebug>
#include <KGlobal>
#include <KGlobalSettings>

// plasma
#include <plasma/plasma.h>

namespace Plasma
{

class Delegate::Private
{
    public:
        Private()
        {
            roles[ColumnTypeRole] = MainColumn;
        }

        ~Private() { }

        QFont fontForSubTitle(const QFont& titleFont) const;
        QRect titleRect(const QStyleOptionViewItem& option, const QModelIndex& index) const;
        QRect subTitleRect(const QStyleOptionViewItem& option, const QModelIndex& index) const;

        QMap<int, int> roles;
};


QFont Delegate::Private::fontForSubTitle(const QFont& titleFont) const
{
    QFont subTitleFont = titleFont;
    subTitleFont.setPointSize(qMax(subTitleFont.pointSize() - 2,
                              KGlobalSettings::smallestReadableFont().pointSize()));
    return subTitleFont;
}

QRect Delegate::Private::titleRect(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QFont font(option.font);
    font.setBold(true);
    QFontMetrics fm(font);

    Qt::Alignment textAlignment = option.decorationAlignment & Qt::AlignRight ? Qt::AlignRight : Qt::AlignLeft;

    QRect emptyRect;
    if (option.direction == Qt::LeftToRight) {
        emptyRect = option.rect.adjusted(ICON_SIZE+ICON_TEXT_MARGIN+ITEM_LEFT_MARGIN, ITEM_TOP_MARGIN, -ITEM_RIGHT_MARGIN, -ITEM_BOTTOM_MARGIN);
    } else {
        emptyRect = option.rect.adjusted(ITEM_LEFT_MARGIN, ITEM_TOP_MARGIN, -ITEM_RIGHT_MARGIN-ICON_SIZE-ICON_TEXT_MARGIN, -ITEM_BOTTOM_MARGIN);
    }

    if (emptyRect.width() < 0) {
        emptyRect.setWidth(0);
        return emptyRect;
    }

    QRect textRect = QStyle::alignedRect(option.direction,
                                         textAlignment,
                                         fm.boundingRect(index.data(Qt::DisplayRole).toString()).size(),
                                         emptyRect);

    textRect.setWidth(textRect.width() + TEXT_RIGHT_MARGIN);
    textRect.setHeight(emptyRect.height()/2);
    return textRect;
}

QRect Delegate::Private::subTitleRect(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QString subTitle = index.data(roles[SubTitleRole]).toString();

    QFontMetrics fm(fontForSubTitle(option.font));

    QRect textRect = titleRect(option, index);
    int right = textRect.right();

    //if title=subtitle subtitle won't be displayed
    if (subTitle != index.data(Qt::DisplayRole).toString()) {
        textRect.setWidth(fm.width("  " + subTitle) + TEXT_RIGHT_MARGIN);
    } else {
        textRect.setWidth(0);
    }
    textRect.translate(0, textRect.height());

    if (option.direction == Qt::RightToLeft) {
        textRect.moveRight(right);
    }

    return textRect;
}

Delegate::Delegate()
    : d(new Private)
{
}

Delegate::~Delegate()
{
    delete d;
}

void Delegate::setRole(SpecificRoles role, int actual)
{
    d->roles[role] = actual;
}

QRect Delegate::rectAfterTitle(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect textRect = d->titleRect(option, index);

    QRect emptyRect(0, textRect.top(), option.rect.width() - textRect.width() - ITEM_LEFT_MARGIN - ITEM_RIGHT_MARGIN - ICON_SIZE - ICON_TEXT_MARGIN, textRect.height());

    if (option.direction == Qt::LeftToRight) {
        emptyRect.moveLeft(textRect.right());
    } else {
        emptyRect.moveRight(textRect.left());
    }

    if (emptyRect.width() < 0) {
        emptyRect.setWidth(0);
    }

    return emptyRect;
}

QRect Delegate::rectAfterSubTitle(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect textRect = d->subTitleRect(option, index);

    QRect emptyRect(0, textRect.top(), option.rect.width() - textRect.width() - ITEM_LEFT_MARGIN - ITEM_RIGHT_MARGIN - ICON_SIZE - ICON_TEXT_MARGIN, textRect.height());

    if (option.direction == Qt::LeftToRight) {
        emptyRect.moveLeft(textRect.right());
    } else {
        emptyRect.moveRight(textRect.left());
    }

    if (emptyRect.width() < 0) {
        emptyRect.setWidth(0);
    }

    return emptyRect;
}

QRect Delegate::emptyRect(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect afterTitleRect = rectAfterTitle(option, index);
    QRect afterSubTitleRect = rectAfterSubTitle(option, index);

    afterTitleRect.setHeight(afterTitleRect.height()*2);
    afterSubTitleRect.setTop(afterTitleRect.top());

    return afterTitleRect.intersected(afterSubTitleRect);
}


void Delegate::paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const bool hover = option.state & (QStyle::State_Selected|QStyle::State_MouseOver|QStyle::State_HasFocus);

    QRect contentRect = option.rect;
    contentRect.setBottom(contentRect.bottom() - 1);

    QRect decorationRect = QStyle::alignedRect(option.direction,
                                               option.decorationPosition == QStyleOptionViewItem::Left ?
                                                                        Qt::AlignLeft : Qt::AlignRight,
                                               option.decorationSize,
                                               contentRect.adjusted(ITEM_LEFT_MARGIN,ITEM_TOP_MARGIN,-ITEM_RIGHT_MARGIN,-ITEM_BOTTOM_MARGIN));


    QString titleText = index.data(Qt::DisplayRole).value<QString>();
    QString subTitleText = index.data(d->roles[SubTitleRole]).value<QString>();

    QRect titleRect = d->titleRect(option, index);
    QRect subTitleRect = d->subTitleRect(option, index);


    bool uniqueTitle = !index.data(SubTitleMandatoryRole).value<bool>();// true;
    if (uniqueTitle) {
        QModelIndex sib = index.sibling(index.row() + 1, index.column());
        if (sib.isValid()) {
            uniqueTitle = sib.data(Qt::DisplayRole).value<QString>() != titleText;
        }

        if (uniqueTitle) {
            sib = index.sibling(index.row() + -1, index.column());
            if (sib.isValid()) {
                uniqueTitle = sib.data(Qt::DisplayRole).value<QString>() != titleText;
            }
        }
    }

    if (subTitleText == titleText) {
        subTitleText = QString();
    }

    QFont subTitleFont = d->fontForSubTitle(option.font);

    QFont titleFont(option.font);

    if (hover) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const int column = index.column();
        const int columns = index.model()->columnCount();
        const int roundedRadius = 5;

        // use a slightly translucent version of the palette's highlight color
        // for the background
        QColor backgroundColor = option.palette.color(QPalette::Highlight);
        backgroundColor.setAlphaF(0.2);

        QColor backgroundColor2 = option.palette.color(QPalette::Highlight);
        backgroundColor.setAlphaF(0.5);

        QRect highlightRect = option.rect.adjusted(2, 2, -2, -2);

        QPen outlinePen(backgroundColor, 2);

        if (column == 0) {
            //clip right (or left for rtl languages) to make the connection with the next column
            if (columns > 1) {
                if (option.direction == Qt::LeftToRight) {
                    painter->setClipRect(option.rect.adjusted(0, 0, ITEM_RIGHT_MARGIN, 0));
                    highlightRect.adjust(0, 0, ITEM_RIGHT_MARGIN+roundedRadius, 0);
                } else {
                    painter->setClipRect(option.rect.adjusted(-ITEM_LEFT_MARGIN, 0, 0, 0));
                    highlightRect.adjust(-ITEM_LEFT_MARGIN-roundedRadius, 0, 0, 0);
                }
            }

            QLinearGradient gradient(highlightRect.topLeft(), highlightRect.topRight());

            //reverse the gradient
            if (option.direction == Qt::RightToLeft) {
                gradient.setStart(highlightRect.topRight());
                gradient.setFinalStop(highlightRect.topLeft());
            }

            gradient.setColorAt(0, backgroundColor);

            gradient.setColorAt(((qreal)titleRect.width()/3.0) / (qreal)highlightRect.width(), backgroundColor2);

            gradient.setColorAt(0.7, backgroundColor);

            outlinePen.setBrush(gradient);

        //last column, clip left (right for rtl)
        } else if (column == columns-1) {
            if (option.direction == Qt::LeftToRight) {
                painter->setClipRect(option.rect.adjusted(-ITEM_LEFT_MARGIN, 0, 0, 0));
                highlightRect.adjust(-ITEM_LEFT_MARGIN-roundedRadius, 0, 0, 0);
            } else {
                painter->setClipRect(option.rect.adjusted(0, 0, ITEM_RIGHT_MARGIN, 0));
                highlightRect.adjust(0, 0, ITEM_RIGHT_MARGIN+roundedRadius, 0);
            }

        //column < columns-1; clip both ways
        } else {
            painter->setClipRect(option.rect.adjusted(-ITEM_LEFT_MARGIN, 0, ITEM_RIGHT_MARGIN, 0));
            highlightRect.adjust(-ITEM_LEFT_MARGIN-roundedRadius, 0, ITEM_RIGHT_MARGIN+roundedRadius, 0);
        }

        painter->setPen(outlinePen);
        painter->drawPath(Plasma::roundedRectangle(highlightRect, roundedRadius));

        painter->restore();
    }

    // draw icon
    QIcon decorationIcon = index.data(Qt::DecorationRole).value<QIcon>();

    if (index.data(d->roles[ColumnTypeRole]).toInt() == MainColumn) {

        if (!hover) {
          decorationRect.adjust(2, 2, -2, -2);
        }

        decorationIcon.paint(painter, decorationRect, option.decorationAlignment);

    //if d->roles[ColumnTypeRole] == SecondaryActionColumn only display the icon on mouse hover
    } else if (hover) {
         const int delta = floor((qreal)(ICON_SIZE - ACTION_ICON_SIZE)/2.0);

         decorationRect.adjust(delta, delta-1, -delta-1, -delta);

         decorationIcon.paint(painter, decorationRect, option.decorationAlignment);
    }

    painter->save();

    // draw title
    if (hover) {
      titleFont.setBold(true);
    }
    painter->setFont(titleFont);
    painter->drawText(titleRect, Qt::AlignLeft|Qt::AlignVCenter, titleText);

    if (hover || !uniqueTitle) {
        // draw sub-title
        painter->setPen(QPen(option.palette.dark(), 1));
        painter->setFont(subTitleFont);
        painter->drawText(subTitleRect, Qt::AlignLeft|Qt::AlignVCenter, "  " + subTitleText);
    }

    painter->restore();

}

QSize Delegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)
    QSize size = option.rect.size();

    QFontMetrics metrics(option.font);

    QFontMetrics subMetrics(d->fontForSubTitle(option.font));
    size.setHeight(qMax(option.decorationSize.height(), qMax(size.height(), metrics.height() + subMetrics.ascent()) + 3) + 4);
//    kDebug() << "size hint is" << size << (metrics.height() + subMetrics.ascent());

    size*=1.1;

    return size;
}

}

