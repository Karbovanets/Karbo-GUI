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

#include <cstring>

#include <QApplication>
#include <QGuiApplication>
#include <QActionGroup>
#include <QEnterEvent>
#include <QInputDialog>
#include <QLinearGradient>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPushButton>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaMethod>
#include <QSessionManager>
#include <QSystemTrayIcon>
#include <QToolBar>
#include <QUrlQuery>
#include <QVBoxLayout>

#include <Common/Base58.h>
#include "MainWindow.h"
#include "Settings/Settings.h"
#include "WalletLogger/WalletLogger.h"
#include "Gui/AddressBook/AddressBookFrame.h"
#include "Gui/Common/AboutDialog.h"
#include "Gui/Common/ChangePasswordDialog.h"
#include "Gui/Common/NewPasswordDialog.h"
#include "Gui/Common/KeyDialog.h"
#include "Gui/Common/QuestionDialog.h"
#include "Gui/Common/QRCodeDialog.h"
#include "Gui/Common/MnemonicDialog.h"
#include "Gui/Common/OpenUriDialog.h"
#include "Gui/Common/SignMessageDialog.h"
#include "Gui/Common/BalanceProofDialog.h"
#include "ICryptoNoteAdapter.h"
#include "INodeAdapter.h"
#include "IWalletAdapter.h"
#include "Application/CurrentAddressState.h"
#include "Gui/AddressList/AddressSidebar.h"
#include "IWalletLabelsManager.h"
#include "Models/AddressBookModel.h"
#include "Models/AddressListModel.h"
#include "Models/BlockchainModel.h"
#include "Models/NodeStateModel.h"
#include "Models/SortedAddressBookModel.h"
#include "Models/SortedTransactionsModel.h"
#include "Models/TransactionsModel.h"
#include "Models/TransactionPoolModel.h"
#include "Models/WalletStateModel.h"
#include "Gui/Options/OptionsDialog.h"
#include "Style/Style.h"
#include "Gui/Common/RestoreFromMnemonicSeedDialog.h"
#include "Mnemonics/electrum-words.h"
#include "CryptoNote.h"
#include "CryptoNoteCore/Account.h"
#include "crypto/crypto.h"
#include "../include/IDonationManager.h"
#include "Application/IWalletUiItem.h"
#include "Application/WalletApplication.h"
#include "Gui/Common/CopyMagicLabel.h"
#include "Gui/Common/WalletBlueButton.h"
#include "Gui/Common/WalletCancelButton.h"
#include "Gui/Common/WalletDescriptionLabel.h"
#include "Gui/Common/WalletGrayCheckBox.h"
#include "Gui/Common/WalletLinkLikeButton.h"
#include "Gui/Common/WalletNavigationButton.h"
#include "Gui/Common/WalletOkButton.h"
#include "Gui/Common/WalletTableView.h"
#include "Gui/Common/WalletTextLabel.h"
#include "Gui/Common/WalletTreeView.h"
#include "ui_MainWindow.h"

namespace WalletGui {

namespace {

// QLabel subclass for the toolbar Total/Locked balances. Default state
// shows a truncated 4-decimal form (so a long fractional balance can't
// blow up the toolbar width); on mouseover the full-precision form
// replaces it in-place. When the truncated form is on screen the label
// paints its own text with a left-to-right gradient that fades the last
// ~30% into the toolbar background — same UX as the transactions
// history amount column (TransactionsAmountDelegate).
//
// Holds both strings itself so the model-update path doesn't have to
// poke dynamic properties or run an external event filter.
class FadingAmountLabel : public QLabel {
public:
  explicit FadingAmountLabel(QWidget* _parent = nullptr) : QLabel(_parent) {
    setTextInteractionFlags(Qt::NoTextInteraction);
  }

  void setSelectableWhenExpanded(bool _selectable) {
    m_selectableWhenExpanded = _selectable;
    updateTextInteraction();
  }

  void setAmount(const QString& _shortText, const QString& _fullText) {
    m_shortText = _shortText;
    m_fullText = _fullText;
    // Truncation actually happened (and so the fade is meaningful) only
    // when the two forms differ. If they match — e.g. the balance has 4
    // or fewer decimals already — render plainly.
    m_truncated = (_shortText != _fullText);
    setText(m_hovered ? m_fullText : m_shortText);
    updateTextInteraction();
    update();
  }

protected:
  void enterEvent(QEnterEvent* _event) override {
    m_hovered = true;
    if (!m_fullText.isEmpty()) {
      setText(m_fullText);
    }
    updateTextInteraction();
    QLabel::enterEvent(_event);
    update();
  }

  void leaveEvent(QEvent* _event) override {
    if (m_contextMenuOpen) {
      QLabel::leaveEvent(_event);
      return;
    }

    m_hovered = false;
    if (!m_shortText.isEmpty()) {
      setText(m_shortText);
    }
    updateTextInteraction();
    QLabel::leaveEvent(_event);
    update();
  }

  void contextMenuEvent(QContextMenuEvent* _event) override {
    if (!m_selectableWhenExpanded) {
      QLabel::contextMenuEvent(_event);
      return;
    }

    const QString selected = selectedText();
    m_hovered = true;
    if (!m_fullText.isEmpty() && text() != m_fullText) {
      setText(m_fullText);
    }
    updateTextInteraction();
    update();

    QMenu menu(this);
    QAction* copyAction = menu.addAction(tr("Copy"));
    QAction* selectAllAction = menu.addAction(tr("Select All"));

    m_contextMenuOpen = true;
    QAction* chosenAction = menu.exec(_event->globalPos());
    m_contextMenuOpen = false;

    if (chosenAction == copyAction) {
      QApplication::clipboard()->setText(selected.isEmpty() ? m_fullText : selected);
    } else if (chosenAction == selectAllAction) {
      setSelection(0, text().length());
    }

    if (!underMouse() && chosenAction != selectAllAction) {
      m_hovered = false;
      if (!m_shortText.isEmpty()) {
        setText(m_shortText);
      }
      updateTextInteraction();
      update();
    }

    _event->accept();
  }

  void paintEvent(QPaintEvent* _event) override {
    // Hover or non-truncated cases get the stock QLabel render so we
    // inherit QSS-driven foreground color, font, alignment, etc. as-is.
    if (m_hovered || !m_truncated) {
      QLabel::paintEvent(_event);
      return;
    }

    // Truncated + not hovered: paint the text with a gradient pen so the
    // rightmost characters fade into the toolbar background, signaling
    // "more digits hidden — hover to see them all". The fade target is
    // the active theme's toolbar background (toolButtonBackgroundColorNormal),
    // which is what the transparent toolbar frame is sitting on.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& style = Settings::instance().getCurrentStyle();
    const QColor textColor = palette().color(foregroundRole());
    const QColor fadeColor(style.toolButtonBackgroundColorNormal());

    QLinearGradient gradient(0, 0, 1, 0);
    gradient.setCoordinateMode(QLinearGradient::ObjectBoundingMode);
    gradient.setColorAt(0.0, textColor);
    gradient.setColorAt(0.7, textColor);
    gradient.setColorAt(1.0, fadeColor);

    painter.setPen(QPen(gradient, 0));
    painter.setFont(font());
    painter.drawText(contentsRect(),
                     static_cast<int>(alignment()) | Qt::TextSingleLine,
                     text());
  }

private:
  void updateTextInteraction() {
    const bool selectable = m_selectableWhenExpanded && m_hovered;
    const Qt::TextInteractionFlags flags = selectable ? Qt::TextSelectableByMouse : Qt::NoTextInteraction;
    setTextInteractionFlags(flags);
    setCursor(selectable ? Qt::IBeamCursor : Qt::ArrowCursor);
  }

  QString m_shortText;
  QString m_fullText;
  bool m_truncated = false;
  bool m_hovered = false;
  bool m_selectableWhenExpanded = false;
  bool m_contextMenuOpen = false;
};

const int MAX_RECENT_WALLET_COUNT = 10;
const char COMMUNITY_FORUM_URL[] = "https://forum.karbo.io";
const char REPORT_ISSUE_URL[] = "https://karbo.org";

const char DONATION_URL_DONATION_TAG[] = "donation";
const char DONATION_URL_LABEL_TAG[] = "label";
const char DONATION_ADDRESS[] = "Kdev1L9V5ow3cdKNqDpLcFFxZCqu5W2GE9xMKewsB2pUXWxcXvJaUWHcSrHuZw91eYfQFzRtGfTemReSSMN4kE445i6Etb3";

QByteArray convertAccountKeysToByteArray(const AccountKeys& _accountKeys) {
  QByteArray spendPublicKey(reinterpret_cast<const char*>(&_accountKeys.spendKeys.publicKey), sizeof(Crypto::PublicKey));
  QByteArray viewPublicKey(reinterpret_cast<const char*>(&_accountKeys.viewKeys.publicKey), sizeof(Crypto::PublicKey));
  QByteArray spendPrivateKey(reinterpret_cast<const char*>(&_accountKeys.spendKeys.secretKey), sizeof(Crypto::SecretKey));
  QByteArray viewPrivateKey(reinterpret_cast<const char*>(&_accountKeys.viewKeys.secretKey), sizeof(Crypto::SecretKey));
  QByteArray trackingKeys;
  trackingKeys.append(spendPublicKey).append(viewPublicKey).append(spendPrivateKey).append(viewPrivateKey);
  return trackingKeys;
}

AccountKeys convertByteArrayToAccountKeys(const QByteArray& _array) {
  AccountKeys accountKeys;
  QDataStream trackingKeysDataStream(_array);
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.spendKeys.publicKey), sizeof(Crypto::PublicKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.viewKeys.publicKey), sizeof(Crypto::PublicKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.spendKeys.secretKey), sizeof(Crypto::SecretKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.viewKeys.secretKey), sizeof(Crypto::SecretKey));
  return accountKeys;
}

bool isDonationUrl(const QUrl& _url) {
  QUrlQuery urlQuery(_url);
  if(!urlQuery.hasQueryItem(DONATION_URL_DONATION_TAG)) {
    return false;
  }

  return (urlQuery.queryItemValue(DONATION_URL_DONATION_TAG).compare("true", Qt::CaseInsensitive) == 0);
}

template<typename Widget>
void updateChildrenOfType(QWidget* _root) {
  const QList<Widget*> children = _root->findChildren<Widget*>();
  for (Widget* child : children) {
    child->updateStyle();
  }
}

}

MainWindow::MainWindow(ICryptoNoteAdapter* _cryptoNoteAdapter, IAddressBookManager* _addressBookManager,
  IDonationManager* _donationManager,
  IApplicationEventHandler* _applicationEventHandler, const QString& _styleSheetTemplate, QWidget* _parent) :
  QMainWindow(_parent), m_ui(new Ui::MainWindow), m_cryptoNoteAdapter(_cryptoNoteAdapter),
  m_addressBookManager(_addressBookManager), m_donationManager(_donationManager),
  m_applicationEventHandler(_applicationEventHandler),
  m_blockChainModel(nullptr), m_transactionPoolModel(nullptr), m_recentWalletsMenu(new QMenu(this)),
  m_addRecipientAction(new QAction(this)), m_styleSheetTemplate(_styleSheetTemplate),
  m_addressListModel(nullptr), m_currentAddressState(nullptr), m_addressSidebar(nullptr),
  m_mainToolBar(nullptr), m_navActionGroup(nullptr),
  m_overviewNavAction(nullptr), m_sendNavAction(nullptr), m_receiveNavAction(nullptr),
  m_historyNavAction(nullptr), m_contactsNavAction(nullptr), m_explorerNavAction(nullptr),
  m_menuBarBalanceLabel(nullptr),
  m_sidebarTotalLabel(nullptr), m_sidebarLockedLabel(nullptr),
  m_createAddressAction(nullptr), m_importAddressAction(nullptr) {
  m_ui->setupUi(this);
  setWindowTitle(tr("Karbo Spring Wallet %1").arg(Settings::instance().getVersion()));
  m_addRecipientAction->setObjectName("m_addRecipientAction");
  m_cryptoNoteAdapter->addObserver(this);
  m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->addObserver(this);
  m_applicationEventHandler->addObserver(this);
  addActions(QList<QAction*>() << m_ui->m_createWalletAction << m_ui->m_openWalletAction);

  m_nodeStateModel = new NodeStateModel(m_cryptoNoteAdapter, this);
  m_walletStateModel = new WalletStateModel(m_cryptoNoteAdapter, this);
  m_transactionsModel = new TransactionsModel(m_cryptoNoteAdapter, m_nodeStateModel, this);
  m_sortedTranactionsModel = new SortedTransactionsModel(m_transactionsModel, this);
  m_addressBookModel = new AddressBookModel(m_addressBookManager, this);
  m_sortedAddressBookModel = new SortedAddressBookModel(m_addressBookModel, this);
  m_blockChainModel = new BlockchainModel(m_cryptoNoteAdapter, m_nodeStateModel, this);
  m_transactionPoolModel = new TransactionPoolModel(m_cryptoNoteAdapter, this);

  IWalletLabelsManager* labelsManager = dynamic_cast<IWalletLabelsManager*>(m_addressBookManager);
  m_addressListModel = new AddressListModel(m_cryptoNoteAdapter, labelsManager, this);
  m_currentAddressState = new CurrentAddressState(this);

  buildTopNavToolBar();
  buildAddressSidebar();
  installSidebarBalance();
  rearrangeWalletMenu();

  QList<IWalletUiItem*> uiItems;
  uiItems << m_ui->m_noWalletFrame << m_ui->m_overviewFrame << m_ui->m_sendFrame << m_ui->m_transactionsFrame <<
    m_ui->m_receiveFrame << m_ui->m_addressBookFrame << m_ui->m_blockExplorerFrame << m_ui->statusBar;
  for (auto& uiItem : uiItems) {
    uiItem->setCryptoNoteAdapter(m_cryptoNoteAdapter);
    uiItem->setAddressBookManager(m_addressBookManager);
    uiItem->setDonationManager(m_donationManager);
    uiItem->setApplicationEventHandler(m_applicationEventHandler);
    uiItem->setMainWindow(this);
    uiItem->setNodeStateModel(m_nodeStateModel);
    uiItem->setWalletStateModel(m_walletStateModel);
    uiItem->setTransactionsModel(m_transactionsModel);
    uiItem->setSortedTransactionsModel(m_sortedTranactionsModel);
    uiItem->setAddressBookModel(m_addressBookModel);
    uiItem->setSortedAddressBookModel(m_sortedAddressBookModel);
    uiItem->setBlockChainModel(m_blockChainModel);
    uiItem->setTransactionPoolModel(m_transactionPoolModel);
    uiItem->setAddressListModel(m_addressListModel);
    uiItem->setCurrentAddressState(m_currentAddressState);
  }

  if (!Settings::instance().isSystemTrayAvailable() && QSystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_minimizeToTrayAction->deleteLater();
    m_ui->m_closeToTrayAction->deleteLater();
  } else {
    m_ui->m_minimizeToTrayAction->setChecked(Settings::instance().isMinimizeToTrayEnabled());
    m_ui->m_closeToTrayAction->setChecked(Settings::instance().isCloseToTrayEnabled());
  }

  createRecentWalletMenu();

  m_ui->m_enableBlockchainExplorerAction->setChecked(Settings::instance().isBlockchainExplorerEnabled());

  setClosedState();

 /*!
  * \brief Open the wallet if it exists or create a new one.
  *
  * When application starts, the wallet file which is set in settings is opened here
  * if it exists, or if wallet doesn't exist, we create a new wallet.
  */
  if (QFile::exists(Settings::instance().getWalletFile())) {
    m_ui->m_noWalletFrame->openWallet(Settings::instance().getWalletFile(), QString());
  } else {
 /*!
  * This is the original behavior:
  * m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->create(Settings::instance().getWalletFile(), "");
  *
  * Instead of silent creation of the new wallet in default location we just show the Welcome Screen,
  * and let users create wallet themselves where they want or open existing wallet.
  * At least they will know it's location...
  *
  * createWallet();
  */
  }

  QActionGroup* themeActionGroup = new QActionGroup(this);
  quintptr styleCount = Settings::instance().getStyleCount();
  for (quintptr i = 0; i < styleCount; ++i) {
    const Style& style = Settings::instance().getStyle(i);
    QAction* styleAction = m_ui->menuThemes->addAction(style.getStyleName());
    styleAction->setData(style.getStyleId());
    styleAction->setCheckable(true);
    if (style.getStyleId() == Settings::instance().getCurrentTheme()) {
      styleAction->setChecked(true);
    }

    themeActionGroup->addAction(styleAction);
    connect(styleAction, &QAction::triggered, this, &MainWindow::themeChanged);
  }
  connect(m_walletStateModel, &QAbstractItemModel::dataChanged, this, &MainWindow::walletStateModelDataChanged);
  connect(m_addRecipientAction, &QAction::triggered, this, &MainWindow::addRecipientTriggered);
  connect(m_ui->m_exitAction, &QAction::triggered, qApp, &QApplication::quit);
  connect(qApp, &QGuiApplication::commitDataRequest, this, &MainWindow::commitData);
  connect(m_walletStateModel, SIGNAL(synchronizationCompletedSignal()), this, SLOT(synchronizationCompleted()));
  connect(m_walletStateModel, SIGNAL(balanceUpdatedSignal(quint64, quint64)), this, SLOT(balanceUpdated(quint64, quint64)));
}

MainWindow::~MainWindow() {
}

bool MainWindow::eventFilter(QObject* _object, QEvent* _event) {
  Q_UNUSED(_object);
  Q_UNUSED(_event);
  // Toolbar Total/Locked hover behavior moved to FadingAmountLabel, which
  // handles enter/leave/paint itself. Legacy header address/balance
  // labels were click-to-copy; both have moved into the sidebar and
  // toolbar, which wire up their own click handlers.
  return false;
}

void MainWindow::walletOpened() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  setOpenedState();
  // Reset the per-card "registration in flight" suppression so a fresh
  // session never inherits stale UI state from whatever was open before.
  if (m_addressSidebar != nullptr) {
    m_addressSidebar->clearRegistrationPending();
  }
  QStringList recentWalletList = Settings::instance().getRecentWalletList();
  recentWalletList.removeAll(Settings::instance().getWalletFile());
  recentWalletList.prepend(Settings::instance().getWalletFile());
  while (recentWalletList.size() > MAX_RECENT_WALLET_COUNT) {
    recentWalletList.removeLast();
  }

  Settings::instance().setRecentWalletList(recentWalletList);
  updateRecentWalletActions();
  if (walletAdapter->isTrackingWallet()) {
    if (m_sendNavAction != nullptr) m_sendNavAction->setEnabled(false);
    if (m_contactsNavAction != nullptr) m_contactsNavAction->setEnabled(false);
    m_ui->m_openPaymentRequestAction->setEnabled(false);
    m_ui->m_exportKeyAction->setEnabled(false);
  }
  AccountKeys accountKeys = walletAdapter->getAccountKeys(0);
  const bool hasHdSeed = walletAdapter->getAddressGenerationMode() == CryptoNote::AddressGenerationMode::HD_DETERMINISTIC;
  if (!hasHdSeed && !m_deterministicAdapter.isDeterministic(accountKeys)) {
    m_ui->m_showSeedAction->setEnabled(false);
  }

  QUrl url = m_applicationEventHandler->getLastReceivedUrl();
  if (url.isValid()) {
    urlReceived(url);
  }

  setDevDonation();

  m_ui->m_receiveFrame->walletOpened(walletAdapter->getAddress(0));

  if (m_currentAddressState != nullptr && m_currentAddressState->currentAddress().isEmpty()) {
    m_currentAddressState->setCurrent(0, walletAdapter->getAddress(0));
  }
}

void MainWindow::walletOpenError(int _initStatus) {
  if (_initStatus != IWalletAdapter::INIT_SUCCESS) {
    setClosedState();
  }
}

void MainWindow::walletClosed() {
  setClosedState();
  if (m_currentAddressState != nullptr) {
    m_currentAddressState->clear();
  }
  if (m_addressSidebar != nullptr) {
    m_addressSidebar->clearRegistrationPending();
  }
}

void MainWindow::passwordChanged() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  m_ui->m_changePasswordAction->setEnabled(walletAdapter->isEncrypted());
  m_ui->m_encryptWalletAction->setEnabled(!walletAdapter->isEncrypted());
  walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
}

void MainWindow::synchronizationProgressUpdated(quint32 _current, quint32 _total) {
  if (_total < _current) {
    return;
  }

  m_ui->m_getBalanceProofAction->setEnabled(false);

  qreal value = static_cast<qreal>(_current) / _total;
  m_ui->m_syncProgress->setValue(value * m_ui->m_syncProgress->maximum());
}

void MainWindow::synchronizationCompleted() {
  m_ui->m_syncProgress->setValue(m_ui->m_syncProgress->maximum());
  if (m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getActualBalance() > 0) {
    m_ui->m_getBalanceProofAction->setEnabled(true);
  }
  // Now that the chain is up to date, ask the daemon for each address's
  // registered account number alias (if any) and propagate to the cards.
  if (m_addressSidebar != nullptr) {
    m_addressSidebar->refreshAccountNumbers();
  }
}

void MainWindow::balanceUpdated(quint64 _actualBalance, quint64 _pendingBalance) {
  if (_actualBalance == 0) {
    m_ui->m_getBalanceProofAction->setEnabled(false);
  }
}

void MainWindow::externalTransactionCreated(quintptr _transactionId, const FullTransactionInfo& _transaction) {
  QApplication::alert(this);
}

void MainWindow::transactionUpdated(quintptr _transactionId, const FullTransactionInfo& _transaction) {
  // Do nothing
}

void MainWindow::urlReceived(const QUrl& _url) {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (!walletAdapter->isOpen()) {
    return;
  }

  showNormal();
  activateWindow();
  raise();
  if (isDonationUrl(_url)) {
    QUrlQuery urlQuery(_url);
    QString address = _url.path();
    QString paymentid = urlQuery.queryItemValue("payment_id");
    QString label = urlQuery.queryItemValue(DONATION_URL_LABEL_TAG);
    m_addressBookManager->addAddress(label, address, paymentid, true);

    OptionsDialog dlg(m_cryptoNoteAdapter, m_donationManager, m_addressBookModel, this);
    dlg.setDonationAddress(label, address);
    dlg.exec();
  } else if (_url.isValid()) {
    if (m_sendNavAction != nullptr) m_sendNavAction->setChecked(true);
  }
}

void MainWindow::screenLocked() {
  // Do nothing
}

void MainWindow::screenUnlocked() {
  // Do nothing
}

void MainWindow::cryptoNoteAdapterInitCompleted(int _status) {
  setEnabled(true);
  if (_status == 0) {
    m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->addObserver(this);
    if (QFile::exists(Settings::instance().getWalletFile())) {
      m_ui->m_noWalletFrame->openWallet(Settings::instance().getWalletFile(), QString());
    }
  }
}

void MainWindow::cryptoNoteAdapterDeinitCompleted() {

}

void MainWindow::changeEvent(QEvent* _event) {
  QMainWindow::changeEvent(_event);
  switch (_event->type()) {
  case QEvent::WindowStateChange:
    if(isMinimized() && Settings::instance().isMinimizeToTrayEnabled()) {
      hide();
    }

    break;
  default:
    break;
  }

  QMainWindow::changeEvent(_event);
}

void MainWindow::closeEvent(QCloseEvent* _event) {
#ifndef Q_OS_MAC
  if (!Settings::instance().isCloseToTrayEnabled()) {
    m_ui->m_exitAction->trigger();
  }
#endif
  QMainWindow::closeEvent(_event);
}

void MainWindow::setOpenedState() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  m_ui->m_backupWalletAction->setEnabled(true);
  m_ui->m_resetAction->setEnabled(true);
  m_ui->m_closeWalletAction->setEnabled(true);
  m_ui->m_exportTrackingKeyAction->setEnabled(true);
  m_ui->m_exportKeyAction->setEnabled(true);
  m_ui->m_saveKeysAction->setEnabled(true);
  m_ui->m_showSeedAction->setEnabled(true);
  m_ui->m_encryptWalletAction->setEnabled(!walletAdapter->isEncrypted());
  m_ui->m_changePasswordAction->setEnabled(walletAdapter->isEncrypted());
  m_ui->m_openPaymentRequestAction->setEnabled(true);
  m_ui->m_createPaymentRequestAction->setEnabled(true);
  m_ui->m_signMessageAction->setEnabled(true);
  m_ui->m_verifyMessageAction->setEnabled(true);

  m_ui->m_noWalletFrame->hide();
  m_ui->m_overviewFrame->show();

  if (m_overviewNavAction != nullptr) m_overviewNavAction->setEnabled(true);
  if (m_sendNavAction != nullptr) m_sendNavAction->setEnabled(true);
  if (m_receiveNavAction != nullptr) m_receiveNavAction->setEnabled(true);
  if (m_historyNavAction != nullptr) m_historyNavAction->setEnabled(true);
  if (m_contactsNavAction != nullptr) m_contactsNavAction->setEnabled(true);
  if (m_explorerNavAction != nullptr) {
    m_explorerNavAction->setEnabled(m_cryptoNoteAdapter->getNodeAdapter()->getBlockChainExplorerAdapter() != nullptr);
  }
  if (m_overviewNavAction != nullptr) m_overviewNavAction->setChecked(true);

  // The blockchain-explorer toggle used to be restricted to local/embedded nodes,
  // but ProxyRpcNodeWorker also wires up a BlockChainExplorerAdapter when the
  // setting is on, so the feature works over remote RPC too. Always allow toggling.
  m_ui->m_enableBlockchainExplorerAction->setEnabled(true);

  if (m_createAddressAction != nullptr) m_createAddressAction->setEnabled(true);
  if (m_importAddressAction != nullptr) m_importAddressAction->setEnabled(true);
}

void MainWindow::setClosedState() {
  if (m_overviewNavAction != nullptr) {
    m_overviewNavAction->setChecked(false);
    m_overviewNavAction->setEnabled(false);
  }
  if (m_sendNavAction != nullptr) m_sendNavAction->setEnabled(false);
  if (m_receiveNavAction != nullptr) m_receiveNavAction->setEnabled(false);
  if (m_historyNavAction != nullptr) m_historyNavAction->setEnabled(false);
  if (m_contactsNavAction != nullptr) m_contactsNavAction->setEnabled(false);
  if (m_explorerNavAction != nullptr) m_explorerNavAction->setEnabled(false);

  m_ui->m_backupWalletAction->setEnabled(false);
  m_ui->m_resetAction->setEnabled(false);
  m_ui->m_closeWalletAction->setEnabled(false);
  m_ui->m_exportTrackingKeyAction->setEnabled(false);
  m_ui->m_exportKeyAction->setEnabled(false);
  m_ui->m_saveKeysAction->setEnabled(false);
  m_ui->m_encryptWalletAction->setEnabled(false);
  m_ui->m_changePasswordAction->setEnabled(false);
  m_ui->m_showSeedAction->setEnabled(false);
  m_ui->m_openPaymentRequestAction->setEnabled(false);
  m_ui->m_createPaymentRequestAction->setEnabled(false);
  m_ui->m_signMessageAction->setEnabled(false);
  m_ui->m_verifyMessageAction->setEnabled(false);
  m_ui->m_getBalanceProofAction->setEnabled(false);

  if (m_createAddressAction != nullptr) m_createAddressAction->setEnabled(false);
  if (m_importAddressAction != nullptr) m_importAddressAction->setEnabled(false);

  m_ui->m_overviewFrame->hide();
  m_ui->m_sendFrame->hide();
  m_ui->m_transactionsFrame->hide();
  m_ui->m_addressBookFrame->hide();
  m_ui->m_blockExplorerFrame->hide();
  m_ui->m_receiveFrame->hide();
  m_ui->m_noWalletFrame->show();
  m_ui->m_syncProgress->setValue(0);

  m_ui->m_receiveFrame->walletClosed();
}

void MainWindow::addRecipientTriggered() {
  RecepientPair recepient_pair = m_addRecipientAction->data().value<RecepientPair>();
  m_ui->m_sendFrame->addRecipient(recepient_pair);
  if (m_sendNavAction != nullptr) m_sendNavAction->setChecked(true);
}

void MainWindow::commitData(QSessionManager& _manager) {
  WalletLogger::debug(tr("[Main window] Commit data request"));
  if (m_cryptoNoteAdapter->getNodeAdapter() != nullptr) {
    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    if (walletAdapter->isOpen()) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
    }
  }
}

void MainWindow::walletStateModelDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight, const QVector<int>& _roles) {
  Q_UNUSED(_bottomRight);
  Q_UNUSED(_roles);
  if (_topLeft.column() == WalletStateModel::COLUMN_ABOUT_TO_BE_SYNCHRONIZED) {
    const bool walletAboutToBeSynchronized = _topLeft.data().toBool();
    m_ui->m_getBalanceProofAction->setEnabled(!walletAboutToBeSynchronized);
  }
}

void MainWindow::createRecentWalletMenu() {
  m_ui->m_recentWalletsAction->setMenu(m_recentWalletsMenu);
  for (quint32 i = 0; i < MAX_RECENT_WALLET_COUNT; ++i) {
    QAction* action = new QAction(this);
    action->setVisible(false);
    m_recentWalletsActionList.append(action);
    m_recentWalletsMenu->addAction(action);
    connect(action, &QAction::triggered, this, &MainWindow::openRecentWallet);
  }

  updateRecentWalletActions();
}

void MainWindow::updateRecentWalletActions() {
  QStringList recentWallets = Settings::instance().getRecentWalletList();
  int recentWalletCount = qMin(recentWallets.size(), MAX_RECENT_WALLET_COUNT);
  for (int i = 0; i < recentWalletCount; ++i) {
    m_recentWalletsActionList[i]->setText(recentWallets[i]);
    m_recentWalletsActionList[i]->setData(recentWallets[i]);
    m_recentWalletsActionList[i]->setVisible(true);
  }

  for (int i = recentWalletCount; i < MAX_RECENT_WALLET_COUNT; ++i) {
    m_recentWalletsActionList[i]->setVisible(false);
  }
}

void MainWindow::openRecentWallet() {
  QAction* action = qobject_cast<QAction*>(sender());
  if (action == nullptr) {
    return;
  }

  QString filePath = action->data().toString();
  if (!filePath.isEmpty()) {
    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    if (walletAdapter->isOpen()) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }

    m_ui->m_noWalletFrame->openWallet(filePath, QString());
  }
}

void MainWindow::themeChanged() {
  QAction* styleAction = qobject_cast<QAction*>(sender());
  Settings::instance().setCurrentTheme(styleAction->data().toString());
  if (WalletApplication* application = qobject_cast<WalletApplication*>(qApp)) {
    application->applyCurrentTheme();
  }

  qApp->setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(m_styleSheetTemplate));
  QList<IWalletUiItem*> uiItems;
  uiItems << m_ui->m_noWalletFrame << m_ui->m_overviewFrame << m_ui->m_sendFrame << m_ui->m_transactionsFrame <<
    m_ui->m_blockExplorerFrame <<  m_ui->m_receiveFrame << m_ui->m_addressBookFrame << m_ui->statusBar;
  for (auto& uiItem : uiItems) {
    uiItem->updateStyle();
  }

  updateThemedWidgets();
}

void MainWindow::updateThemedWidgets() {
  updateChildrenOfType<CopyMagicLabel>(this);
  updateChildrenOfType<WalletBlueButton>(this);
  updateChildrenOfType<WalletCancelButton>(this);
  updateChildrenOfType<WalletDescriptionLabel>(this);
  updateChildrenOfType<WalletGrayCheckBox>(this);
  updateChildrenOfType<WalletLinkLikeButton>(this);
  updateChildrenOfType<WalletNavigationButton>(this);
  updateChildrenOfType<WalletOkButton>(this);
  updateChildrenOfType<WalletTableView>(this);
  updateChildrenOfType<WalletTextLabel>(this);
  updateChildrenOfType<WalletTreeView>(this);
}

// Creates a wallet whose additional addresses have independent random spend keys.
void MainWindow::createNonDeterministicWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)")
    );

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (QFile::exists(filePath)) {
    QMessageBox::warning(this, tr("Warning"),
      tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
    return;
  }

  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (!filePath.isEmpty()) {
    if (walletAdapter->isOpen()) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }

    QString oldWalletFile = Settings::instance().getWalletFile();
    Settings::instance().setWalletFile(filePath);
    if (walletAdapter->create(filePath, "") == IWalletAdapter::INIT_SUCCESS) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
    } else {
      Settings::instance().setWalletFile(oldWalletFile);
    }
  }
}

void MainWindow::createWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)")
    );

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (QFile::exists(filePath)) {
    QMessageBox::warning(this, tr("Warning"),
      tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
    return;
  }

  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (!filePath.isEmpty()) {
    if (walletAdapter->isOpen()) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }
    QString oldWalletFile = Settings::instance().getWalletFile();
    Settings::instance().setWalletFile(filePath);
    AccountKeys accountKeys = m_deterministicAdapter.generateDeterministicKeys();
    if (walletAdapter->createHd(filePath, "", accountKeys, 1, false) == IWalletAdapter::INIT_SUCCESS) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
      Q_ASSERT(walletAdapter->isOpen());
      QString fileName = Settings::instance().getWalletFile();
      fileName.append(QString(".backup"));
      walletAdapter->exportWallet(fileName,false,CryptoNote::WalletSaveLevel::SAVE_KEYS_ONLY,true);
      showMnemonicSeed();
    } else {
      Settings::instance().setWalletFile(oldWalletFile);
    }
  }
}

void MainWindow::openWallet() {
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open .wallet/.keys file"),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallet (*.wallet *.keys)"));

  QString walletFilePath = filePath;
  if (!filePath.isEmpty()) {
    if (filePath.endsWith(".keys")) {
      walletFilePath.replace(filePath.lastIndexOf(".keys"), 5, ".wallet");
      if (QFile::exists(walletFilePath)) {
        QMessageBox::warning(this, tr("Warning"),
          tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(walletFilePath).fileName()));
        return;
      }
    } else {
      filePath.clear();
    }
  } else {
    return;
  }

  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter->isOpen()) {
    walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
    walletAdapter->removeObserver(this);
    walletAdapter->close();
    walletAdapter->addObserver(this);
  }

  m_ui->m_noWalletFrame->openWallet(walletFilePath, filePath);
}

void MainWindow::backupWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Backup wallet to..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)")
    );
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (QFile::exists(filePath)) {
    QMessageBox::warning(this, tr("Warning"),
      tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
    return;
  }

  if (!filePath.isEmpty()) {
    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    walletAdapter->exportWallet(filePath, true, CryptoNote::WalletSaveLevel::SAVE_KEYS_AND_TRANSACTIONS, false);
  }
}

void MainWindow::saveWalletKeys() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save keys to..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallet (*.wallet)")
    );
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (QFile::exists(filePath)) {
    QMessageBox::warning(this, tr("Warning"),
      tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
    return;
  }

  if (!filePath.isEmpty()) {
    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    walletAdapter->exportWallet(filePath, false, CryptoNote::WalletSaveLevel::SAVE_KEYS_ONLY, false);
  }
}

void MainWindow::resetWallet() {
  QuestionDialog dlg(tr("Reset wallet?"), tr("Reset wallet to resynchronize its transactions and balance based\n"
    "on the blockchain data. This operation can take some time.\n"
    "Are you sure you would like to reset this wallet?"), this);
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }

  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  Q_ASSERT(walletAdapter->isOpen());
  QString fileName = Settings::instance().getWalletFile();
  QDateTime currentDateTime = QDateTime::currentDateTime();
  fileName.append(QString(".%1.backup").arg(currentDateTime.toString("yyyyMMddHHMMss")));

  walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_KEYS_ONLY, true);
  walletAdapter->removeObserver(this);
  walletAdapter->close();
  walletAdapter->addObserver(this);
  m_ui->m_noWalletFrame->openWallet(Settings::instance().getWalletFile(), QString());
}

void MainWindow::encryptWallet() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter->isEncrypted()) {
    IWalletAdapter::PasswordStatus status = IWalletAdapter::PASSWORD_SUCCESS;
    do {
      ChangePasswordDialog dlg(status == IWalletAdapter::PASSWORD_ERROR, this);
      if (dlg.exec() == QDialog::Rejected) {
        return;
      }

      QString oldPassword = dlg.getOldPassword();
      QString newPassword = dlg.getNewPassword();
      status = walletAdapter->changePassword(oldPassword, newPassword);
    } while (status != IWalletAdapter::PASSWORD_SUCCESS);
  } else {
    NewPasswordDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
      QString password = dlg.getPassword();
      if (password.isEmpty()) {
        return;
      }

      walletAdapter->changePassword("", password);
    }
  }
  QString fileName = Settings::instance().getWalletFile();
  fileName.append(QString(".backup"));
  if (!fileName.isEmpty()) {
    // remove old unencrypted backup
    if(QFile::exists(fileName)) {
       QFile::remove(fileName);
    }
    // create new encrypted backup
    walletAdapter->exportWallet(fileName,false,CryptoNote::WalletSaveLevel::SAVE_KEYS_ONLY,true);
  }
}

void MainWindow::signMessage() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  quintptr index = 0;
  if (m_currentAddressState != nullptr) {
    index = m_currentAddressState->currentAddressIndex();
  }
  if (index >= walletAdapter->getAddressCount()) {
    index = 0;
  }
  AccountKeys accountKeys = walletAdapter->getAccountKeys(index);
  QString address = walletAdapter->getAddress(index);
  SignMessageDialog dlg(m_cryptoNoteAdapter, accountKeys, address, this);
  dlg.sign();
  dlg.exec();
}

void MainWindow::verifyMessage() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  quintptr index = 0;
  if (m_currentAddressState != nullptr) {
    index = m_currentAddressState->currentAddressIndex();
  }
  if (index >= walletAdapter->getAddressCount()) {
    index = 0;
  }
  AccountKeys accountKeys = walletAdapter->getAccountKeys(index);
  QString address = walletAdapter->getAddress(index);
  SignMessageDialog dlg(m_cryptoNoteAdapter, accountKeys, address, this);
  dlg.verify();
  dlg.exec();
}

void MainWindow::getBalanceProof() {
  BalanceProofDialog dlg(m_cryptoNoteAdapter, this);
  dlg.exec();
}

void MainWindow::exportKey() {
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);
  QByteArray keys = convertAccountKeysToByteArray(accountKeys);
  KeyDialog dlg(keys, false, this);
  dlg.exec();
}

void MainWindow::exportTrackingKey() {
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);
  std::memset(&accountKeys.spendKeys.secretKey, 0, sizeof(Crypto::SecretKey));
  QByteArray trackingKeys = convertAccountKeysToByteArray(accountKeys);
  KeyDialog dlg(trackingKeys, true, this);
  dlg.exec();
}

void MainWindow::importKey() {
  KeyDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QByteArray key = dlg.getKey();
    QString keyString = dlg.getKeyString();

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save wallet to..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)"));
    if (filePath.isEmpty()) {
      return;
    }

    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    if (QFile::exists(filePath)) {
      QMessageBox::warning(this, tr("Warning"),
        tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
      return;
    }

    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    if (walletAdapter->isOpen()) {
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }

    QString oldWalletFile = Settings::instance().getWalletFile();

    uint64_t addressPrefix;
    std::string data;
    AccountKeys accountKeys;

    if (!keyString.isEmpty() && Tools::Base58::decode_addr(keyString.toStdString(), addressPrefix, data) && addressPrefix == Settings::instance().getAddressPrefix() &&
      data.size() == sizeof(accountKeys)) {
      accountKeys = convertByteArrayToAccountKeys(QByteArray::fromStdString(data));
    } else if (key.size() == sizeof(CryptoNote::AccountKeys)) {
      accountKeys = convertByteArrayToAccountKeys(key);
    } else {
      QMessageBox::warning(this, tr("Warning"), tr("The keys are not valid."), QMessageBox::Ok);
      return;
    }

    Settings::instance().setWalletFile(filePath);
    if (walletAdapter->createWithKeys(filePath, accountKeys) == IWalletAdapter::INIT_SUCCESS) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
    } else {
      QMessageBox::warning(this, tr("Error"), tr("Could not import wallet keys."), QMessageBox::Ok);
      Settings::instance().setWalletFile(oldWalletFile);
    }
  }
}

void MainWindow::restoreFromMnemonicSeed() {
  RestoreFromMnemonicSeedDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QString mnemonicString = dlg.getSeedString().trimmed();
    if (mnemonicString.isEmpty()) {
      return;
    }

    bool addressCountAccepted = false;
    const int restoreAddressCount = QInputDialog::getInt(this, tr("Restore HD addresses"),
      tr("Number of HD addresses to restore:"), 1, 1, 1000000, 1, &addressCountAccepted);
    if (!addressCountAccepted) {
      return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save wallet to..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)"));
    if (filePath.isEmpty()) {
      return;
    }

    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    if (QFile::exists(filePath)) {
      QMessageBox::warning(this, tr("Warning"),
        tr("Can't overwrite existing %1 because it may lead to loss of private keys").arg(QFileInfo(filePath).fileName()));
      return;
    }

    QString oldWalletFile = Settings::instance().getWalletFile();

    AccountKeys _keys;
    std::string seed_language;
    std::string seed = mnemonicString.toUtf8().constData();

    if(!Crypto::ElectrumWords::words_to_bytes(seed, _keys.spendKeys.secretKey, seed_language)) {
      QMessageBox::critical(nullptr, tr("Mnemonic seed is invalid"),
                            tr("Mnemonic seed is invalid. "
                               "Make sure you entered it correctly."), QMessageBox::Ok);
      return;
    }

    Crypto::secret_key_to_public_key(_keys.spendKeys.secretKey, _keys.spendKeys.publicKey);
    CryptoNote::AccountBase::generateViewFromSpend(_keys.spendKeys.secretKey, _keys.viewKeys.secretKey, _keys.viewKeys.publicKey);

    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    if (walletAdapter->isOpen()) {
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }

    Settings::instance().setWalletFile(filePath);
    if (walletAdapter->createHd(filePath, "", _keys, static_cast<quint32>(restoreAddressCount), true) == IWalletAdapter::INIT_SUCCESS) {
      walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
    } else {
      Settings::instance().setWalletFile(oldWalletFile);
    }
  }
}

void MainWindow::openPaymentRequestClicked() {
  OpenUriDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QUrl request = dlg.getURI();
    if (request.isEmpty()) {
      return;
    }
    m_ui->m_sendFrame->urlReceived(request);
    if (m_sendNavAction != nullptr) m_sendNavAction->setChecked(true);
  }
}

void MainWindow::aboutQt() {
  QMessageBox::aboutQt(this);
}

void MainWindow::about() {
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::setStartOnLoginEnabled(bool _enable) {
  Settings::instance().setStartOnLoginEnabled(_enable);
  m_ui->m_autostartAction->setChecked(Settings::instance().isStartOnLoginEnabled());
}

void MainWindow::setMinimizeToTrayEnabled(bool _enable) {
  Settings::instance().setMinimizeToTrayEnabled(_enable);
}

void MainWindow::setCloseToTrayEnabled(bool _enable) {
  Settings::instance().setCloseToTrayEnabled(_enable);
}

void MainWindow::setBlockchainExplorerEnabled(bool _enable) {
  Settings::instance().setBlockchainExplorerEnabled(_enable);
  QMessageBox::information(nullptr, tr("Blockchain explorer enabling"),
                        tr("Changes will take effect after wallet restart."), QMessageBox::Ok);
}

void MainWindow::showPreferences() {
  OptionsDialog dlg(m_cryptoNoteAdapter, m_donationManager, m_addressBookModel, this);
  ConnectionMethod currentConnectionMethod = Settings::instance().getConnectionMethod();
  quint16 currentLocalRpcPort = Settings::instance().getLocalRpcPort();
  QUrl currentRemoteRpcUrl = Settings::instance().getRemoteRpcUrl();
  if (dlg.exec() == QDialog::Accepted) {
    ConnectionMethod newConnectionMethod = Settings::instance().getConnectionMethod();
    quint16 newLocalRpcPort = Settings::instance().getLocalRpcPort();
    QUrl newRemoteRpcUrl = Settings::instance().getRemoteRpcUrl();

    if(newConnectionMethod != currentConnectionMethod ||
      (newConnectionMethod == ConnectionMethod::LOCAL && newLocalRpcPort != currentLocalRpcPort) ||
      (newConnectionMethod == ConnectionMethod::REMOTE && newRemoteRpcUrl != currentRemoteRpcUrl)) {
      setEnabled(false);
      Q_EMIT reinitCryptoNoteAdapterSignal();
    }
  }
}

void MainWindow::showMnemonicSeed() {
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);

  if (m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->isTrackingWallet()) {
    WalletLogger::info(tr("[Deterministic Wallet Adapter] Wallet is watch-only and has no seed."));
    QMessageBox::critical(nullptr, tr("This is tracking wallet"),
                          tr("Wallet is watch-only and has no seed."), QMessageBox::Ok);
    return;
  }
  if(!m_deterministicAdapter.isDeterministic(accountKeys)) {
    WalletLogger::info(tr("[Deterministic Wallet Adapter] Wallet uses independent address keys and has no seed."));
    QMessageBox::critical(nullptr, tr("This wallet has independent address keys"),
                          tr("This wallet has independent address keys and has no mnemonic seed."), QMessageBox::Ok);
    return;
  }

  MnemonicDialog dlg(accountKeys, this);
  dlg.exec();
}

void MainWindow::showSeedFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_address);
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter != nullptr && walletAdapter->isOpen() &&
      walletAdapter->getAddressGenerationMode() == CryptoNote::AddressGenerationMode::HD_DETERMINISTIC) {
    showMnemonicSeed();
    return;
  }

  // Older deterministic containers only have a seed for address 0. Independent
  // address containers require backing up each address's spend key separately.
  if (_index != 0) {
    QMessageBox::information(this, tr("Mnemonic seed unavailable"),
                             tr("This wallet uses independent address keys, so additional addresses are not recoverable "
                                "from one seed phrase.\n\nBack them up via \"Show keys...\" on the address card."),
                             QMessageBox::Ok);
    return;
  }
  showMnemonicSeed();
}

void MainWindow::communityForumTriggered() {
  QDesktopServices::openUrl(QUrl::fromUserInput(COMMUNITY_FORUM_URL));
}

void MainWindow::reportIssueTriggered() {
  QDesktopServices::openUrl(QUrl::fromUserInput(REPORT_ISSUE_URL));
}

void MainWindow::setDevDonation() {
  if(m_addressBookManager->findAddressByAddress(DONATION_ADDRESS) == INVALID_ADDRESS_INDEX){
     m_addressBookManager->addAddress(tr("Development Fund"), DONATION_ADDRESS, "", true);
     m_donationManager->setDonationChangeAddress(DONATION_ADDRESS);
     m_donationManager->setDonationChangeEnabled(true);
     m_donationManager->setDonationChangeAmount(1);
  }
}

void MainWindow::closeWallet() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  Q_ASSERT(walletAdapter->isOpen());
  walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);
  walletAdapter->removeObserver(this);
  walletAdapter->close();
  walletClosed();
  walletAdapter->addObserver(this);
}

void MainWindow::buildTopNavToolBar() {
  m_mainToolBar = new QToolBar(this);
  m_mainToolBar->setObjectName("m_mainToolBar");
  m_mainToolBar->setMovable(false);
  m_mainToolBar->setFloatable(false);
  m_mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
  m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_mainToolBar->setIconSize(QSize(20, 20));
  addToolBar(Qt::TopToolBarArea, m_mainToolBar);

  m_navActionGroup = new QActionGroup(this);
  m_navActionGroup->setExclusive(true);

  // Each nav action owns the visibility of one content frame. The QActionGroup
  // is exclusive, so toggling one on auto-toggles the others off and the
  // action->frame visibility wiring keeps the right frame on screen.
  struct NavSpec {
    QAction** outAction;
    QFrame* frame;
    QString objectName;
    QString text;
    QString iconPath;
  };
  const NavSpec specs[] = {
    { &m_overviewNavAction, m_ui->m_overviewFrame,      "m_overviewNavAction", tr("Overview"), ":icons/overview" },
    { &m_sendNavAction,     m_ui->m_sendFrame,          "m_sendNavAction",     tr("Send"),     ":icons/send" },
    { &m_receiveNavAction,  m_ui->m_receiveFrame,       "m_receiveNavAction",  tr("Receive"),  ":icons/receive" },
    { &m_historyNavAction,  m_ui->m_transactionsFrame,  "m_historyNavAction",  tr("History"),  ":icons/transactions" },
    { &m_contactsNavAction, m_ui->m_addressBookFrame,   "m_contactsNavAction", tr("Contacts"), ":icons/address_book" },
    { &m_explorerNavAction, m_ui->m_blockExplorerFrame, "m_explorerNavAction", tr("Explorer"), ":icons/explorer" },
  };
  for (const NavSpec& spec : specs) {
    QAction* action = new QAction(spec.text, this);
    action->setObjectName(spec.objectName);
    action->setCheckable(true);
    action->setIcon(QIcon(spec.iconPath));
    m_navActionGroup->addAction(action);
    m_mainToolBar->addAction(action);
    QFrame* frame = spec.frame;
    connect(action, &QAction::toggled, frame, &QWidget::setVisible);
    action->setEnabled(false);
    *spec.outAction = action;
  }

  // Route the Payment menu's "Create payment request" straight to the Receive tab.
  if (m_receiveNavAction != nullptr && m_ui->m_createPaymentRequestAction != nullptr) {
    connect(m_ui->m_createPaymentRequestAction, &QAction::triggered,
      m_receiveNavAction, [this]() { m_receiveNavAction->setChecked(true); });
  }

  // Right-aligned Locked / Total balance block at the far end of the toolbar.
  QWidget* toolBarSpacer = new QWidget(m_mainToolBar);
  toolBarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_mainToolBar->addWidget(toolBarSpacer);

  QFrame* balanceFrame = new QFrame(m_mainToolBar);
  balanceFrame->setObjectName("m_toolBarBalanceFrame");
  balanceFrame->setAttribute(Qt::WA_StyledBackground, true);
  QHBoxLayout* bl = new QHBoxLayout(balanceFrame);
  bl->setContentsMargins(8, 0, 16, 0);
  bl->setSpacing(6);

  QLabel* lockedCaption = new QLabel(tr("Locked: "), balanceFrame);
  lockedCaption->setObjectName("m_lockedCaption");
  // FadingAmountLabel: truncated 4-decimal form by default, full
  // precision on hover, gradient fade on the truncated tail. See the
  // class definition near the top of this file.
  m_sidebarLockedLabel = new FadingAmountLabel(balanceFrame);
  m_sidebarLockedLabel->setObjectName("m_lockedValue");

  QLabel* totalCaption = new QLabel(tr("Total: "), balanceFrame);
  totalCaption->setObjectName("m_totalCaption");
  m_sidebarTotalLabel = new FadingAmountLabel(balanceFrame);
  m_sidebarTotalLabel->setObjectName("m_totalValue");
  static_cast<FadingAmountLabel*>(m_sidebarTotalLabel)->setSelectableWhenExpanded(true);

  bl->addWidget(lockedCaption);
  bl->addWidget(m_sidebarLockedLabel);
  bl->addSpacing(24);
  bl->addWidget(totalCaption);
  bl->addWidget(m_sidebarTotalLabel);

  m_mainToolBar->addWidget(balanceFrame);
}

void MainWindow::buildAddressSidebar() {
  m_addressSidebar = new AddressSidebar(m_ui->m_toolFrame);
  m_addressSidebar->setModel(m_addressListModel);
  m_addressSidebar->setCurrentAddressState(m_currentAddressState);
  m_addressSidebar->setCryptoNoteAdapter(m_cryptoNoteAdapter);

  QVBoxLayout* toolLayout = qobject_cast<QVBoxLayout*>(m_ui->m_toolFrame->layout());
  if (toolLayout != nullptr) {
    toolLayout->addWidget(m_addressSidebar, 1);
  }

  m_ui->m_toolFrame->setMinimumWidth(280);

  connect(m_addressSidebar, &AddressSidebar::addAddressRequestedSignal, this, &MainWindow::createAddressRequested);
  connect(m_addressSidebar, &AddressSidebar::copyAddressRequestedSignal, this, &MainWindow::copyAddressFromCard);
  connect(m_addressSidebar, &AddressSidebar::copyAccountNumberRequestedSignal, this, &MainWindow::copyAccountNumberFromCard);
  connect(m_addressSidebar, &AddressSidebar::registerAccountNumberRequestedSignal, this, &MainWindow::registerAccountNumberFromCard);
  connect(m_addressSidebar, &AddressSidebar::showQrRequestedSignal, this, &MainWindow::showQrFromCard);
  connect(m_addressSidebar, &AddressSidebar::showKeysRequestedSignal, this, &MainWindow::showKeysFromCard);
  connect(m_addressSidebar, &AddressSidebar::exportTrackingKeyRequestedSignal, this, &MainWindow::exportTrackingKeyFromCard);
  connect(m_addressSidebar, &AddressSidebar::showSeedRequestedSignal, this, &MainWindow::showSeedFromCard);
  connect(m_addressSidebar, &AddressSidebar::renameRequestedSignal, this, &MainWindow::renameAddressFromCard);
  connect(m_addressSidebar, &AddressSidebar::sendFromRequestedSignal, this, &MainWindow::sendFromCard);
  connect(m_addressSidebar, &AddressSidebar::balanceProofRequestedSignal, this, &MainWindow::balanceProofFromCard);
  connect(m_addressSidebar, &AddressSidebar::deleteRequestedSignal, this, &MainWindow::deleteAddressFromCard);
}

void MainWindow::installSidebarBalance() {
  if (m_sidebarTotalLabel == nullptr || m_sidebarLockedLabel == nullptr) {
    return;
  }

  // FadingAmountLabel handles hover/swap/gradient internally; we just feed
  // it the truncated and full strings each time the model changes. (The
  // labels are constructed as FadingAmountLabel* in buildTopNavToolBar
  // and then exposed as QLabel* on MainWindow, so we cast back here.)
  auto applyAmount = [](QLabel* label, const QString& shortText, const QString& fullText) {
    auto* fading = static_cast<FadingAmountLabel*>(label);
    fading->setAmount(shortText, fullText);
    // Tooltip stays as the full amount for screen-reader / keyboard-only
    // users who can't hover the label themselves.
    fading->setToolTip(fullText);
  };

  auto updateBalance = [this, applyAmount]() {
    const quint64 actual = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_ACTUAL_BALANCE).value<quint64>();
    const quint64 pending = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_PENDING_BALANCE).value<quint64>();
    const quint64 total = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_TOTAL_BALANCE).value<quint64>();
    const quint64 locked = (total >= actual) ? (total - actual) : pending;
    if (m_cryptoNoteAdapter != nullptr) {
      const QString totalShort = m_cryptoNoteAdapter->formatAmountToShort(static_cast<qint64>(total));
      const QString lockedShort = m_cryptoNoteAdapter->formatAmountToShort(static_cast<qint64>(locked));
      const QString totalFull = m_cryptoNoteAdapter->formatUnsignedAmount(total);
      const QString lockedFull = m_cryptoNoteAdapter->formatUnsignedAmount(locked);
      applyAmount(m_sidebarTotalLabel, totalShort, totalFull);
      applyAmount(m_sidebarLockedLabel, lockedShort, lockedFull);
    } else {
      const QString totalRaw = QString::number(total);
      const QString lockedRaw = QString::number(locked);
      applyAmount(m_sidebarTotalLabel, totalRaw, totalRaw);
      applyAmount(m_sidebarLockedLabel, lockedRaw, lockedRaw);
    }
  };
  updateBalance();
  connect(m_walletStateModel, &QAbstractItemModel::dataChanged, this, [updateBalance]() { updateBalance(); });
  connect(m_walletStateModel, &QAbstractItemModel::modelReset, this, [updateBalance]() { updateBalance(); });
}

void MainWindow::rearrangeWalletMenu() {
  // Move wallet-container import actions (create a new wallet file from keys/seed)
  // out of the Wallet menu and into the File menu, where other container operations live.
  QMenu* walletMenu = m_ui->menuWallet;
  QMenu* fileMenu = m_ui->menuFile;
  if (walletMenu == nullptr || fileMenu == nullptr) {
    return;
  }

  QAction* importKeyAction = m_ui->m_importKeyAction;
  QAction* importSeedAction = m_ui->m_importSeedAction;
  QAction* exportKeyAction = m_ui->m_exportKeyAction;
  QAction* exportTrackingKeyAction = m_ui->m_exportTrackingKeyAction;
  QAction* balanceProofAction = m_ui->m_getBalanceProofAction;
  QAction* showSeedAction = m_ui->m_showSeedAction;

  if (importKeyAction != nullptr) walletMenu->removeAction(importKeyAction);
  if (importSeedAction != nullptr) walletMenu->removeAction(importSeedAction);
  // Export key, Export tracking key, Prove balance and Show mnemonic seed become
  // per-address operations reachable from the address card's Advanced menu;
  // remove them from the Wallet menu. (Show seed only works for the primary
  // address; the per-card handler shows an "unsupported" error for the rest.)
  if (exportKeyAction != nullptr) walletMenu->removeAction(exportKeyAction);
  if (exportTrackingKeyAction != nullptr) walletMenu->removeAction(exportTrackingKeyAction);
  if (balanceProofAction != nullptr) walletMenu->removeAction(balanceProofAction);
  if (showSeedAction != nullptr) walletMenu->removeAction(showSeedAction);

  // Rename to clarify these actions create a NEW wallet container (not an address).
  if (importKeyAction != nullptr) {
    importKeyAction->setText(tr("Import wallet from keys..."));
  }
  if (importSeedAction != nullptr) {
    importSeedAction->setText(tr("Import wallet from mnemonic seed..."));
  }

  QAction* fileAnchor = m_ui->m_signMessageAction;
  if (importKeyAction != nullptr) {
    fileMenu->insertAction(fileAnchor, importKeyAction);
  }
  if (importSeedAction != nullptr) {
    fileMenu->insertAction(fileAnchor, importSeedAction);
  }
  if ((importKeyAction != nullptr || importSeedAction != nullptr) && fileAnchor != nullptr) {
    fileMenu->insertSeparator(fileAnchor);
  }

  // Insert Create/Import address actions at the top of the Wallet menu.
  QList<QAction*> walletActions = walletMenu->actions();
  QAction* walletAnchor = walletActions.isEmpty() ? nullptr : walletActions.first();

  m_createAddressAction = new QAction(tr("Create new address"), this);
  m_importAddressAction = new QAction(tr("Import address..."), this);
  connect(m_createAddressAction, &QAction::triggered, this, &MainWindow::createAddressRequested);
  connect(m_importAddressAction, &QAction::triggered, this, &MainWindow::importAddressRequested);

  walletMenu->insertAction(walletAnchor, m_createAddressAction);
  walletMenu->insertAction(walletAnchor, m_importAddressAction);
  walletMenu->insertSeparator(walletAnchor);

  // Gate both new actions on wallet-open state (start disabled; walletOpened/Closed toggle).
  m_createAddressAction->setEnabled(false);
  m_importAddressAction->setEnabled(false);
}

void MainWindow::importAddressRequested() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter == nullptr || !walletAdapter->isOpen()) {
    return;
  }

  bool ok = false;
  const QString prompt = tr(
    "Paste one of the following to import an address into this wallet:\n"
    "  \u2022 a spend secret key (64 hex characters)\n"
    "  \u2022 a spend public key (64 hex characters, watch-only)\n"
    "  \u2022 an encoded address/keys blob produced by \"Save keys\"\n\n"
    "The view key is shared across all addresses in this wallet, so it is reused automatically.");
  const QString input = QInputDialog::getMultiLineText(this, tr("Import address"), prompt, QString(), &ok);
  if (!ok) {
    return;
  }
  const QString trimmed = input.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }

  // Pick the view key from the primary address (all addresses in WalletGreen share a single view key).
  AccountKeys primaryKeys = walletAdapter->getAccountKeys(0);
  AccountKeys newKeys;
  newKeys.viewKeys = primaryKeys.viewKeys;
  std::memset(&newKeys.spendKeys, 0, sizeof(newKeys.spendKeys));

  bool parsed = false;

  // 1) Try Base58 address/keys blob (same encoding Save-keys produces).
  {
    uint64_t prefix = 0;
    std::string data;
    AccountKeys tmp;
    if (Tools::Base58::decode_addr(trimmed.toStdString(), prefix, data) &&
        prefix == Settings::instance().getAddressPrefix() &&
        data.size() == sizeof(tmp)) {
      tmp = convertByteArrayToAccountKeys(QByteArray::fromStdString(data));
      newKeys.spendKeys = tmp.spendKeys;
      parsed = true;
    }
  }

  // 2) Try a bare 64-hex-char spend key (secret or public).
  if (!parsed && trimmed.size() == 64) {
    QByteArray raw = QByteArray::fromHex(trimmed.toLatin1());
    if (raw.size() == static_cast<int>(sizeof(Crypto::SecretKey))) {
      // Heuristic: try as secret first. If the derived public key is a valid point, keep the secret.
      Crypto::SecretKey secret;
      std::memcpy(&secret, raw.constData(), sizeof(secret));
      Crypto::PublicKey derived;
      if (Crypto::secret_key_to_public_key(secret, derived)) {
        newKeys.spendKeys.secretKey = secret;
        newKeys.spendKeys.publicKey = derived;
        parsed = true;
      } else {
        // Fall back to treating it as a public key (watch-only sub-address).
        Crypto::PublicKey pub;
        std::memcpy(&pub, raw.constData(), sizeof(pub));
        if (Crypto::check_key(pub)) {
          newKeys.spendKeys.publicKey = pub;
          std::memset(&newKeys.spendKeys.secretKey, 0, sizeof(newKeys.spendKeys.secretKey));
          parsed = true;
        }
      }
    }
  }

  if (!parsed) {
    QMessageBox::warning(this, tr("Import address"),
      tr("The input is not a recognized spend key or address/keys blob."));
    return;
  }

  const QString created = walletAdapter->createAddress(newKeys);
  if (created.isEmpty()) {
    QMessageBox::warning(this, tr("Import address"),
      tr("The wallet refused the key (it may already be present or the view key does not match)."));
    return;
  }

  walletAdapter->save(CryptoNote::WalletSaveLevel::SAVE_ALL, true);

  if (m_currentAddressState != nullptr && m_addressListModel != nullptr) {
    const int rows = m_addressListModel->rowCount();
    for (int row = 0; row < rows; ++row) {
      const QModelIndex idx = m_addressListModel->index(row, 0);
      if (m_addressListModel->data(idx, AddressListModel::ROLE_ADDRESS).toString() == created) {
        m_currentAddressState->setCurrent(static_cast<quintptr>(row), created);
        break;
      }
    }
  }
}

void MainWindow::createAddressRequested() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter == nullptr || !walletAdapter->isOpen()) {
    return;
  }
  walletAdapter->createAddress();
}

void MainWindow::copyAddressFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_index);
  QApplication::clipboard()->setText(_address);
}

void MainWindow::copyAccountNumberFromCard(quintptr _index, const QString& _accountNumber) {
  Q_UNUSED(_index);
  if (_accountNumber.isEmpty()) {
    return;
  }
  QApplication::clipboard()->setText(_accountNumber);
}

void MainWindow::registerAccountNumberFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_address);
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter == nullptr || !walletAdapter->isOpen() || walletAdapter->isTrackingWallet()) {
    return;
  }
  if (_index >= walletAdapter->getAddressCount()) {
    return;
  }
  QuestionDialog dlg(tr("Register account number?"),
    tr("Registering an account number publishes a short alias for this "
       "address on-chain. This costs one network fee plus a small dust amount "
       "(returned to you in the same transaction).\n\n"
       "After the next block confirms the registration, your account number "
       "will appear under this address. Each address can have its own "
       "account number.\n\n"
       "Continue?"), this);
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }
  IWalletAdapter::SendTransactionStatus status = walletAdapter->registerAccountNumber(_index);
  if (status == IWalletAdapter::SEND_SUCCESS) {
    // Hide the Register action immediately so the user can't re-trigger it
    // while the registration tx is still unconfirmed. The flag is cleared
    // automatically once the daemon hands us back the resolved account
    // number, or on wallet close/open.
    if (m_addressSidebar != nullptr) {
      m_addressSidebar->markRegistrationSent(_index);
    }
    QMessageBox::information(this, tr("Account number"),
      tr("Account-number registration submitted. It will become visible once "
         "the transaction confirms."), QMessageBox::Ok);
  } else {
    QMessageBox::warning(this, tr("Account number"),
      tr("Failed to register account number (status %1). Make sure the address "
         "has at least 1 KRB available and try again.").arg(static_cast<int>(status)),
      QMessageBox::Ok);
  }
}

void MainWindow::showQrFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_index);
  QRCodeDialog dlg(tr("Address QR code"), _address, this);
  dlg.exec();
}

void MainWindow::showKeysFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_address);
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter == nullptr || !walletAdapter->isOpen()) {
    return;
  }
  AccountKeys accountKeys = walletAdapter->getAccountKeys(_index);
  QByteArray keys = convertAccountKeysToByteArray(accountKeys);
  KeyDialog dlg(keys, false, this);
  dlg.exec();
}

void MainWindow::exportTrackingKeyFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_address);
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter == nullptr || !walletAdapter->isOpen()) {
    return;
  }
  AccountKeys accountKeys = walletAdapter->getAccountKeys(_index);
  std::memset(&accountKeys.spendKeys.secretKey, 0, sizeof(Crypto::SecretKey));
  QByteArray trackingKeys = convertAccountKeysToByteArray(accountKeys);
  KeyDialog dlg(trackingKeys, true, this);
  dlg.exec();
}

void MainWindow::balanceProofFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_index);
  Q_UNUSED(_address);
  // TODO: extend BalanceProofDialog to prove a single address's balance instead
  //       of the aggregate wallet balance. For now invoke the existing dialog.
  BalanceProofDialog dlg(m_cryptoNoteAdapter, this);
  dlg.exec();
}

void MainWindow::renameAddressFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_index);
  if (m_addressListModel == nullptr || _address.isEmpty()) {
    return;
  }
  AddressListModel* model = static_cast<AddressListModel*>(m_addressListModel);
  const int row = static_cast<int>(_index);
  QString existingLabel;
  if (row >= 0 && row < model->rowCount()) {
    existingLabel = model->data(model->index(row, AddressListModel::COLUMN_LABEL),
                                AddressListModel::ROLE_LABEL).toString();
  }
  bool ok = false;
  const QString newLabel = QInputDialog::getText(this, tr("Rename address"),
    tr("Label for this address (leave empty to clear):"),
    QLineEdit::Normal, existingLabel, &ok);
  if (!ok) {
    return;
  }
  model->setLabelForAddress(_address, newLabel.trimmed());
}

void MainWindow::sendFromCard(quintptr _index, const QString& _address) {
  if (m_currentAddressState != nullptr && !_address.isEmpty()) {
    m_currentAddressState->setCurrent(_index, _address);
  }
  if (m_sendNavAction != nullptr) {
    m_sendNavAction->trigger();
  }
}

void MainWindow::deleteAddressFromCard(quintptr _index, const QString& _address) {
  Q_UNUSED(_address);
  if (_index == 0) {
    return;
  }
  QuestionDialog dlg(tr("Delete address"),
    tr("Are you sure you want to delete this address? This cannot be undone."), this);
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter != nullptr && walletAdapter->isOpen()) {
    walletAdapter->deleteAddress(_index);
    if (m_currentAddressState != nullptr && walletAdapter->getAddressCount() > 0) {
      m_currentAddressState->setCurrent(0, walletAdapter->getAddress(0));
    }
  }
}

}
