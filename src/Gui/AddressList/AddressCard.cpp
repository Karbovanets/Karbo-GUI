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
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace WalletGui {


AddressCard::AddressCard(QWidget* _parent) : QFrame(_parent),
  m_labelLabel(nullptr), m_addressLabel(nullptr),
  m_accountNumberRow(nullptr), m_copyAccountNumberButton(nullptr),
  m_availableRow(nullptr), m_lockedRow(nullptr), m_pendingRow(nullptr), m_totalRow(nullptr),
  m_copyButton(nullptr), m_qrButton(nullptr), m_advancedButton(nullptr),
  m_advancedMenu(nullptr),
  m_renameAction(nullptr), m_showKeysAction(nullptr), m_exportTrackingKeyAction(nullptr),
  m_showSeedAction(nullptr), m_sendFromAction(nullptr), m_balanceProofAction(nullptr),
  m_registerAccountNumberAction(nullptr), m_deleteAction(nullptr),
  m_canRegisterAccountNumber(false), m_registrationPending(false),
  m_isPrimary(false), m_isSelected(false) {
  setObjectName("m_addressCard");
  setFrameShape(QFrame::NoFrame);
  setAttribute(Qt::WA_StyledBackground, true);
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
  m_addressLabel->setToolTip(_address);
  updateElidedAddress();
}

QString AddressCard::label() const {
  return m_label;
}

void AddressCard::setLabel(const QString& _label) {
  m_label = _label;
  refreshLabelDisplay();
}

QString AddressCard::accountNumber() const {
  return m_accountNumber;
}

void AddressCard::setAccountNumber(const QString& _accountNumber) {
  if (m_accountNumber == _accountNumber) {
    return;
  }
  m_accountNumber = _accountNumber;
  refreshAccountNumberRow();
}

void AddressCard::setCanRegisterAccountNumber(bool _canRegister) {
  if (m_canRegisterAccountNumber == _canRegister) {
    return;
  }
  m_canRegisterAccountNumber = _canRegister;
  refreshAccountNumberRow();
}

void AddressCard::setRegistrationPending(bool _pending) {
  if (m_registrationPending == _pending) {
    return;
  }
  m_registrationPending = _pending;
  refreshAccountNumberRow();
}

bool AddressCard::isRegistrationPending() const {
  return m_registrationPending;
}

void AddressCard::setBalances(const QString& _unlockedFormatted, quint64 _unlockedRaw,
                              const QString& _pendingFormatted, quint64 _pendingRaw,
                              const QString& _totalFormatted, quint64 _totalRaw,
                              const QString& _lockedFormatted) {
  m_totalRow->setText(tr("Total: %1").arg(_totalFormatted));
  // When some of the total is locked (unspendable right now) show both Available
  // and Locked together so the user doesn't have to do the subtraction mentally.
  // When total == unlocked there's nothing to disambiguate — Total alone suffices.
  const bool hasLocked = _totalRaw > _unlockedRaw;
  m_availableRow->setText(tr("Available: %1").arg(_unlockedFormatted));
  m_availableRow->setVisible(hasLocked);
  m_lockedRow->setText(tr("Locked: %1").arg(_lockedFormatted));
  m_lockedRow->setVisible(hasLocked);
  m_pendingRow->setText(tr("Pending: %1").arg(_pendingFormatted));
  m_pendingRow->setVisible(_pendingRaw > 0);
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

void AddressCard::resizeEvent(QResizeEvent* _event) {
  QFrame::resizeEvent(_event);
  updateElidedAddress();
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
  m_addressLabel->setTextInteractionFlags(Qt::NoTextInteraction);
  m_addressLabel->setCursor(Qt::PointingHandCursor);
  // Address must be allowed to shrink below its natural width so the card can
  // own the available width and we can middle-elide to fit (same UX as the
  // transaction history address column).
  m_addressLabel->setMinimumWidth(0);
  m_addressLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_accountNumberRow = new QLabel(this);
  m_accountNumberRow->setObjectName("m_addressCardAccountNumber");
  m_accountNumberRow->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_accountNumberRow->setCursor(Qt::IBeamCursor);
  m_accountNumberRow->setVisible(false);

  m_copyAccountNumberButton = new QToolButton(this);
  m_copyAccountNumberButton->setObjectName("m_addressCardCopyAccountNumberButton");
  m_copyAccountNumberButton->setText(tr("Copy"));
  m_copyAccountNumberButton->setToolTip(tr("Copy account number"));
  m_copyAccountNumberButton->setAutoRaise(true);
  m_copyAccountNumberButton->setVisible(false);

  m_availableRow = new QLabel(this);
  m_availableRow->setObjectName("m_addressCardAvailable");
  m_lockedRow = new QLabel(this);
  m_lockedRow->setObjectName("m_addressCardLocked");
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
  m_exportTrackingKeyAction = m_advancedMenu->addAction(tr("Export tracking key..."));
  m_showSeedAction = m_advancedMenu->addAction(tr("Show mnemonic seed..."));
  m_sendFromAction = m_advancedMenu->addAction(tr("Send from this address"));
  m_advancedMenu->addSeparator();
  m_balanceProofAction = m_advancedMenu->addAction(tr("Prove balance..."));
  // Register-account-number is hidden by default. refreshAccountNumberRow()
  // reveals it only for the primary address of a full wallet that doesn't
  // yet have a registered account number.
  m_registerAccountNumberAction = m_advancedMenu->addAction(tr("Register account number..."));
  m_registerAccountNumberAction->setVisible(false);
  m_advancedMenu->addSeparator();
  m_deleteAction = m_advancedMenu->addAction(tr("Delete address"));
  m_advancedButton->setMenu(m_advancedMenu);

  QHBoxLayout* accountNumberRow = new QHBoxLayout();
  accountNumberRow->setContentsMargins(0, 0, 0, 0);
  accountNumberRow->setSpacing(4);
  accountNumberRow->addWidget(m_accountNumberRow, 1);
  accountNumberRow->addWidget(m_copyAccountNumberButton);

  QHBoxLayout* buttonRow = new QHBoxLayout();
  buttonRow->setContentsMargins(0, 0, 0, 0);
  buttonRow->setSpacing(4);
  buttonRow->addWidget(m_copyButton);
  buttonRow->addWidget(m_qrButton);
  buttonRow->addStretch();
  buttonRow->addWidget(m_advancedButton);

  root->addWidget(m_labelLabel);
  root->addWidget(m_addressLabel);
  root->addLayout(accountNumberRow);
  root->addWidget(m_totalRow);
  root->addWidget(m_availableRow);
  root->addWidget(m_lockedRow);
  root->addWidget(m_pendingRow);
  root->addLayout(buttonRow);

  connect(m_copyButton, &QToolButton::clicked, this, &AddressCard::copyAddressRequestedSignal);
  connect(m_copyAccountNumberButton, &QToolButton::clicked, this, &AddressCard::copyAccountNumberRequestedSignal);
  connect(m_qrButton, &QToolButton::clicked, this, &AddressCard::showQrRequestedSignal);
  connect(m_renameAction, &QAction::triggered, this, &AddressCard::renameRequestedSignal);
  connect(m_showKeysAction, &QAction::triggered, this, &AddressCard::showKeysRequestedSignal);
  connect(m_exportTrackingKeyAction, &QAction::triggered, this, &AddressCard::exportTrackingKeyRequestedSignal);
  connect(m_showSeedAction, &QAction::triggered, this, &AddressCard::showSeedRequestedSignal);
  connect(m_sendFromAction, &QAction::triggered, this, &AddressCard::sendFromRequestedSignal);
  connect(m_balanceProofAction, &QAction::triggered, this, &AddressCard::balanceProofRequestedSignal);
  connect(m_registerAccountNumberAction, &QAction::triggered, this, &AddressCard::registerAccountNumberRequestedSignal);
  connect(m_deleteAction, &QAction::triggered, this, &AddressCard::deleteRequestedSignal);
}

void AddressCard::refreshAccountNumberRow() {
  const bool hasNumber = !m_accountNumber.isEmpty();
  // Once a number actually arrives from the daemon the pending flag has done
  // its job — clear it so the visibility check below collapses to the simple
  // hasNumber test and the row reverts to the steady-state path.
  if (hasNumber && m_registrationPending) {
    m_registrationPending = false;
  }
  if (hasNumber) {
    m_accountNumberRow->setText(tr("Account #: %1").arg(m_accountNumber));
    m_accountNumberRow->setToolTip(m_accountNumber);
  } else {
    m_accountNumberRow->clear();
    m_accountNumberRow->setToolTip(QString());
  }
  m_accountNumberRow->setVisible(hasNumber);
  m_copyAccountNumberButton->setVisible(hasNumber);

  // Registration is offered for any address that doesn't already have a
  // number AND for which registration is possible (non-tracking wallet) AND
  // for which we don't already have a registration tx in flight. The
  // pending flag prevents users from spamming duplicate registration
  // transactions while the first one is still unconfirmed.
  if (m_registerAccountNumberAction != nullptr) {
    const bool show = !hasNumber && m_canRegisterAccountNumber && !m_registrationPending;
    m_registerAccountNumberAction->setVisible(show);
  }
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

void AddressCard::updateElidedAddress() {
  if (m_addressLabel == nullptr) {
    return;
  }
  if (m_address.isEmpty()) {
    m_addressLabel->clear();
    return;
  }
  // Elide middle to fit the label's current width, matching how addresses are
  // rendered in the transaction history view.
  const int available = m_addressLabel->width();
  if (available <= 0) {
    m_addressLabel->setText(m_address);
    return;
  }
  m_addressLabel->setText(m_addressLabel->fontMetrics().elidedText(m_address, Qt::ElideMiddle, available));
}

}
