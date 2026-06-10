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

#include "DarkStyle.h"

namespace WalletGui {

namespace {
QPixmap renderIcon(const QString& _path, int _size) {
  return QIcon(_path).pixmap(_size, _size);
}
}

DarkStyle::DarkStyle() : Style("dark", "Dark") {

}

QString DarkStyle::statusBarBackgroundColor() const {
  return "#2E466C";
}

QString DarkStyle::statusBarFontColor() const {
  return "#ffffff";
}

QString DarkStyle::headerBackgroundColor() const {
  return "#2E466C";
}

QString DarkStyle::headerBorderColor() const {
  return "#2E466C";
}

QString DarkStyle::addressFontColor() const {
  return "#ffffff";
}

QString DarkStyle::balanceFontColor() const {
  return "#ffffff";
}

QString DarkStyle::fontColorGray() const {
  return "#AAB6C4";
}

QString DarkStyle::fontColorBlueNormal() const {
  return "#69B7F2";
}

QString DarkStyle::fontColorBlueHover() const {
  return "#8EC9FF";
}

QString DarkStyle::fontColorBluePressed() const {
  return "#4DA3DF";
}

QString DarkStyle::backgroundColorGray() const {
  return "#172235";
}

QString DarkStyle::backgroundButtonColorGrayHover() const {
  return "#2A3A54";
}

QString DarkStyle::backgroundColorAlternate() const {
  return "#202D42";
}

QString DarkStyle::panelBackgroundColor() const {
  return "#1E2A3E";
}

QString DarkStyle::inputBackgroundColor() const {
  return "#111B2B";
}

QString DarkStyle::primaryTextColor() const {
  return "#F1F5FA";
}

QString DarkStyle::hoverBackgroundColor() const {
  return "#283A56";
}

QString DarkStyle::menuBackgroundColor() const {
  return "#1E2A3E";
}

QString DarkStyle::tabBackgroundColor() const {
  return "#1E2A3E";
}

QString DarkStyle::tabInactiveBackgroundColor() const {
  return "#172235";
}

QString DarkStyle::scrollBarBackgroundColor() const {
  return "#101928";
}

QString DarkStyle::calendarHeaderBackgroundColor() const {
  return "#253A5B";
}

QString DarkStyle::borderColor() const {
  return "#344763";
}

QString DarkStyle::borderColorDark() const {
  return "#435673";
}

QString DarkStyle::selectionColor() const {
  return "#3E81B8";
}

QString DarkStyle::glassColor() const {
  return "#dd101827";
}

QString DarkStyle::toolButtonBackgroundColorNormal() const {
  return "#1D3254";
}

QString DarkStyle::toolButtonBackgroundColorHover() const {
  return "#6582AA";
}

QString DarkStyle::toolButtonBackgroundColorPressed() const {
  return "#425A80";
}

QString DarkStyle::toolButtonFontColorNormal() const {
  return "#ffffff";
}

QString DarkStyle::toolButtonFontColorDisabled() const {
  return "#b2ffffff";
}

QString DarkStyle::toolBarBorderColor() const {
  return "#2e4558";
}

QString DarkStyle::getWalletSyncGifFile() const {
  return QString(":icons/dark/wallet-sync");
}

QPixmap DarkStyle::getLogoPixmap() const {
  return QPixmap(QString(":icons/dark/logo"));
}

QPixmap DarkStyle::getBalanceIcon() const {
  return renderIcon(QString(":icons/dark/balance"), 32);
}

QPixmap DarkStyle::getConnectedIcon() const {
  return renderIcon(QString(":icons/dark/connected"), 20);
}

QPixmap DarkStyle::getDisconnectedIcon() const {
  return renderIcon(QString(":icons/dark/disconnected"), 20);
}

QPixmap DarkStyle::getEncryptedIcon() const {
  return renderIcon(QString(":icons/dark/encrypted"), 20);
}

QPixmap DarkStyle::getNotEncryptedIcon() const {
  return renderIcon(QString(":icons/dark/decrypted"), 20);
}

QPixmap DarkStyle::getSyncedIcon() const {
  return renderIcon(QString(":icons/dark/synced"), 20);
}

}
