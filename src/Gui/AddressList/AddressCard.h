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

class QAction;
class QLabel;
class QToolButton;
class QMenu;

namespace WalletGui {

class AddressCard : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(AddressCard)
  Q_PROPERTY(bool selected READ isSelected WRITE setSelected)

public:
  explicit AddressCard(QWidget* _parent = nullptr);
  ~AddressCard();

  QString address() const;
  void setAddress(const QString& _address);

  QString label() const;
  void setLabel(const QString& _label);

  void setBalances(const QString& _unlockedFormatted, quint64 _unlockedRaw,
                   const QString& _pendingFormatted, quint64 _pendingRaw,
                   const QString& _totalFormatted);

  bool isPrimary() const;
  void setIsPrimary(bool _primary);

  bool isSelected() const;
  void setSelected(bool _selected);

Q_SIGNALS:
  void clickedSignal();
  void copyAddressRequestedSignal();
  void showQrRequestedSignal();
  void showKeysRequestedSignal();
  void exportTrackingKeyRequestedSignal();
  void showSeedRequestedSignal();
  void renameRequestedSignal();
  void sendFromRequestedSignal();
  void balanceProofRequestedSignal();
  void deleteRequestedSignal();

protected:
  void mousePressEvent(QMouseEvent* _event) override;

private:
  QLabel* m_labelLabel;
  QLabel* m_addressLabel;
  QLabel* m_unlockedRow;
  QLabel* m_pendingRow;
  QLabel* m_totalRow;
  QToolButton* m_copyButton;
  QToolButton* m_qrButton;
  QToolButton* m_advancedButton;
  QMenu* m_advancedMenu;
  QAction* m_renameAction;
  QAction* m_showKeysAction;
  QAction* m_exportTrackingKeyAction;
  QAction* m_showSeedAction;
  QAction* m_sendFromAction;
  QAction* m_balanceProofAction;
  QAction* m_deleteAction;

  QString m_address;
  QString m_label;
  bool m_isPrimary;
  bool m_isSelected;

  void buildUi();
  void refreshLabelDisplay();
  QString elide(const QString& _address) const;
};

}
