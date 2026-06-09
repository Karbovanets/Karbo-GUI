// Copyright (c) 2015-2017, The Bytecoin developers
// Copyright (c) 2017-2018, The Karbo developers
//
// This file is part of Karbo.
//
// Karbovanets is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Karbovanets is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Karbovanets.  If not, see <http://www.gnu.org/licenses/>.

#include "DonationColumnDelegate.h"

#include <QPainter>
#include <QPixmap>

#include "Models/AddressBookModel.h"


namespace WalletGui {

DonationColumnDelegate::DonationColumnDelegate(QObject* _parent) : QStyledItemDelegate(_parent) {
}

DonationColumnDelegate::~DonationColumnDelegate() {
}

void DonationColumnDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const {
  if (_index.column() == AddressBookModel::COLUMN_DONATION) {
    QStyleOptionViewItem opt(_option);
    initStyleOption(&opt, _index);
    opt.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, _painter, opt.widget);
    const QPixmap pixmap = _index.data(Qt::DecorationRole).value<QPixmap>();
    if (!pixmap.isNull()) {
      const QSize iconSize(20, 20);
      const QRect iconRect(_option.rect.center() - QPoint(iconSize.width() / 2, iconSize.height() / 2), iconSize);
      _painter->drawPixmap(iconRect, pixmap);
    }
    return;
  }

  QStyledItemDelegate::paint(_painter, _option, _index);
}

}
