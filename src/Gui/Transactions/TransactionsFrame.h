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

#include <QFrame>
#include <QMenu>
#include <QPointer>
#include <QSortFilterProxyModel>

#include "Application/IWalletUiItem.h"

class QLabel;
class QPropertyAnimation;
class QToolButton;

namespace Ui {
class TransactionsFrame;
}

namespace WalletGui {

class CurrentAddressState;

class TransactionsFrame : public QFrame, public IWalletUiItem {
  Q_OBJECT
  Q_DISABLE_COPY(TransactionsFrame)

public:
  explicit TransactionsFrame(QWidget* _parent);
  ~TransactionsFrame();
  QModelIndex index;

  // IWalletUiItem
  virtual void setCryptoNoteAdapter(ICryptoNoteAdapter* _cryptoNoteAdapter) override;
  virtual void setAddressBookManager(IAddressBookManager* _addressBookManager) override;
  virtual void setMainWindow(QWidget* _mainWindow) override;
  virtual void setWalletStateModel(QAbstractItemModel* _model) override;
  virtual void setTransactionsModel(QAbstractItemModel* _model) override;
  virtual void setSortedTransactionsModel(QAbstractItemModel* _model) override;
  virtual void setCurrentAddressState(CurrentAddressState* _currentAddressState) override;
  virtual void updateStyle() override;

public slots:
  void onCustomContextMenu(const QPoint &point);

public Q_SLOTS:
  void copyTxHash();
  void copyAmount();
  void copyPaymentID();
  void showTxDetails();

private:
  QScopedPointer<Ui::TransactionsFrame> m_ui;
  ICryptoNoteAdapter* m_cryptoNoteAdapter;
  IAddressBookManager* m_addressBookManager;
  QWidget* m_mainWindow;
  QAbstractItemModel* m_transactionsModel;
  QAbstractItemModel* m_walletStateModel;
  QAbstractItemModel* m_filterByAgeModel;
  QAbstractItemModel* m_filterByPeriodModel;
  QAbstractItemModel* m_filterByHashModel;
  QAbstractItemModel* m_filterByAddressModel;
  QPropertyAnimation* m_animation;
  QMenu* contextMenu;
  QPointer<CurrentAddressState> m_currentAddressState;
  QLabel* m_scopeStatusLabel;
  QToolButton* m_scopeToggleButton;
  bool m_showAllAddresses;

  void rowsInserted(const QModelIndex& _parent, int _first, int _last);
  void resetFilter();
  void applyAddressScope();

  Q_SLOT void exportToCsv();
  Q_SLOT void transactionDoubleClicked(const QModelIndex& _index);
  Q_SLOT void transactionClicked(const QModelIndex& _index);
  Q_SLOT void filterChanged(int _index);
  Q_SLOT void filterPeriodChanged(const QDateTime& _dateTime);
  Q_SLOT void filterHashChanged(const QString& _hash);
  Q_SLOT void filterAddressChanged(const QString& _hash);
  Q_SLOT void showFilter(bool _on);
  Q_SLOT void resetClicked();
  Q_SLOT void scopeToggled(bool _showAll);
  Q_SLOT void currentAddressChanged(quintptr _index, const QString& _address);
};

}
