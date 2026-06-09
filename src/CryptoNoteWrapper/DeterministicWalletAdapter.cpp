// Copyright (c) 2016-2017, The Karbovanets developers
//
// This file is part of Karbovanets.
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

#include <cstring>
#include <system_error>

#include "WalletLogger/WalletLogger.h"
#include "CryptoNoteWrapper/DeterministicWalletAdapter.h"
#include "CryptoNoteCore/Account.h"
#include "crypto/crypto.h"

namespace WalletGui {

DeterministicWalletAdapter::DeterministicWalletAdapter(QObject *parent) :
    QObject(parent)
{
}

DeterministicWalletAdapter::~DeterministicWalletAdapter() {
}

AccountKeys DeterministicWalletAdapter::generateDeterministicKeys() {
  AccountKeys keys;
  WalletLogger::info(tr("[Deterministic Wallet Adapter] Generating deterministic keys..."));
  try {
    Crypto::generate_keys(keys.spendKeys.publicKey, keys.spendKeys.secretKey);
    CryptoNote::AccountBase::generateViewFromSpend(keys.spendKeys.secretKey, keys.viewKeys.secretKey, keys.viewKeys.publicKey);
  } catch (std::system_error&) {

  }
  return keys;
}

bool DeterministicWalletAdapter::isDeterministic(AccountKeys& _keys) const {
  if (_keys.spendKeys.secretKey == CryptoNote::NULL_SECRET_KEY) {
    return false;
  }

  Crypto::SecretKey deterministicViewSecretKey;
  CryptoNote::AccountBase::generateViewFromSpend(_keys.spendKeys.secretKey, deterministicViewSecretKey);
  return std::memcmp(deterministicViewSecretKey.data, _keys.viewKeys.secretKey.data, sizeof(Crypto::SecretKey)) == 0;
}

}
