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

#pragma once

#include <QFrame>
#include <QPointer>
#include <QVector>

class QAbstractItemModel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace WalletGui {

class AddressCard;
class CurrentAddressState;
class ICryptoNoteAdapter;

class AddressSidebar : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(AddressSidebar)

public:
  explicit AddressSidebar(QWidget* _parent = nullptr);
  ~AddressSidebar();

  void setModel(QAbstractItemModel* _model);
  void setCurrentAddressState(CurrentAddressState* _state);
  void setCryptoNoteAdapter(ICryptoNoteAdapter* _adapter);

Q_SIGNALS:
  void addAddressRequestedSignal();
  void copyAddressRequestedSignal(quintptr _addressIndex, const QString& _address);
  void showQrRequestedSignal(quintptr _addressIndex, const QString& _address);
  void showKeysRequestedSignal(quintptr _addressIndex, const QString& _address);
  void exportTrackingKeyRequestedSignal(quintptr _addressIndex, const QString& _address);
  void showSeedRequestedSignal(quintptr _addressIndex, const QString& _address);
  void renameRequestedSignal(quintptr _addressIndex, const QString& _address);
  void sendFromRequestedSignal(quintptr _addressIndex, const QString& _address);
  void balanceProofRequestedSignal(quintptr _addressIndex, const QString& _address);
  void deleteRequestedSignal(quintptr _addressIndex, const QString& _address);

private Q_SLOTS:
  void onRowsInserted(const QModelIndex& _parent, int _first, int _last);
  void onRowsRemoved(const QModelIndex& _parent, int _first, int _last);
  void onDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight, const QVector<int>& _roles);
  void onModelReset();
  void onCurrentChanged(quintptr _index, const QString& _address);

private:
  QScrollArea* m_scrollArea;
  QWidget* m_listContainer;
  QVBoxLayout* m_listLayout;
  QPushButton* m_addButton;
  QAbstractItemModel* m_model;
  QPointer<CurrentAddressState> m_currentAddressState;
  ICryptoNoteAdapter* m_cryptoNoteAdapter;
  QVector<AddressCard*> m_cards;

  void rebuildCards();
  void updateCardAt(int _row);
  AddressCard* makeCard(int _row);
  void bindCardSignals(AddressCard* _card, int _row);
  void applySelection();
};

}
