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

#include "CurrentAddressState.h"

namespace WalletGui {

CurrentAddressState::CurrentAddressState(QObject* _parent) : QObject(_parent), m_index(static_cast<quintptr>(-1)) {
}

CurrentAddressState::~CurrentAddressState() {
}

QString CurrentAddressState::currentAddress() const {
  return m_address;
}

quintptr CurrentAddressState::currentAddressIndex() const {
  return m_index;
}

void CurrentAddressState::setCurrent(quintptr _index, const QString& _address) {
  if (m_index == _index && m_address == _address) {
    return;
  }
  m_index = _index;
  m_address = _address;
  Q_EMIT currentAddressChangedSignal(m_index, m_address);
}

void CurrentAddressState::clear() {
  if (m_address.isEmpty() && m_index == static_cast<quintptr>(-1)) {
    return;
  }
  m_index = static_cast<quintptr>(-1);
  m_address.clear();
  Q_EMIT currentAddressChangedSignal(m_index, m_address);
}

}
