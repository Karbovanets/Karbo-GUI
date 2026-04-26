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

#include "Application/IWalletUiItem.h"
#include "ICryptoNoteAdapter.h"
#include "INodeAdapter.h"

class QLabel;
class QTimer;

namespace Ui {
  class OverviewHeaderFrame;
}

namespace WalletGui {

// Overview status band: two-column snapshot of node and blockchain state.
// Replaces the old balance/pool header — the sidebar + toolbar already show
// balance, and pool volume is niche info that doesn't belong on a dashboard.
class OverviewHeaderFrame : public QFrame, public IWalletUiItem,
  public ICryptoNoteAdapterObserver {
  Q_OBJECT
  Q_DISABLE_COPY(OverviewHeaderFrame)

public:
  explicit OverviewHeaderFrame(QWidget* _parent);
  ~OverviewHeaderFrame();

  // IWalletUiItem
  virtual void setCryptoNoteAdapter(ICryptoNoteAdapter* _cryptoNoteAdapter) override;
  virtual void setMainWindow(QWidget *_mainWindow) override;
  virtual void setNodeStateModel(QAbstractItemModel* _model) override;
  virtual void updateStyle() override;

  // ICryptoNoteAdapterObserver
  Q_SLOT virtual void cryptoNoteAdapterInitCompleted(int _status) override;
  Q_SLOT virtual void cryptoNoteAdapterDeinitCompleted() override;

private:
  QScopedPointer<Ui::OverviewHeaderFrame> m_ui;
  ICryptoNoteAdapter* m_cryptoNoteAdapter;
  QWidget* m_mainWindow;
  QAbstractItemModel* m_nodeStateModel;
  QTimer* m_lastBlockAgeTimer;

  // Network column
  QLabel* m_connectionStateLabel;
  QLabel* m_nodeTypeLabel;
  QLabel* m_peerCountLabel;
  QLabel* m_networkHashrateLabel;

  // Blockchain column
  QLabel* m_heightLabel;
  QLabel* m_lastBlockAgeLabel;
  QLabel* m_difficultyLabel;

  void buildUi();
  void refreshFromModel();
  void refreshLastBlockAge();

  Q_SLOT void nodeStateModelDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                                        const QVector<int>& _roles);
};

}
