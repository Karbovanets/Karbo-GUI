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

#include <cstring>

#include <QApplication>
#include <QGuiApplication>
#include <QActionGroup>
#include <QInputDialog>
#include <QPushButton>
#include <QClipboard>
#include <QCloseEvent>
#include <QDataWidgetMapper>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaMethod>
#include <QMovie>
#include <QSessionManager>
#include <QSystemTrayIcon>
#include <QToolBar>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <ctime>

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
#include "crypto/crypto.h"
extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}
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

const int MAX_RECENT_WALLET_COUNT = 10;
const char COMMUNITY_FORUM_URL[] = "https://forum.karbo.io";
const char REPORT_ISSUE_URL[] = "https://karbo.io/contact";

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
  m_addRecipientAction(new QAction(this)), m_styleSheetTemplate(_styleSheetTemplate), m_walletStateMapper(new QDataWidgetMapper(this)),
  m_syncMovie(new QMovie(Settings::instance().getCurrentStyle().getWalletSyncGifFile(), QByteArray(), this)),
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
  m_ui->m_headerFrame->hide();

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
  m_ui->m_addressLabel->installEventFilter(this);
  m_ui->m_balanceLabel->installEventFilter(this);
  m_walletStateMapper->setModel(m_walletStateModel);
  m_walletStateMapper->addMapping(m_ui->m_balanceFrame, WalletStateModel::COLUMN_IS_OPEN, "visible");
  m_walletStateMapper->addMapping(m_ui->m_walletFrame, WalletStateModel::COLUMN_IS_OPEN, "visible");
  m_walletStateMapper->addMapping(m_ui->m_noWalletLabel, WalletStateModel::COLUMN_IS_CLOSED, "visible");
  m_walletStateMapper->addMapping(m_ui->m_notEncryptedFrame, WalletStateModel::COLUMN_IS_NOT_ENCRYPTED, "visible");
  m_walletStateMapper->addMapping(m_ui->m_addressLabel, WalletStateModel::COLUMN_ADDRESS, "text");
  m_walletStateMapper->addMapping(m_ui->m_balanceLabel, WalletStateModel::COLUMN_TOTAL_SHORT_BALANCE, "text");
  m_walletStateMapper->setCurrentIndex(0);

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

  m_ui->m_balanceIconLabel->setPixmap(Settings::instance().getCurrentStyle().getBalanceIcon());
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
  if (_object == m_ui->m_addressLabel && _event->type() == QEvent::MouseButtonRelease) {
    copyAddress();
  } else if (_object == m_ui->m_balanceLabel && _event->type() == QEvent::MouseButtonRelease) {
    copyBalance();
  }

  return false;
}

void MainWindow::walletOpened() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  setOpenedState();
  QStringList recentWalletList = Settings::instance().getRecentWalletList();
  recentWalletList.removeAll(Settings::instance().getWalletFile());
  recentWalletList.prepend(Settings::instance().getWalletFile());
  while (recentWalletList.size() > MAX_RECENT_WALLET_COUNT) {
    recentWalletList.removeLast();
  }

  Settings::instance().setRecentWalletList(recentWalletList);
  updateRecentWalletActions();
  if (walletAdapter->isTrackingWallet()) {
    m_ui->m_sendButton->setEnabled(false);
    m_ui->m_addressBookButton->setEnabled(false);
    m_ui->m_openPaymentRequestAction->setEnabled(false);
    m_ui->m_exportKeyAction->setEnabled(false);
  }
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);
  if (!m_deterministicAdapter.isDeterministic(accountKeys)) {
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
}

void MainWindow::passwordChanged() {
  IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
  if (walletAdapter->isEncrypted()) {
    m_ui->m_notEncryptedFrame->hide();
  }

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
    m_ui->m_sendButton->click();
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
  QList<QAbstractButton*> toolButtons = m_ui->m_toolButtonGroup->buttons();
  for (const auto& button : toolButtons) {
    button->setChecked(false);
    button->setEnabled(true);
  }

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

  m_ui->m_overviewButton->setChecked(true);
  m_ui->m_blockExplorerButton->setEnabled(m_cryptoNoteAdapter->getNodeAdapter()->getBlockChainExplorerAdapter() != nullptr);

  if (m_overviewNavAction != nullptr) m_overviewNavAction->setEnabled(true);
  if (m_sendNavAction != nullptr) m_sendNavAction->setEnabled(true);
  if (m_receiveNavAction != nullptr) m_receiveNavAction->setEnabled(true);
  if (m_historyNavAction != nullptr) m_historyNavAction->setEnabled(true);
  if (m_contactsNavAction != nullptr) m_contactsNavAction->setEnabled(true);
  if (m_explorerNavAction != nullptr) {
    m_explorerNavAction->setEnabled(m_cryptoNoteAdapter->getNodeAdapter()->getBlockChainExplorerAdapter() != nullptr);
  }

  m_ui->m_enableBlockchainExplorerAction->setEnabled(Settings::instance().getConnectionMethod() == ConnectionMethod::EMBEDDED ||
                                                     m_cryptoNoteAdapter->getNodeAdapter()->getNodeType() == NodeType::IN_PROCESS);
  m_ui->m_receiveButton->setEnabled(true);

  if (m_createAddressAction != nullptr) m_createAddressAction->setEnabled(true);
  if (m_importAddressAction != nullptr) m_importAddressAction->setEnabled(true);
}

void MainWindow::setClosedState() {
  QList<QAbstractButton*> toolButtons = m_ui->m_toolButtonGroup->buttons();
  for (const auto& button : toolButtons) {
    button->setChecked(false);
    button->setEnabled(false);
  }

  if (m_overviewNavAction != nullptr) m_overviewNavAction->setEnabled(false);
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
  m_ui->m_sendButton->click();
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
  if (_topLeft.column() == WalletStateModel::COLUMN_ABOUT_TO_BE_SYNCHRONIZED) {
    bool walletAboutToBeSynchronized = _topLeft.data().toBool();
    if (!walletAboutToBeSynchronized) {
      //m_walletStateMapper->removeMapping(m_ui->m_balanceLabel);
      //m_ui->m_balanceLabel->setMovie(m_syncMovie);
      //m_syncMovie->start();
      //m_ui->m_balanceLabel->setCursor(Qt::ArrowCursor);
      //m_ui->m_balanceLabel->removeEventFilter(this);
      //m_ui->m_balanceLabel->setToolTip(QString());
      m_ui->m_getBalanceProofAction->setEnabled(true);
  } else {
      //m_syncMovie->stop();
      //m_ui->m_balanceLabel->setMovie(nullptr);
      //m_walletStateMapper->addMapping(m_ui->m_balanceLabel, WalletStateModel::COLUMN_TOTAL_SHORT_BALANCE, "text");
      //m_walletStateMapper->revert();
      //m_ui->m_balanceLabel->setCursor(Qt::PointingHandCursor);
      //m_ui->m_balanceLabel->installEventFilter(this);
      //m_ui->m_balanceLabel->setToolTip(tr("Click to copy"));
      m_ui->m_getBalanceProofAction->setEnabled(false);
    }
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
  m_ui->m_balanceIconLabel->setPixmap(Settings::instance().getCurrentStyle().getBalanceIcon());
  m_syncMovie->stop();
  m_syncMovie->setFileName(Settings::instance().getCurrentStyle().getWalletSyncGifFile());
  m_syncMovie->start();
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

// This is original createWallet() function renamed and used to create nondeterminisctic wallets
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
    uint64_t creationTimestamp = static_cast<uint64_t>(time(nullptr));
    if (walletAdapter->createWithKeysAndTimestamp(filePath, accountKeys, creationTimestamp) == IWalletAdapter::INIT_SUCCESS) {
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
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);
  QString address = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAddress(0);
  SignMessageDialog dlg(m_cryptoNoteAdapter, accountKeys, address, this);
  dlg.sign();
  dlg.exec();
}

void MainWindow::verifyMessage() {
  AccountKeys accountKeys = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAccountKeys(0);
  QString address = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter()->getAddress(0);
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
    Crypto::SecretKey second;
    keccak((uint8_t *)&_keys.spendKeys.secretKey, sizeof(Crypto::SecretKey), (uint8_t *)&second, sizeof(Crypto::SecretKey));
    Crypto::generate_deterministic_keys(_keys.viewKeys.publicKey, _keys.viewKeys.secretKey, second);

    IWalletAdapter* walletAdapter = m_cryptoNoteAdapter->getNodeAdapter()->getWalletAdapter();
    if (walletAdapter->isOpen()) {
      walletAdapter->removeObserver(this);
      walletAdapter->close();
      walletAdapter->addObserver(this);
    }

    Settings::instance().setWalletFile(filePath);
    if (walletAdapter->createWithKeys(filePath, _keys) == IWalletAdapter::INIT_SUCCESS) {
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
    m_ui->m_sendButton->click();
  }
}

void MainWindow::aboutQt() {
  QMessageBox::aboutQt(this);
}

void MainWindow::about() {
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::copyAddress() {
  QApplication::clipboard()->setText(m_walletStateModel->index(0, WalletStateModel::COLUMN_ADDRESS).data().toString());
  m_ui->m_copyAddressLabel->start();
}

void MainWindow::copyBalance() {
  QString balanceString = m_walletStateModel->index(0, WalletStateModel::COLUMN_TOTAL_BALANCE).data().toString();
  balanceString.remove(',');
  QApplication::clipboard()->setText(balanceString);
  m_ui->m_balanceCopyLabel->start();
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

void MainWindow::showQrCode() {
  QRCodeDialog dlg(tr("QR Code"), m_walletStateModel->index(0, WalletStateModel::COLUMN_ADDRESS).data().toString(), this);
  dlg.exec();
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
    WalletLogger::info(tr("[Deterministic Wallet Adapter] Wallet is non-deterministic and has no seed."));
    QMessageBox::critical(nullptr, tr("This is non-deterministic wallet"),
                          tr("Wallet is non-deterministic and has no seed."), QMessageBox::Ok);
    return;
  }

  MnemonicDialog dlg(accountKeys, this);
  dlg.exec();
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
  m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_mainToolBar->setIconSize(QSize(20, 20));
  addToolBar(Qt::TopToolBarArea, m_mainToolBar);

  m_navActionGroup = new QActionGroup(this);
  m_navActionGroup->setExclusive(true);

  struct NavSpec {
    QAction** outAction;
    QPushButton* button;
    QString text;
    QString iconPath;
  };
  NavSpec specs[] = {
    { &m_overviewNavAction, m_ui->m_overviewButton,      tr("Overview"), ":icons/overview" },
    { &m_sendNavAction,     m_ui->m_sendButton,          tr("Send"),     ":icons/send" },
    { &m_receiveNavAction,  m_ui->m_receiveButton,       tr("Receive"),  ":icons/receive" },
    { &m_historyNavAction,  m_ui->m_transactionsButton,  tr("History"),  ":icons/transactions" },
    { &m_contactsNavAction, m_ui->m_addressBookButton,   tr("Contacts"), ":icons/address_book" },
    { &m_explorerNavAction, m_ui->m_blockExplorerButton, tr("Explorer"), ":icons/explorer" },
  };
  for (const NavSpec& spec : specs) {
    QAction* action = new QAction(spec.text, this);
    action->setCheckable(true);
    action->setIcon(QIcon(spec.iconPath));
    m_navActionGroup->addAction(action);
    m_mainToolBar->addAction(action);
    QPushButton* button = spec.button;
    connect(action, &QAction::triggered, this, [button]() { button->setChecked(true); });
    connect(button, &QPushButton::toggled, action, &QAction::setChecked);
    action->setEnabled(button->isEnabled());
    *spec.outAction = action;
  }

  for (QAbstractButton* button : m_ui->m_toolButtonGroup->buttons()) {
    button->hide();
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

  QLabel* lockedCaption = new QLabel(tr("Locked"), balanceFrame);
  lockedCaption->setObjectName("m_lockedCaption");
  m_sidebarLockedLabel = new QLabel(balanceFrame);
  m_sidebarLockedLabel->setObjectName("m_lockedValue");
  m_sidebarLockedLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  QLabel* totalCaption = new QLabel(tr("Total"), balanceFrame);
  totalCaption->setObjectName("m_totalCaption");
  m_sidebarTotalLabel = new QLabel(balanceFrame);
  m_sidebarTotalLabel->setObjectName("m_totalValue");
  m_sidebarTotalLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

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
    // Remove any QSpacerItem left over from the legacy .ui layout so the sidebar
    // (and its "+ New address" button at the bottom) can grow to fill the frame.
    for (int i = toolLayout->count() - 1; i >= 0; --i) {
      QLayoutItem* item = toolLayout->itemAt(i);
      if (item != nullptr && item->spacerItem() != nullptr) {
        toolLayout->takeAt(i);
        delete item;
      }
    }
    toolLayout->insertWidget(0, m_addressSidebar, 1);
  }

  m_ui->m_toolFrame->setMinimumWidth(240);

  connect(m_addressSidebar, &AddressSidebar::addAddressRequestedSignal, this, &MainWindow::createAddressRequested);
  connect(m_addressSidebar, &AddressSidebar::copyAddressRequestedSignal, this, &MainWindow::copyAddressFromCard);
  connect(m_addressSidebar, &AddressSidebar::showQrRequestedSignal, this, &MainWindow::showQrFromCard);
  connect(m_addressSidebar, &AddressSidebar::showKeysRequestedSignal, this, &MainWindow::showKeysFromCard);
  connect(m_addressSidebar, &AddressSidebar::exportTrackingKeyRequestedSignal, this, &MainWindow::exportTrackingKeyFromCard);
  connect(m_addressSidebar, &AddressSidebar::renameRequestedSignal, this, &MainWindow::renameAddressFromCard);
  connect(m_addressSidebar, &AddressSidebar::sendFromRequestedSignal, this, &MainWindow::sendFromCard);
  connect(m_addressSidebar, &AddressSidebar::balanceProofRequestedSignal, this, &MainWindow::balanceProofFromCard);
  connect(m_addressSidebar, &AddressSidebar::deleteRequestedSignal, this, &MainWindow::deleteAddressFromCard);
}

void MainWindow::installSidebarBalance() {
  if (m_sidebarTotalLabel == nullptr || m_sidebarLockedLabel == nullptr) {
    return;
  }

  auto updateBalance = [this]() {
    const quint64 actual = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_ACTUAL_BALANCE).value<quint64>();
    const quint64 pending = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_PENDING_BALANCE).value<quint64>();
    const quint64 total = m_walletStateModel->index(0, 0).data(WalletStateModel::ROLE_TOTAL_BALANCE).value<quint64>();
    const quint64 locked = (total >= actual) ? (total - actual) : pending;
    if (m_cryptoNoteAdapter != nullptr) {
      m_sidebarTotalLabel->setText(m_cryptoNoteAdapter->formatUnsignedAmount(total));
      m_sidebarLockedLabel->setText(m_cryptoNoteAdapter->formatUnsignedAmount(locked));
    } else {
      m_sidebarTotalLabel->setText(QString::number(total));
      m_sidebarLockedLabel->setText(QString::number(locked));
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

  if (importKeyAction != nullptr) walletMenu->removeAction(importKeyAction);
  if (importSeedAction != nullptr) walletMenu->removeAction(importSeedAction);
  // Export key, Export tracking key and Prove balance become per-address operations
  // reachable from the address card's Advanced menu; remove them from the Wallet menu.
  if (exportKeyAction != nullptr) walletMenu->removeAction(exportKeyAction);
  if (exportTrackingKeyAction != nullptr) walletMenu->removeAction(exportTrackingKeyAction);
  if (balanceProofAction != nullptr) walletMenu->removeAction(balanceProofAction);

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
