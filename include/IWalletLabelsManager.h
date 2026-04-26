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

#include <QObject>

namespace WalletGui {

class IWalletLabelsManagerObserver {
public:
  virtual ~IWalletLabelsManagerObserver() {}
  virtual void ownAddressLabelChanged(const QString& _address, const QString& _label) = 0;
};

class IWalletLabelsManager {
public:
  virtual ~IWalletLabelsManager() {}
  virtual QString getOwnAddressLabel(const QString& _address) const = 0;
  virtual void setOwnAddressLabel(const QString& _address, const QString& _label) = 0;
  virtual void addObserver(IWalletLabelsManagerObserver* _observer) = 0;
  virtual void removeObserver(IWalletLabelsManagerObserver* _observer) = 0;
};

}
