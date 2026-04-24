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

#include "FilteredByHashTransactionsModel.h"
#include "Models/TransactionsModel.h"

Q_DECLARE_METATYPE(QList<CryptoNote::WalletTransfer>)

namespace WalletGui {

FilteredByHashTransactionsModel::FilteredByHashTransactionsModel(QObject* _parent) : QSortFilterProxyModel(_parent),
  m_query() {
  setDynamicSortFilter(true);
}

FilteredByHashTransactionsModel::~FilteredByHashTransactionsModel() {
}

void FilteredByHashTransactionsModel::setFilter(const QString& _query) {
  if (m_query.compare(_query, Qt::CaseInsensitive) == 0) {
    return;
  }
  if (_query.isEmpty()) {
    beginResetModel();
    m_query = _query;
    endResetModel();
  } else {
    m_query = _query;
    invalidateFilter();
  }
}

bool FilteredByHashTransactionsModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const {
  if (m_query.isEmpty()) {
    return true;
  }

  QModelIndex index = sourceModel()->index(_sourceRow, 0, _sourceParent);

  // Hash (hex) prefix match.
  const QString transactionHash = index.data(TransactionsModel::ROLE_HASH).toByteArray().toHex();
  if (transactionHash.startsWith(m_query, Qt::CaseInsensitive)) {
    return true;
  }

  // Payment ID prefix match.
  const QString paymentId = index.data(TransactionsModel::ROLE_PAYMENT_ID).toString();
  if (!paymentId.isEmpty() && paymentId.startsWith(m_query, Qt::CaseInsensitive)) {
    return true;
  }

  // Address substring match across any transfer in this transaction.
  const QList<CryptoNote::WalletTransfer> transfers =
      index.data(TransactionsModel::ROLE_TRANSFERS).value<QList<CryptoNote::WalletTransfer>>();
  for (const CryptoNote::WalletTransfer& transfer : transfers) {
    if (QString::fromStdString(transfer.address).contains(m_query, Qt::CaseInsensitive)) {
      return true;
    }
  }

  return false;
}

}
