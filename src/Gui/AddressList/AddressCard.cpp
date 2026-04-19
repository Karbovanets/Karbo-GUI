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

#include "AddressCard.h"

#include <QAction>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace WalletGui {

namespace {
const int ADDRESS_HEAD = 6;
const int ADDRESS_TAIL = 6;
}

AddressCard::AddressCard(QWidget* _parent) : QFrame(_parent),
  m_labelLabel(nullptr), m_addressLabel(nullptr),
  m_unlockedRow(nullptr), m_pendingRow(nullptr), m_totalRow(nullptr),
  m_copyButton(nullptr), m_qrButton(nullptr), m_advancedButton(nullptr),
  m_advancedMenu(nullptr),
  m_renameAction(nullptr), m_showKeysAction(nullptr), m_sendFromAction(nullptr), m_deleteAction(nullptr),
  m_isPrimary(false), m_isSelected(false) {
  setObjectName("m_addressCard");
  setFrameShape(QFrame::StyledPanel);
  setProperty("selected", false);
  setFocusPolicy(Qt::ClickFocus);
  buildUi();
}

AddressCard::~AddressCard() {
}

QString AddressCard::address() const {
  return m_address;
}

void AddressCard::setAddress(const QString& _address) {
  m_address = _address;
  m_addressLabel->setText(elide(_address));
  m_addressLabel->setToolTip(_address);
}

QString AddressCard::label() const {
  return m_label;
}

void AddressCard::setLabel(const QString& _label) {
  m_label = _label;
  refreshLabelDisplay();
}

void AddressCard::setBalances(const QString& _unlockedFormatted, quint64 _unlockedRaw,
                              const QString& _pendingFormatted, quint64 _pendingRaw,
                              const QString& _totalFormatted) {
  m_unlockedRow->setText(tr("Unlocked: %1").arg(_unlockedFormatted));
  m_unlockedRow->setVisible(_unlockedRaw > 0);
  m_pendingRow->setText(tr("Pending: %1").arg(_pendingFormatted));
  m_pendingRow->setVisible(_pendingRaw > 0);
  m_totalRow->setText(tr("Total: %1").arg(_totalFormatted));
}

bool AddressCard::isPrimary() const {
  return m_isPrimary;
}

void AddressCard::setIsPrimary(bool _primary) {
  m_isPrimary = _primary;
  m_deleteAction->setEnabled(!_primary);
  if (_primary) {
    m_deleteAction->setToolTip(tr("The primary address cannot be deleted"));
  } else {
    m_deleteAction->setToolTip(QString());
  }
  refreshLabelDisplay();
}

bool AddressCard::isSelected() const {
  return m_isSelected;
}

void AddressCard::setSelected(bool _selected) {
  if (m_isSelected == _selected) {
    return;
  }
  m_isSelected = _selected;
  setProperty("selected", _selected);
  style()->unpolish(this);
  style()->polish(this);
  update();
}

void AddressCard::mousePressEvent(QMouseEvent* _event) {
  if (_event->button() == Qt::LeftButton) {
    Q_EMIT clickedSignal();
  }
  QFrame::mousePressEvent(_event);
}

void AddressCard::buildUi() {
  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(4);

  m_labelLabel = new QLabel(this);
  m_labelLabel->setObjectName("m_addressCardLabel");
  m_labelLabel->setTextInteractionFlags(Qt::NoTextInteraction);

  m_addressLabel = new QLabel(this);
  m_addressLabel->setObjectName("m_addressCardAddress");
  m_addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_addressLabel->setCursor(Qt::IBeamCursor);

  m_unlockedRow = new QLabel(this);
  m_unlockedRow->setObjectName("m_addressCardUnlocked");
  m_pendingRow = new QLabel(this);
  m_pendingRow->setObjectName("m_addressCardPending");
  m_totalRow = new QLabel(this);
  m_totalRow->setObjectName("m_addressCardTotal");

  m_copyButton = new QToolButton(this);
  m_copyButton->setObjectName("m_addressCardCopyButton");
  m_copyButton->setText(tr("Copy"));
  m_copyButton->setToolTip(tr("Copy address"));
  m_copyButton->setAutoRaise(true);

  m_qrButton = new QToolButton(this);
  m_qrButton->setObjectName("m_addressCardQrButton");
  m_qrButton->setText(tr("QR"));
  m_qrButton->setToolTip(tr("Show QR code"));
  m_qrButton->setAutoRaise(true);

  m_advancedButton = new QToolButton(this);
  m_advancedButton->setObjectName("m_addressCardAdvancedButton");
  m_advancedButton->setText(tr("Advanced"));
  m_advancedButton->setToolTip(tr("Advanced"));
  m_advancedButton->setAutoRaise(true);
  m_advancedButton->setPopupMode(QToolButton::InstantPopup);

  m_advancedMenu = new QMenu(this);
  m_renameAction = m_advancedMenu->addAction(tr("Rename..."));
  m_showKeysAction = m_advancedMenu->addAction(tr("Show keys..."));
  m_sendFromAction = m_advancedMenu->addAction(tr("Send from this address"));
  m_advancedMenu->addSeparator();
  m_deleteAction = m_advancedMenu->addAction(tr("Delete address"));
  m_advancedButton->setMenu(m_advancedMenu);

  QHBoxLayout* buttonRow = new QHBoxLayout();
  buttonRow->setContentsMargins(0, 0, 0, 0);
  buttonRow->setSpacing(4);
  buttonRow->addWidget(m_copyButton);
  buttonRow->addWidget(m_qrButton);
  buttonRow->addStretch();
  buttonRow->addWidget(m_advancedButton);

  root->addWidget(m_labelLabel);
  root->addWidget(m_addressLabel);
  root->addWidget(m_unlockedRow);
  root->addWidget(m_pendingRow);
  root->addWidget(m_totalRow);
  root->addLayout(buttonRow);

  connect(m_copyButton, &QToolButton::clicked, this, &AddressCard::copyAddressRequestedSignal);
  connect(m_qrButton, &QToolButton::clicked, this, &AddressCard::showQrRequestedSignal);
  connect(m_renameAction, &QAction::triggered, this, &AddressCard::renameRequestedSignal);
  connect(m_showKeysAction, &QAction::triggered, this, &AddressCard::showKeysRequestedSignal);
  connect(m_sendFromAction, &QAction::triggered, this, &AddressCard::sendFromRequestedSignal);
  connect(m_deleteAction, &QAction::triggered, this, &AddressCard::deleteRequestedSignal);
}

void AddressCard::refreshLabelDisplay() {
  if (!m_label.isEmpty()) {
    m_labelLabel->setText(m_label);
  } else if (m_isPrimary) {
    m_labelLabel->setText(tr("Primary address"));
  } else {
    m_labelLabel->setText(tr("Unnamed address"));
  }
}

QString AddressCard::elide(const QString& _address) const {
  if (_address.size() <= ADDRESS_HEAD + ADDRESS_TAIL + 3) {
    return _address;
  }
  return _address.left(ADDRESS_HEAD) + QStringLiteral("…") + _address.right(ADDRESS_TAIL);
}

}
