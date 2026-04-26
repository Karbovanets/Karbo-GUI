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

#include "AddressSidebar.h"

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "AddressCard.h"
#include "Application/CurrentAddressState.h"
#include "ICryptoNoteAdapter.h"
#include "INodeAdapter.h"
#include "IWalletAdapter.h"
#include "Models/AddressListModel.h"

namespace WalletGui {

AddressSidebar::AddressSidebar(QWidget* _parent) : QFrame(_parent),
  m_scrollArea(nullptr), m_listContainer(nullptr), m_listLayout(nullptr), m_addButton(nullptr),
  m_model(nullptr), m_cryptoNoteAdapter(nullptr) {
  setObjectName("m_addressSidebar");

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_scrollArea->viewport()->setAutoFillBackground(false);

  m_listContainer = new QWidget(m_scrollArea);
  m_listContainer->setObjectName("m_addressSidebarList");
  m_listLayout = new QVBoxLayout(m_listContainer);
  m_listLayout->setContentsMargins(10, 10, 10, 10);
  m_listLayout->setSpacing(8);
  m_listLayout->addStretch();
  m_scrollArea->setWidget(m_listContainer);

  m_addButton = new QPushButton(this);
  m_addButton->setObjectName("m_addAddressButton");
  m_addButton->setText(tr("+ New address"));
  m_addButton->setFlat(true);
  m_addButton->setCursor(Qt::PointingHandCursor);
  m_addButton->setFocusPolicy(Qt::NoFocus);
  connect(m_addButton, &QPushButton::clicked, this, &AddressSidebar::addAddressRequestedSignal);

  root->addWidget(m_scrollArea, 1);
  root->addWidget(m_addButton, 0);
}

AddressSidebar::~AddressSidebar() {
}

void AddressSidebar::setModel(QAbstractItemModel* _model) {
  if (m_model == _model) {
    return;
  }
  if (m_model != nullptr) {
    m_model->disconnect(this);
  }
  m_model = _model;
  if (m_model != nullptr) {
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &AddressSidebar::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &AddressSidebar::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &AddressSidebar::onDataChanged);
    connect(m_model, &QAbstractItemModel::modelReset, this, &AddressSidebar::onModelReset);
  }
  rebuildCards();
}

void AddressSidebar::setCurrentAddressState(CurrentAddressState* _state) {
  if (m_currentAddressState == _state) {
    return;
  }
  if (!m_currentAddressState.isNull()) {
    disconnect(m_currentAddressState.data(), nullptr, this, nullptr);
  }
  m_currentAddressState = _state;
  if (!m_currentAddressState.isNull()) {
    connect(m_currentAddressState.data(), &CurrentAddressState::currentAddressChangedSignal,
            this, &AddressSidebar::onCurrentChanged);
    applySelection();
  }
}

void AddressSidebar::setCryptoNoteAdapter(ICryptoNoteAdapter* _adapter) {
  m_cryptoNoteAdapter = _adapter;
  for (int i = 0; i < m_cards.size(); ++i) {
    updateCardAt(i);
  }
}

void AddressSidebar::refreshAccountNumbers() {
  for (int i = 0; i < m_cards.size(); ++i) {
    fetchAccountNumberFor(i);
  }
}

void AddressSidebar::markRegistrationSent(quintptr _addressIndex) {
  const int row = static_cast<int>(_addressIndex);
  if (row < 0 || row >= m_cards.size()) {
    return;
  }
  m_cards.at(row)->setRegistrationPending(true);
}

void AddressSidebar::clearRegistrationPending() {
  for (AddressCard* card : m_cards) {
    if (card != nullptr) {
      card->setRegistrationPending(false);
    }
  }
}

void AddressSidebar::onRowsInserted(const QModelIndex& _parent, int _first, int _last) {
  if (_parent.isValid() || m_model == nullptr) {
    return;
  }
  for (int row = _first; row <= _last; ++row) {
    AddressCard* card = makeCard(row);
    m_cards.insert(row, card);
    m_listLayout->insertWidget(row, card);
    bindCardSignals(card, row);
  }
  applySelection();
}

void AddressSidebar::onRowsRemoved(const QModelIndex& _parent, int _first, int _last) {
  if (_parent.isValid()) {
    return;
  }
  for (int row = _last; row >= _first; --row) {
    if (row < 0 || row >= m_cards.size()) {
      continue;
    }
    AddressCard* card = m_cards.takeAt(row);
    m_listLayout->removeWidget(card);
    card->deleteLater();
  }
  for (int i = 0; i < m_cards.size(); ++i) {
    bindCardSignals(m_cards[i], i);
    updateCardAt(i);
  }
  applySelection();
}

void AddressSidebar::onDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight, const QVector<int>& _roles) {
  Q_UNUSED(_roles);
  if (_topLeft.parent().isValid()) {
    return;
  }
  for (int row = _topLeft.row(); row <= _bottomRight.row() && row < m_cards.size(); ++row) {
    updateCardAt(row);
  }
}

void AddressSidebar::onModelReset() {
  rebuildCards();
}

void AddressSidebar::onCurrentChanged(quintptr _index, const QString& _address) {
  Q_UNUSED(_index);
  Q_UNUSED(_address);
  applySelection();
}

void AddressSidebar::rebuildCards() {
  for (AddressCard* card : m_cards) {
    m_listLayout->removeWidget(card);
    card->deleteLater();
  }
  m_cards.clear();
  if (m_model == nullptr) {
    return;
  }
  const int rows = m_model->rowCount();
  for (int row = 0; row < rows; ++row) {
    AddressCard* card = makeCard(row);
    m_cards.append(card);
    m_listLayout->insertWidget(row, card);
    bindCardSignals(card, row);
  }
  applySelection();
}

AddressCard* AddressSidebar::makeCard(int _row) {
  AddressCard* card = new AddressCard(m_listContainer);
  updateCardAt(_row);
  return card;
}

void AddressSidebar::updateCardAt(int _row) {
  if (m_model == nullptr || _row < 0 || _row >= m_cards.size()) {
    return;
  }
  AddressCard* card = m_cards.at(_row);
  const QModelIndex addrIdx = m_model->index(_row, AddressListModel::COLUMN_ADDRESS);
  const QString address = m_model->data(addrIdx, AddressListModel::ROLE_ADDRESS).toString();
  const QString label = m_model->data(m_model->index(_row, AddressListModel::COLUMN_LABEL),
                                      AddressListModel::ROLE_LABEL).toString();
  const quint64 unlocked = m_model->data(addrIdx, AddressListModel::ROLE_ACTUAL_BALANCE).toULongLong();
  const quint64 pending = m_model->data(addrIdx, AddressListModel::ROLE_PENDING_BALANCE).toULongLong();
  const quint64 total = m_model->data(addrIdx, AddressListModel::ROLE_TOTAL_BALANCE).toULongLong();

  const QString priorAddress = card->address();
  card->setAddress(address);
  card->setLabel(label);
  card->setIsPrimary(_row == 0);

  // Account-number row gating: every address can register independently; only
  // tracking wallets are excluded since they cannot sign transactions.
  bool canRegister = false;
  if (m_cryptoNoteAdapter != nullptr) {
    INodeAdapter* nodeAdapter = m_cryptoNoteAdapter->getNodeAdapter();
    IWalletAdapter* walletAdapter = nodeAdapter != nullptr ? nodeAdapter->getWalletAdapter() : nullptr;
    canRegister = walletAdapter != nullptr && walletAdapter->isOpen() && !walletAdapter->isTrackingWallet();
  }
  card->setCanRegisterAccountNumber(canRegister);

  // If the card's address actually changed, clear any stale account number so
  // we don't display the previous occupant's alias while we re-fetch.
  if (priorAddress != address) {
    card->setAccountNumber(QString());
  }
  fetchAccountNumberFor(_row);

  const quint64 locked = (total > unlocked) ? (total - unlocked) : 0;
  QString unlockedText;
  QString pendingText;
  QString totalText;
  QString lockedText;
  if (m_cryptoNoteAdapter != nullptr) {
    unlockedText = m_cryptoNoteAdapter->formatUnsignedAmount(unlocked);
    pendingText = m_cryptoNoteAdapter->formatUnsignedAmount(pending);
    totalText = m_cryptoNoteAdapter->formatUnsignedAmount(total);
    lockedText = m_cryptoNoteAdapter->formatUnsignedAmount(locked);
  } else {
    unlockedText = QString::number(unlocked);
    pendingText = QString::number(pending);
    totalText = QString::number(total);
    lockedText = QString::number(locked);
  }
  card->setBalances(unlockedText, unlocked, pendingText, pending, totalText, total, lockedText);
}

void AddressSidebar::bindCardSignals(AddressCard* _card, int _row) {
  disconnect(_card, nullptr, this, nullptr);
  connect(_card, &AddressCard::clickedSignal, this, [this, _row]() {
    if (m_currentAddressState.isNull() || m_model == nullptr || _row >= m_cards.size()) {
      return;
    }
    const QString address = m_cards.at(_row)->address();
    m_currentAddressState->setCurrent(static_cast<quintptr>(_row), address);
  });
  connect(_card, &AddressCard::copyAddressRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT copyAddressRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::copyAccountNumberRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT copyAccountNumberRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->accountNumber());
    }
  });
  connect(_card, &AddressCard::registerAccountNumberRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT registerAccountNumberRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::showQrRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT showQrRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::showKeysRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT showKeysRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::exportTrackingKeyRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT exportTrackingKeyRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::showSeedRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT showSeedRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::renameRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT renameRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::sendFromRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT sendFromRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::balanceProofRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT balanceProofRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
  connect(_card, &AddressCard::deleteRequestedSignal, this, [this, _row]() {
    if (_row < m_cards.size()) {
      Q_EMIT deleteRequestedSignal(static_cast<quintptr>(_row), m_cards.at(_row)->address());
    }
  });
}

void AddressSidebar::fetchAccountNumberFor(int _row) {
  if (m_cryptoNoteAdapter == nullptr || _row < 0 || _row >= m_cards.size()) {
    return;
  }
  INodeAdapter* nodeAdapter = m_cryptoNoteAdapter->getNodeAdapter();
  if (nodeAdapter == nullptr) {
    return;
  }
  AddressCard* card = m_cards.at(_row);
  const QString address = card->address();
  if (address.isEmpty()) {
    return;
  }
  // Guard against the card outliving the sidebar: QPointer goes null if the
  // card is destroyed. The callback fires on a worker thread, so we hop back
  // to the GUI thread via QMetaObject::invokeMethod before touching widgets.
  QPointer<AddressCard> guardedCard(card);
  QPointer<AddressSidebar> self(this);
  nodeAdapter->getAccountNumber(address,
    [guardedCard, self, address](std::error_code _ec, const QString& _accountNumber) {
      const QString result = _ec ? QString() : _accountNumber;
      QMetaObject::invokeMethod(QCoreApplication::instance(),
        [guardedCard, self, address, result]() {
          if (self.isNull() || guardedCard.isNull()) {
            return;
          }
          // Make sure the card still represents the same address; rows can
          // shift between fetch start and reply when addresses are deleted.
          if (guardedCard->address() != address) {
            return;
          }
          guardedCard->setAccountNumber(result);
        }, Qt::QueuedConnection);
    });
}

void AddressSidebar::applySelection() {
  if (m_currentAddressState.isNull()) {
    for (AddressCard* card : m_cards) {
      card->setSelected(false);
    }
    return;
  }
  const QString current = m_currentAddressState->currentAddress();
  for (AddressCard* card : m_cards) {
    card->setSelected(!current.isEmpty() && card->address() == current);
  }
}

}
