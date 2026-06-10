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

#include "Style.h"

namespace WalletGui {

class DarkStyle : public Style {
public:
  DarkStyle();

  virtual QString statusBarBackgroundColor() const override;
  virtual QString statusBarFontColor() const override;
  virtual QString headerBackgroundColor() const override;
  virtual QString headerBorderColor() const override;
  virtual QString addressFontColor() const override;
  virtual QString balanceFontColor() const override;
  virtual QString fontColorGray() const override;
  virtual QString fontColorBlueNormal() const override;
  virtual QString fontColorBlueHover() const override;
  virtual QString fontColorBluePressed() const override;
  virtual QString backgroundColorGray() const override;
  virtual QString backgroundButtonColorGrayHover() const override;
  virtual QString backgroundColorAlternate() const override;
  virtual QString panelBackgroundColor() const override;
  virtual QString inputBackgroundColor() const override;
  virtual QString primaryTextColor() const override;
  virtual QString hoverBackgroundColor() const override;
  virtual QString menuBackgroundColor() const override;
  virtual QString tabBackgroundColor() const override;
  virtual QString tabInactiveBackgroundColor() const override;
  virtual QString scrollBarBackgroundColor() const override;
  virtual QString calendarHeaderBackgroundColor() const override;
  virtual QString borderColor() const override;
  virtual QString borderColorDark() const override;
  virtual QString selectionColor() const override;
  virtual QString glassColor() const override;
  virtual QString toolButtonBackgroundColorNormal() const override;
  virtual QString toolButtonBackgroundColorHover() const override;
  virtual QString toolButtonBackgroundColorPressed() const override;
  virtual QString toolButtonFontColorNormal() const override;
  virtual QString toolButtonFontColorDisabled() const override;
  virtual QString toolBarBorderColor() const override;

  virtual QString getWalletSyncGifFile() const override;
  virtual QPixmap getLogoPixmap() const override;
  virtual QPixmap getBalanceIcon() const override;
  virtual QPixmap getConnectedIcon() const override;
  virtual QPixmap getDisconnectedIcon() const override;
  virtual QPixmap getEncryptedIcon() const override;
  virtual QPixmap getNotEncryptedIcon() const override;
  virtual QPixmap getSyncedIcon() const override;
};

}
