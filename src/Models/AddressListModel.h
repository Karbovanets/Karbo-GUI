// Copyright (c) 2015-2017, The Bytecoin developers
// Copyright (c) 2017-2026, The Karbo developers
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

#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "ICryptoNoteAdapter.h"
#include "IWalletAdapter.h"
#include "IWalletLabelsManager.h"

namespace WalletGui {

class AddressListModel : public QAbstractListModel, public IWalletAdapterObserver, public ICryptoNoteAdapterObserver,
  public IWalletLabelsManagerObserver {
  Q_OBJECT
  Q_DISABLE_COPY(AddressListModel)
  Q_ENUMS(Columns)

public:
  enum Columns {
    COLUMN_ADDRESS = 0,
    COLUMN_LABEL,
    COLUMN_ACTUAL_BALANCE,
    COLUMN_PENDING_BALANCE,
    COLUMN_TOTAL_BALANCE,
    COLUMN_COUNT
  };

  enum Roles {
    ROLE_ADDRESS = Qt::UserRole,
    ROLE_LABEL,
    ROLE_ACTUAL_BALANCE,
    ROLE_PENDING_BALANCE,
    ROLE_TOTAL_BALANCE,
    ROLE_ADDRESS_INDEX
  };

  AddressListModel(ICryptoNoteAdapter* _cryptoNoteAdapter, IWalletLabelsManager* _labelsManager, QObject* _parent);
  ~AddressListModel();

  // QAbstractListModel
  int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& _parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const override;
  QVariant headerData(int _section, Qt::Orientation _orientation, int _role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& _index) const override;

  QString labelForAddress(const QString& _address) const;
  void setLabelForAddress(const QString& _address, const QString& _label);

  // IWalletAdapterObserver
  Q_SLOT void walletOpened() override;
  Q_SLOT void walletOpenError(int _initStatus) override;
  Q_SLOT void walletClosed() override;
  Q_SLOT void passwordChanged() override;
  Q_SLOT void synchronizationProgressUpdated(quint32 _current, quint32 _total) override;
  Q_SLOT void synchronizationCompleted() override;
  Q_SLOT void balanceUpdated(quint64 _actualBalance, quint64 _pendingBalance) override;
  Q_SLOT void externalTransactionCreated(quintptr _transactionId, const FullTransactionInfo& _transaction) override;
  Q_SLOT void transactionUpdated(quintptr _transactionId, const FullTransactionInfo& _transaction) override;
  void addressCreated(quintptr _addressIndex, const QString& _address) override;
  void addressDeleted(quintptr _addressIndex, const QString& _address) override;

  // ICryptoNoteAdapterObserver
  Q_SLOT void cryptoNoteAdapterInitCompleted(int _status) override;
  Q_SLOT void cryptoNoteAdapterDeinitCompleted() override;

  // IWalletLabelsManagerObserver
  void ownAddressLabelChanged(const QString& _address, const QString& _label) override;

Q_SIGNALS:
  void addressListChangedSignal();

private:
  struct Entry {
    QString address;
    quint64 actualBalance = 0;
    quint64 pendingBalance = 0;
  };

  ICryptoNoteAdapter* m_cryptoNoteAdapter;
  IWalletLabelsManager* m_labelsManager;
  QVector<Entry> m_entries;

  void repopulate();
  void refreshBalances();
  IWalletAdapter* walletAdapter() const;
};

}
