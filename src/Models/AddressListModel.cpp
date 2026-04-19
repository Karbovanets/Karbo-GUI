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

#include "AddressListModel.h"
#include "INodeAdapter.h"

namespace WalletGui {

AddressListModel::AddressListModel(ICryptoNoteAdapter* _cryptoNoteAdapter, IWalletLabelsManager* _labelsManager, QObject* _parent) :
  QAbstractListModel(_parent),
  m_cryptoNoteAdapter(_cryptoNoteAdapter),
  m_labelsManager(_labelsManager) {
  m_cryptoNoteAdapter->addObserver(this);
  if (m_labelsManager != nullptr) {
    m_labelsManager->addObserver(this);
  }
  IWalletAdapter* wallet = walletAdapter();
  if (wallet != nullptr) {
    wallet->addObserver(this);
    if (wallet->isOpen()) {
      repopulate();
    }
  }
}

AddressListModel::~AddressListModel() {
}

int AddressListModel::rowCount(const QModelIndex& _parent) const {
  if (_parent.isValid()) {
    return 0;
  }
  return m_entries.size();
}

int AddressListModel::columnCount(const QModelIndex& _parent) const {
  Q_UNUSED(_parent);
  return COLUMN_COUNT;
}

Qt::ItemFlags AddressListModel::flags(const QModelIndex& _index) const {
  if (!_index.isValid()) {
    return Qt::NoItemFlags;
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

QVariant AddressListModel::headerData(int _section, Qt::Orientation _orientation, int _role) const {
  if (_role != Qt::DisplayRole || _orientation != Qt::Horizontal) {
    return QVariant();
  }
  switch (_section) {
  case COLUMN_ADDRESS: return tr("Address");
  case COLUMN_LABEL: return tr("Label");
  case COLUMN_ACTUAL_BALANCE: return tr("Unlocked");
  case COLUMN_PENDING_BALANCE: return tr("Pending");
  case COLUMN_TOTAL_BALANCE: return tr("Total");
  }
  return QVariant();
}

QVariant AddressListModel::data(const QModelIndex& _index, int _role) const {
  if (!_index.isValid() || _index.row() < 0 || _index.row() >= m_entries.size()) {
    return QVariant();
  }

  const Entry& entry = m_entries.at(_index.row());
  const quint64 total = entry.actualBalance + entry.pendingBalance;

  if (_role == Qt::DisplayRole || _role == Qt::EditRole) {
    switch (_index.column()) {
    case COLUMN_ADDRESS: return entry.address;
    case COLUMN_LABEL: return labelForAddress(entry.address);
    case COLUMN_ACTUAL_BALANCE: return m_cryptoNoteAdapter->formatUnsignedAmount(entry.actualBalance);
    case COLUMN_PENDING_BALANCE: return m_cryptoNoteAdapter->formatUnsignedAmount(entry.pendingBalance);
    case COLUMN_TOTAL_BALANCE: return m_cryptoNoteAdapter->formatUnsignedAmount(total);
    }
    return QVariant();
  }

  switch (_role) {
  case ROLE_ADDRESS: return entry.address;
  case ROLE_LABEL: return labelForAddress(entry.address);
  case ROLE_ACTUAL_BALANCE: return entry.actualBalance;
  case ROLE_PENDING_BALANCE: return entry.pendingBalance;
  case ROLE_TOTAL_BALANCE: return total;
  case ROLE_ADDRESS_INDEX: return static_cast<qulonglong>(_index.row());
  }

  return QVariant();
}

QString AddressListModel::labelForAddress(const QString& _address) const {
  if (m_labelsManager == nullptr) {
    return QString();
  }
  return m_labelsManager->getOwnAddressLabel(_address);
}

void AddressListModel::setLabelForAddress(const QString& _address, const QString& _label) {
  if (m_labelsManager == nullptr) {
    return;
  }
  m_labelsManager->setOwnAddressLabel(_address, _label);
}

void AddressListModel::ownAddressLabelChanged(const QString& _address, const QString& _label) {
  Q_UNUSED(_label);
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries.at(i).address == _address) {
      const QModelIndex cell = index(i, COLUMN_LABEL);
      Q_EMIT dataChanged(cell, cell);
      break;
    }
  }
}

void AddressListModel::walletOpened() {
  repopulate();
}

void AddressListModel::walletOpenError(int _initStatus) {
  Q_UNUSED(_initStatus);
}

void AddressListModel::walletClosed() {
  if (m_entries.isEmpty()) {
    return;
  }
  beginResetModel();
  m_entries.clear();
  endResetModel();
  Q_EMIT addressListChangedSignal();
}

void AddressListModel::passwordChanged() {
}

void AddressListModel::synchronizationProgressUpdated(quint32 _current, quint32 _total) {
  Q_UNUSED(_current);
  Q_UNUSED(_total);
}

void AddressListModel::synchronizationCompleted() {
  refreshBalances();
}

void AddressListModel::balanceUpdated(quint64 _actualBalance, quint64 _pendingBalance) {
  Q_UNUSED(_actualBalance);
  Q_UNUSED(_pendingBalance);
  refreshBalances();
}

void AddressListModel::externalTransactionCreated(quintptr _transactionId, const FullTransactionInfo& _transaction) {
  Q_UNUSED(_transactionId);
  Q_UNUSED(_transaction);
  refreshBalances();
}

void AddressListModel::transactionUpdated(quintptr _transactionId, const FullTransactionInfo& _transaction) {
  Q_UNUSED(_transactionId);
  Q_UNUSED(_transaction);
  refreshBalances();
}

void AddressListModel::addressCreated(quintptr _addressIndex, const QString& _address) {
  IWalletAdapter* wallet = walletAdapter();
  if (wallet == nullptr) {
    return;
  }
  const int row = static_cast<int>(_addressIndex);
  if (row < 0 || row > m_entries.size()) {
    repopulate();
    return;
  }
  beginInsertRows(QModelIndex(), row, row);
  Entry entry;
  entry.address = _address;
  entry.actualBalance = wallet->getActualBalance(_addressIndex);
  entry.pendingBalance = wallet->getPendingBalance(_addressIndex);
  m_entries.insert(row, entry);
  endInsertRows();
  Q_EMIT addressListChangedSignal();
}

void AddressListModel::addressDeleted(quintptr _addressIndex, const QString& _address) {
  Q_UNUSED(_address);
  const int row = static_cast<int>(_addressIndex);
  if (row < 0 || row >= m_entries.size()) {
    repopulate();
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  m_entries.remove(row);
  endRemoveRows();
  Q_EMIT addressListChangedSignal();
}

void AddressListModel::cryptoNoteAdapterInitCompleted(int _status) {
  if (_status != 0) {
    return;
  }
  IWalletAdapter* wallet = walletAdapter();
  if (wallet != nullptr) {
    wallet->addObserver(this);
  }
}

void AddressListModel::cryptoNoteAdapterDeinitCompleted() {
}

void AddressListModel::repopulate() {
  IWalletAdapter* wallet = walletAdapter();
  beginResetModel();
  m_entries.clear();
  if (wallet != nullptr && wallet->isOpen()) {
    const quintptr count = wallet->getAddressCount();
    m_entries.reserve(static_cast<int>(count));
    for (quintptr i = 0; i < count; ++i) {
      Entry entry;
      entry.address = wallet->getAddress(i);
      entry.actualBalance = wallet->getActualBalance(i);
      entry.pendingBalance = wallet->getPendingBalance(i);
      m_entries.append(entry);
    }
  }
  endResetModel();
  Q_EMIT addressListChangedSignal();
}

void AddressListModel::refreshBalances() {
  IWalletAdapter* wallet = walletAdapter();
  if (wallet == nullptr || !wallet->isOpen() || m_entries.isEmpty()) {
    return;
  }
  const int rows = m_entries.size();
  for (int i = 0; i < rows; ++i) {
    m_entries[i].actualBalance = wallet->getActualBalance(static_cast<quintptr>(i));
    m_entries[i].pendingBalance = wallet->getPendingBalance(static_cast<quintptr>(i));
  }
  Q_EMIT dataChanged(index(0, COLUMN_ACTUAL_BALANCE),
                     index(rows - 1, COLUMN_TOTAL_BALANCE));
}

IWalletAdapter* AddressListModel::walletAdapter() const {
  INodeAdapter* node = m_cryptoNoteAdapter->getNodeAdapter();
  if (node == nullptr) {
    return nullptr;
  }
  return node->getWalletAdapter();
}

}
