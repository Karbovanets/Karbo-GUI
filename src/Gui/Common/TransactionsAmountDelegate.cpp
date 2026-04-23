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

#include <QLinearGradient>
#include <QPainter>

#include "TransactionsAmountDelegate.h"
#include "Settings/Settings.h"
#include "Models/TransactionsModel.h"
#include "Style/Style.h"

namespace WalletGui {

namespace {

const int AMOUNT_DECIMAL_COUNT = 3;

QColor getAmountTextColor(const QModelIndex& _index, const QStyleOptionViewItem& _option) {
  const Style& style = Settings::instance().getCurrentStyle();
  if (_option.state & QStyle::State_Selected) {
    return QColor(style.selectionTextColor());
  }

  const QColor foregroundColor = _index.data(Qt::ForegroundRole).value<QColor>();
  if (foregroundColor.isValid()) {
    return foregroundColor;
  }

  return QColor(style.primaryTextColor());
}

QColor getAmountFadeColor(const QStyleOptionViewItem& _option) {
  const Style& style = Settings::instance().getCurrentStyle();
  if (_option.state & QStyle::State_Selected) {
    return QColor(style.selectionColor());
  }

  if (_option.state & QStyle::State_MouseOver) {
    return QColor(style.hoverBackgroundColor());
  }

  return _option.features & QStyleOptionViewItem::Alternate ? QColor(style.panelBackgroundColor()) :
    QColor(style.backgroundColorAlternate());
}

}

TransactionsAmountDelegate::TransactionsAmountDelegate(bool _hideLongAmounts, QObject* _parent) : QStyledItemDelegate(_parent),
  m_hideLongAmounts(_hideLongAmounts) {
}

TransactionsAmountDelegate::~TransactionsAmountDelegate() {
}

void TransactionsAmountDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const {
  if (!_index.isValid()) {
    QStyledItemDelegate::paint(_painter, _option, _index);
    return;
  }

  _painter->save();
  _painter->setRenderHints(QPainter::Antialiasing);
  QStyleOptionViewItem opt(_option);
  initStyleOption(&opt, _index);
  opt.decorationPosition = QStyleOptionViewItem::Right;

  opt.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, _painter, opt.widget);
  if (_index.data(Qt::DecorationRole).isValid()) {
    QRect iconRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    opt.widget->style()->drawItemPixmap(_painter, iconRect, _option.decorationAlignment, _index.data(Qt::DecorationRole).value<QPixmap>());
  }

  QString amountString = _index.data().toString();
  bool isAmountTruncated = false;
  if (m_hideLongAmounts) {
    int dotPos = amountString.indexOf('.');
    int decimalDigitNumber = amountString.length() - 1 - dotPos;
    if (decimalDigitNumber < AMOUNT_DECIMAL_COUNT) {
      amountString.append(QString().fill('0', AMOUNT_DECIMAL_COUNT - decimalDigitNumber));
    }

    if (decimalDigitNumber > AMOUNT_DECIMAL_COUNT && ((opt.state & QStyle::State_MouseOver) == 0)) {
      amountString = amountString.left(amountString.length() - decimalDigitNumber + AMOUNT_DECIMAL_COUNT);
      isAmountTruncated = true;
    }
  }

  QRect textRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
  _painter->setFont(opt.font);
  const QColor amountTextColor = getAmountTextColor(_index, opt);
  if (isAmountTruncated) {
    QLinearGradient amountGradient(0, 0, 1, 0);
    amountGradient.setCoordinateMode(QLinearGradient::ObjectBoundingMode);
    amountGradient.setColorAt(0, amountTextColor);
    amountGradient.setColorAt(0.7, amountTextColor);
    amountGradient.setColorAt(1, getAmountFadeColor(opt));
    _painter->setPen(QPen(amountGradient, 0));
    _painter->drawText(textRect, static_cast<int>(opt.displayAlignment) | Qt::TextSingleLine, amountString);
  } else {
    _painter->setPen(amountTextColor);
    opt.widget->style()->drawItemText(_painter, textRect, opt.displayAlignment, opt.palette, opt.state & QStyle::State_Enabled, amountString);
  }

  _painter->restore();
}

}
