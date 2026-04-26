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

#include "AddressCard.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace WalletGui {


AddressCard::AddressCard(QWidget* _parent) : QFrame(_parent),
  m_labelLabel(nullptr), m_addressLabel(nullptr),
  m_accountNumberCaption(nullptr), m_accountNumberValueLabel(nullptr),
  m_availableCaption(nullptr), m_lockedCaption(nullptr), m_pendingCaption(nullptr), m_totalCaption(nullptr),
  m_availableRow(nullptr), m_lockedRow(nullptr), m_pendingRow(nullptr), m_totalRow(nullptr),
  m_advancedButton(nullptr),
  m_advancedMenu(nullptr),
  m_copyAddressAction(nullptr), m_showQrAction(nullptr), m_copyAccountNumberAction(nullptr),
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
  m_totalRow->setText(_totalFormatted);
  // When some of the total is locked (unspendable right now) show both Available
  // and Locked together so the user doesn't have to do the subtraction mentally.
  // When total == unlocked there's nothing to disambiguate — Total alone suffices.
  const bool hasLocked = _totalRaw > _unlockedRaw;
  m_availableRow->setText(_unlockedFormatted);
  m_availableCaption->setVisible(hasLocked);
  m_availableRow->setVisible(hasLocked);
  m_lockedRow->setText(_lockedFormatted);
  m_lockedCaption->setVisible(hasLocked);
  m_lockedRow->setVisible(hasLocked);
  m_pendingRow->setText(_pendingFormatted);
  const bool hasPending = _pendingRaw > 0;
  m_pendingCaption->setVisible(hasPending);
  m_pendingRow->setVisible(hasPending);
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
  // Float the dropdown button in the top-right corner of the card. We use
  // a small inset (4px) from the edge so it sits visually inside the card
  // rounded corner rather than punching out of it. The button isn't part
  // of any layout, so it doesn't push other content around.
  if (m_advancedButton != nullptr) {
    const int inset = 4;
    const int x = width() - m_advancedButton->width() - inset;
    const int y = inset;
    m_advancedButton->move(qMax(0, x), y);
  }
}

void AddressCard::buildUi() {
  QVBoxLayout* root = new QVBoxLayout(this);
  // Tight bottom padding because the advanced button no longer occupies its
  // own row at the bottom of the card — it floats in the top-right corner
  // (see resizeEvent), so the layout doesn't need to reserve space for it.
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(4);

  // Small label showing the user's name for the address (or "Primary
  // address"/"Unnamed address" fallback). Captioning style — not selectable.
  // Right-side contents margin reserves room for the floating dropdown
  // button (positioned in resizeEvent) so long labels don't slide under it.
  m_labelLabel = new QLabel(this);
  m_labelLabel->setObjectName("m_addressCardLabel");
  m_labelLabel->setTextInteractionFlags(Qt::NoTextInteraction);
  m_labelLabel->setContentsMargins(0, 0, 28, 0);

  // The address is shown elided-middle to fit the card's width. We
  // intentionally make it NON-selectable: selecting the elided text would
  // copy only the visible (truncated) portion, which is a footgun — users
  // think they've copied the full address but they've actually copied
  // something useless. The canonical full-address copy is the "Copy
  // address" action in the cog/right-click menu (also reachable by
  // right-clicking anywhere on the card body).
  m_addressLabel = new QLabel(this);
  m_addressLabel->setObjectName("m_addressCardAddress");
  m_addressLabel->setTextInteractionFlags(Qt::NoTextInteraction);
  // Right-clicks bubble up to the card's contextMenuEvent so the cog menu
  // opens regardless of where on the address the user clicks.
  m_addressLabel->setContextMenuPolicy(Qt::NoContextMenu);
  // Address must be allowed to shrink below its natural width so the card can
  // own the available width and we can middle-elide to fit (same UX as the
  // transaction history address column).
  m_addressLabel->setMinimumWidth(0);
  m_addressLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  // Account number row: small "Account #" caption (matches the size of
  // the address-name caption above) followed by a larger monospace value.
  // Hidden until a number
  // is actually registered for this address.
  QHBoxLayout* accountNumberRow = new QHBoxLayout();
  accountNumberRow->setContentsMargins(0, 0, 0, 0);
  accountNumberRow->setSpacing(4);

  m_accountNumberCaption = new QLabel(this);
  m_accountNumberCaption->setObjectName("m_addressCardAccountNumberCaption");
  m_accountNumberCaption->setTextInteractionFlags(Qt::NoTextInteraction);
  m_accountNumberCaption->setText(tr("Account #"));
  m_accountNumberCaption->setVisible(false);

  m_accountNumberValueLabel = new QLabel(this);
  m_accountNumberValueLabel->setObjectName("m_addressCardAccountNumberValue");
  m_accountNumberValueLabel->setTextInteractionFlags(Qt::NoTextInteraction /*Qt::TextSelectableByMouse*/);
  m_accountNumberValueLabel->setCursor(Qt::IBeamCursor);
  m_accountNumberValueLabel->setContextMenuPolicy(Qt::NoContextMenu);
  m_accountNumberValueLabel->setVisible(false);
  // Font (family / size / weight / color) is controlled from the global
  // stylesheet — see the "#m_addressCardAccountNumberValue" rule in
  // src/karbowanecwallet.qss. Edit it there to change how the account
  // number renders.

  // Balance rows: tiny caption on the left, current-size selectable amount
  // on the right so users can highlight + Ctrl+C any formatted value.
  auto makeBalanceCaption = [this](const QString& text) {
    QLabel* label = new QLabel(text, this);
    label->setObjectName("m_addressCardBalanceCaption");
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    return label;
  };

  auto makeAmountRow = [this](const char* objectName) {
    QLabel* label = new QLabel(this);
    label->setObjectName(QString::fromLatin1(objectName));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setCursor(Qt::IBeamCursor);
    // Right-click on any amount also opens the cog menu — consistent with
    // the address label. Selection + Ctrl+C still works.
    label->setContextMenuPolicy(Qt::NoContextMenu);
    return label;
  };
  auto makeBalanceRow = [](QLabel* caption, QLabel* value) {
    QHBoxLayout* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    row->addWidget(caption);
    row->addWidget(value);
    row->addStretch();
    return row;
  };
  m_totalCaption = makeBalanceCaption(tr("Total:"));
  m_availableCaption = makeBalanceCaption(tr("Available:"));
  m_lockedCaption = makeBalanceCaption(tr("Locked:"));
  m_pendingCaption = makeBalanceCaption(tr("Pending:"));
  m_availableRow = makeAmountRow("m_addressCardAvailable");
  m_lockedRow = makeAmountRow("m_addressCardLocked");
  m_pendingRow = makeAmountRow("m_addressCardPending");
  m_totalRow = makeAmountRow("m_addressCardTotal");
  m_availableCaption->setVisible(false);
  m_availableRow->setVisible(false);
  m_lockedCaption->setVisible(false);
  m_lockedRow->setVisible(false);
  m_pendingCaption->setVisible(false);
  m_pendingRow->setVisible(false);

  // Compact dropdown affordance floating in the card's top-right corner.
  // It replaces the old Copy/QR/Advanced button trio. The same menu is
  // surfaced via right-click anywhere on the card body (see
  // contextMenuEvent), so this button is just a visible hint that the
  // card has actions — it shouldn't steal vertical space. We use the
  // shared :icons/down-arrow resource (also used by Qt combo-boxes
  // throughout the app) for a familiar dropdown-arrow look, and size
  // both the icon and the button down to feel like a chrome accent
  // rather than a primary control.
  m_advancedButton = new QToolButton(this);
  m_advancedButton->setObjectName("m_addressCardAdvancedButton");
  m_advancedButton->setIcon(QIcon(QStringLiteral(":icons/down-arrow")));
  m_advancedButton->setIconSize(QSize(10, 10));
  m_advancedButton->setFixedSize(20, 20);
  m_advancedButton->setToolTip(tr("Address actions"));
  m_advancedButton->setAutoRaise(true);
  m_advancedButton->setPopupMode(QToolButton::InstantPopup);
  m_advancedButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_advancedButton->setFocusPolicy(Qt::NoFocus);
  // Floats above the card content rather than taking a layout slot.
  // Position is updated in resizeEvent. raise() is called below after all
  // sibling widgets are constructed so the button stacks on top of them.

  m_advancedMenu = new QMenu(this);
  m_copyAddressAction = m_advancedMenu->addAction(tr("Copy address"));
  m_showQrAction = m_advancedMenu->addAction(tr("Show QR code"));
  // Visible only when this address has an account number — toggled in
  // refreshAccountNumberRow().
  m_copyAccountNumberAction = m_advancedMenu->addAction(tr("Copy account number"));
  m_copyAccountNumberAction->setVisible(false);
  m_advancedMenu->addSeparator();
  m_renameAction = m_advancedMenu->addAction(tr("Rename..."));
  m_showKeysAction = m_advancedMenu->addAction(tr("Show keys..."));
  m_exportTrackingKeyAction = m_advancedMenu->addAction(tr("Export tracking key..."));
  m_showSeedAction = m_advancedMenu->addAction(tr("Show mnemonic seed..."));
  m_advancedMenu->addSeparator();
  m_sendFromAction = m_advancedMenu->addAction(tr("Send from this address"));
  m_balanceProofAction = m_advancedMenu->addAction(tr("Prove balance..."));
  // Register-account-number is hidden by default. refreshAccountNumberRow()
  // reveals it for non-tracking wallets that don't yet have a registered
  // account number AND don't have a registration tx in flight.
  m_registerAccountNumberAction = m_advancedMenu->addAction(tr("Register account number..."));
  m_registerAccountNumberAction->setVisible(false);
  m_advancedMenu->addSeparator();
  m_deleteAction = m_advancedMenu->addAction(tr("Delete address"));
  m_advancedButton->setMenu(m_advancedMenu);

  root->addWidget(m_labelLabel);
  root->addWidget(m_addressLabel);
  accountNumberRow->addWidget(m_accountNumberCaption);
  accountNumberRow->addWidget(m_accountNumberValueLabel);
  accountNumberRow->addStretch();
  root->addLayout(accountNumberRow);
  root->addLayout(makeBalanceRow(m_totalCaption, m_totalRow));
  root->addLayout(makeBalanceRow(m_availableCaption, m_availableRow));
  root->addLayout(makeBalanceRow(m_lockedCaption, m_lockedRow));
  root->addLayout(makeBalanceRow(m_pendingCaption, m_pendingRow));

  // Make sure the floating cog/dropdown button stays on top of the labels
  // it overlaps in the top-right corner. Without this it can end up
  // beneath the address-name label and become unclickable.
  m_advancedButton->raise();

  connect(m_copyAddressAction, &QAction::triggered, this, &AddressCard::copyAddressRequestedSignal);
  connect(m_showQrAction, &QAction::triggered, this, &AddressCard::showQrRequestedSignal);
  connect(m_copyAccountNumberAction, &QAction::triggered, this, &AddressCard::copyAccountNumberRequestedSignal);
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
    m_accountNumberValueLabel->setText(m_accountNumber);
    m_accountNumberValueLabel->setToolTip(m_accountNumber);
  } else {
    m_accountNumberValueLabel->clear();
    m_accountNumberValueLabel->setToolTip(QString());
  }
  m_accountNumberCaption->setVisible(hasNumber);
  m_accountNumberValueLabel->setVisible(hasNumber);

  if (m_copyAccountNumberAction != nullptr) {
    m_copyAccountNumberAction->setVisible(hasNumber);
  }

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

void AddressCard::contextMenuEvent(QContextMenuEvent* _event) {
  // Right-click anywhere on the card body opens the same per-address menu
  // that the cog button shows. Selection on selectable text labels still
  // works — Qt sends contextMenuEvent only when the click isn't already
  // handled by the label's own context menu policy. We override at the
  // frame level so the menu is consistent regardless of where the click
  // lands. Clicking the address (no matter how badly elided) → the menu's
  // "Copy address" action copies the FULL address.
  if (m_advancedMenu != nullptr) {
    m_advancedMenu->exec(_event->globalPos());
    _event->accept();
    return;
  }
  QFrame::contextMenuEvent(_event);
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
