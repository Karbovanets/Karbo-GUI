// Copyright (c) 2015-2017, The Bytecoin developers
// Copyright (c) 2017-2019, The Karbo developers
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

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QSemaphore>

#include <boost/program_options.hpp>
#include <boost/any.hpp>

#include <functional>

#include "INode.h"
#include "BlockchainExplorer/BlockchainExplorer.h"
#include "InProcessNodeWorker.h"
#include "GuardExecutor.h"
#include "Settings/Settings.h"
#include "WalletLogger/WalletLogger.h"
#include "BlockChainExplorerAdapter.h"
#include "WalletGreenAdapter.h"

#include "Checkpoints/CheckpointsData.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/ICore.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "InProcessNode/InProcessNode.h"
#include "Logging/LoggerManager.h"
#include "P2p/NetNode.h"
#include "P2p/NetNodeConfig.h"
#include "System/Dispatcher.h"
#include "System/Event.h"

namespace WalletGui {

namespace {

std::vector<std::string> convertStringListToVector(const QStringList& list) {
  std::vector<std::string> result;
  for (const QString& item : list) {
    result.push_back(item.toStdString());
  }

  return result;
}

CryptoNote::NetNodeConfig makeNetNodeConfig() {
  CryptoNote::NetNodeConfig config;
  boost::program_options::variables_map options;
  boost::any p2pBindIp = Settings::instance().getP2pBindIp().toStdString();
  boost::any p2pBindPort = static_cast<uint16_t>(Settings::instance().getP2pBindPort());
  boost::any p2pExternalPort = static_cast<uint16_t>(Settings::instance().getP2pExternalPort());
  boost::any p2pAllowLocalIp = Settings::instance().hasAllowLocalIpOption();
  boost::any dataDir = std::string(Settings::instance().getDataDir().absolutePath().toLocal8Bit().data());
  boost::any hideMyPort = Settings::instance().hasHideMyPortOption();
  options.insert(std::make_pair("p2p-bind-ip", boost::program_options::variable_value(p2pBindIp, false)));
  options.insert(std::make_pair("p2p-bind-port", boost::program_options::variable_value(p2pBindPort, false)));
  options.insert(std::make_pair("p2p-external-port", boost::program_options::variable_value(p2pExternalPort, false)));
  options.insert(std::make_pair("allow-local-ip", boost::program_options::variable_value(p2pAllowLocalIp, false)));
  std::vector<std::string> peerList = convertStringListToVector(Settings::instance().getPeers());
  if (!peerList.empty()) {
    options.insert(std::make_pair("add-peer", boost::program_options::variable_value(peerList, false)));
  }

  std::vector<std::string> priorityNodeList = convertStringListToVector(Settings::instance().getPriorityNodes());
  if (!priorityNodeList.empty()) {
    options.insert(std::make_pair("add-priority-node", boost::program_options::variable_value(priorityNodeList, false)));
  }

  std::vector<std::string> exclusiveNodeList = convertStringListToVector(Settings::instance().getExclusiveNodes());
  if (!exclusiveNodeList.empty()) {
    options.insert(std::make_pair("add-exclusive-node", boost::program_options::variable_value(exclusiveNodeList, false)));
  }

  std::vector<std::string> seedNodeList = convertStringListToVector(Settings::instance().getSeedNodes());
  if (!seedNodeList.empty()) {
    options.insert(std::make_pair("seed-node", boost::program_options::variable_value(seedNodeList, false)));
  }

  options.insert(std::make_pair("hide-my-port", boost::program_options::variable_value(hideMyPort, false)));
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  config.init(options);
  config.setTestnet(Settings::instance().isTestnet());
  return config;
}

}

InProcessNodeWorker::InProcessNodeWorker(const CryptoNote::Currency& _currency, Logging::ILogger& _loggerManager,
  Logging::ILogger& _walletLogger, QObject* _parent) : QObject(_parent), m_currency(_currency), m_loggerManager(_loggerManager), m_walletLogger(_walletLogger),
  m_blockchainExplorerAdapter(nullptr) {
}

InProcessNodeWorker::~InProcessNodeWorker() {
}

void InProcessNodeWorker::init() {
  WalletLogger::debug(tr("[Embedded node] Initializing..."));
  QMetaObject::invokeMethod(this, "initImpl", Qt::QueuedConnection);
}

void InProcessNodeWorker::deinit() {
  WalletLogger::debug(tr("[Embedded node] Deinitializing..."));
  for (auto it = m_observerConnections.begin(); it != m_observerConnections.end(); ++it) {
    removeObserver(it.key());
  }

  m_observerConnections.clear();
  deinitImpl();
}

NodeType InProcessNodeWorker::getNodeType() const {
  return NodeType::IN_PROCESS;
}

QString InProcessNodeWorker::getNodeVersion() const {
  Q_ASSERT(!m_node.isNull());
  return QString::fromStdString(m_node->getNodeVersion());
}

quintptr InProcessNodeWorker::getPeerCount() const {
  Q_ASSERT(!m_node.isNull());
  try {
    return m_node->getPeerCount();
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Get peer count error: %1").arg(_error.what()));
  }

  return 0;
}

quint32 InProcessNodeWorker::getKnownBlockCount() const {
  Q_ASSERT(!m_node.isNull());
  try {
    return m_node->getKnownBlockCount();
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Get known block count error: %1").arg(_error.what()));
  }

  return 0;
}

quint32 InProcessNodeWorker::getLocalBlockCount() const {
  Q_ASSERT(!m_node.isNull());
  try {
    return m_node->getLocalBlockCount();
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Get local block count error: %1").arg(_error.what()));
  }

  return 0;
}

CryptoNote::BlockHeaderInfo InProcessNodeWorker::getLastLocalBlockInfo() const {
  Q_ASSERT(!m_node.isNull());
  try {
    return m_node->getLastLocalBlockHeaderInfo();
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Get last local block timestamp error: %1").arg(_error.what()));
  }

  return CryptoNote::BlockHeaderInfo();
}

QString InProcessNodeWorker::getNodeHost() const {
  Q_ASSERT(!m_node.isNull());
  return "";
}

quint16 InProcessNodeWorker::getNodePort() const {
  Q_ASSERT(!m_node.isNull());
  return 0;
}

quint64 InProcessNodeWorker::getMinimalFee() const {
  Q_ASSERT(!m_node.isNull());
  try {
    return m_node->getMinimalFee();
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Get minimal fee error: %1").arg(_error.what()));
  }

  return m_currency.minimumFee();
}

quint64 InProcessNodeWorker::getNodeFee() const {
  Q_ASSERT(!m_node.isNull());
  return m_node->feeAmount();
}

QString InProcessNodeWorker::getNodeFeeAddress() const {
  Q_ASSERT(!m_node.isNull());
  return QString::fromStdString(m_node->feeAddress());
}

void InProcessNodeWorker::addObserver(INodeAdapterObserver* _observer) {
  QObject* observer = dynamic_cast<QObject*>(_observer);
  Q_ASSERT(observer != nullptr);
  m_observerConnections[_observer] << connect(this, SIGNAL(initCompletedSignal(int)), observer, SLOT(initCompleted(int)), Qt::QueuedConnection);
  m_observerConnections[_observer] << connect(this, SIGNAL(deinitCompletedSignal()), observer, SLOT(deinitCompleted()), Qt::QueuedConnection);
  m_observerConnections[_observer] << connect(this, SIGNAL(peerCountUpdatedSignal(quintptr)), observer, SLOT(peerCountUpdated(quintptr)), Qt::QueuedConnection);
  m_observerConnections[_observer] << connect(this, SIGNAL(localBlockchainUpdatedSignal(CryptoNote::BlockHeaderInfo)), observer, SLOT(localBlockchainUpdated(CryptoNote::BlockHeaderInfo)), Qt::QueuedConnection);
  m_observerConnections[_observer] << connect(this, SIGNAL(lastKnownBlockHeightUpdatedSignal(quint32)), observer, SLOT(lastKnownBlockHeightUpdated(quint32)), Qt::QueuedConnection);
}

void InProcessNodeWorker::removeObserver(INodeAdapterObserver* _observer) {
  if (!m_observerConnections.contains(_observer) || m_observerConnections.value(_observer).isEmpty()) {
    return;
  }

  for (const auto& connection : m_observerConnections[_observer]) {
    disconnect(connection);
  }

  m_observerConnections[_observer].clear();
}

WalletGui::IBlockChainExplorerAdapter* InProcessNodeWorker::getBlockChainExplorerAdapter() {
  Q_ASSERT(!m_node.isNull());
  return m_blockchainExplorerAdapter;
}

IWalletAdapter* InProcessNodeWorker::getWalletAdapter() {
  Q_ASSERT(!m_node.isNull());
  IWalletAdapter* walletAdapter = new WalletGreenAdapter(m_currency, *m_node, m_walletLogger, nullptr);
  return walletAdapter;
}

void InProcessNodeWorker::getAccountNumber(const QString& _address, AccountNumberCallback _callback) {
  if (m_node.isNull()) {
    _callback(std::make_error_code(std::errc::operation_canceled), QString());
    return;
  }
  // The node fills the std::string via reference, so it must outlive the
  // async call. The shared_ptr keeps it alive until the callback fires.
  auto result = std::make_shared<std::string>();
  const std::string addr = _address.toStdString();
  m_node->getAccountNumber(addr, *result, [result, _callback](std::error_code _ec) {
    _callback(_ec, _ec ? QString() : QString::fromStdString(*result));
  });
}

void InProcessNodeWorker::resolveAccountNumber(const QString& _accountNumber, AccountNumberCallback _callback) {
  if (m_node.isNull()) {
    _callback(std::make_error_code(std::errc::operation_canceled), QString());
    return;
  }
  auto result = std::make_shared<std::string>();
  const std::string acct = _accountNumber.toStdString();
  m_node->resolveAccountNumber(acct, *result, [result, _callback](std::error_code _ec) {
    _callback(_ec, _ec ? QString() : QString::fromStdString(*result));
  });
}

void InProcessNodeWorker::peerCountUpdated(size_t _count) {
  WalletLogger::debug(tr("[Embedded node] Event: Peer count updated: %1").arg(_count));
  Q_EMIT peerCountUpdatedSignal(_count);
}

void InProcessNodeWorker::localBlockchainUpdated(uint32_t _height) {
  WalletLogger::debug(tr("[Embedded node] Event: Local blockchain updated: %1").arg(_height));
  Q_EMIT localBlockchainUpdatedSignal(m_node->getLastLocalBlockHeaderInfo());
}

void InProcessNodeWorker::lastKnownBlockHeightUpdated(uint32_t _height) {
  WalletLogger::debug(tr("[Embedded node] Event: Known blockchain updated: %1").arg(_height));
  Q_EMIT lastKnownBlockHeightUpdatedSignal(_height);
}

void InProcessNodeWorker::initImpl() {
  GuardExecutor executor(nullptr, [this]() {
    m_node.reset();
    m_nodeServer.reset();
    m_protocolHandler.reset();
    m_core.reset();
    m_dispatcher.reset();
  });

  InitStatus status = initCore();
  if (status != INIT_SUCCESS) {
    Q_EMIT initCompletedSignal(status);
    return;
  }

  status = initNodeServer();
  if (status != INIT_SUCCESS) {
    //m_core->save();
    Q_EMIT initCompletedSignal(status);
    return;
  }

  status = initInProcessNode();
  if (status != INIT_SUCCESS) {
    m_nodeServer->deinit();
    //m_core->save();
    Q_EMIT initCompletedSignal(status);
    return;
  }

  if (Settings::instance().isBlockchainExplorerEnabled()) {
    m_blockchainExplorerAdapter = nullptr;
    BlockChainExplorerAdapter* blockchainExplorerAdapter = new BlockChainExplorerAdapter(*m_node, m_loggerManager, nullptr);
    blockchainExplorerAdapter->moveToThread(qApp->thread());
    m_blockchainExplorerAdapter = blockchainExplorerAdapter;
  }

  Q_EMIT initCompletedSignal(INIT_SUCCESS);
  m_nodeServer->run();
  try {
    m_node->removeObserver(this);
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] Remove observer error: %1").arg(_error.what()));
  }

  m_dispatcher->yield();
  m_nodeServer->deinit();
  m_core->deinit();
  m_node->shutdown();
  m_dispatcher->yield();
  Q_EMIT deinitCompletedSignal();
}

void InProcessNodeWorker::deinitImpl() {
  if (!m_nodeServer.isNull()) {
    m_nodeServer->sendStopSignal();
  }
}

INodeAdapter::InitStatus InProcessNodeWorker::initCore() {
  Q_ASSERT(m_core.isNull());
  WalletLogger::debug(tr("[Embedded node] Core initializing..."));
  try {

    m_dispatcher.reset(new System::Dispatcher);

    CryptoNote::Checkpoints checkpoints(m_loggerManager);

    if (!Settings::instance().isTestnet()) {
      for (const CryptoNote::CheckpointData& checkpoint : CryptoNote::CHECKPOINTS) {
        checkpoints.add_checkpoint(checkpoint.height, checkpoint.blockId);
      }
      checkpoints.load_checkpoints_from_dns();
    }

    m_core.reset(new CryptoNote::Core(m_currency, nullptr, m_loggerManager, *m_dispatcher, 0, false));
    m_protocolHandler.reset(new CryptoNote::CryptoNoteProtocolHandler(m_currency, *m_dispatcher, *m_core, nullptr, m_loggerManager));

    if (!Settings::instance().isTestnet()) {
      m_core->set_checkpoints(std::move(checkpoints));
    }

    m_core->set_cryptonote_protocol(m_protocolHandler.data());

    CryptoNote::CoreConfig coreConfig = makeCoreConfig();

    m_core->init(coreConfig, CryptoNote::MinerConfig(), true);

    m_nodeServer.reset(new CryptoNote::NodeServer(*m_dispatcher, *m_protocolHandler, m_loggerManager));
    m_node.reset(new CryptoNote::InProcessNode(*m_core, *m_protocolHandler));
    m_protocolHandler->set_p2p_endpoint(m_nodeServer.data());
  } catch (const std::runtime_error& _error) {
    WalletLogger::critical(tr("Core init error: %1").arg(_error.what()));
    return INIT_CORE_FAILED;
  }

  return INIT_SUCCESS;
}

INodeAdapter::InitStatus InProcessNodeWorker::initNodeServer() {
  try {
    WalletLogger::debug(tr("[Embedded node] Node server initializing..."));
    if (!m_nodeServer->init(makeNetNodeConfig())) {
      WalletLogger::critical(tr("[Embedded node] Node server init error"));
      return INIT_NET_NODE_FAILED;
    }
  } catch (const std::runtime_error& _error) {
    WalletLogger::critical(tr("[Embedded node] Node server init error: %1").arg(_error.what()));
    return INIT_P2P_BIND_FAILED;
  }

  return INIT_SUCCESS;
}

INodeAdapter::InitStatus InProcessNodeWorker::initInProcessNode() {
  QEventLoop waitLoop;
  connect(this, &InProcessNodeWorker::nodeInitCompletedSignal, &waitLoop, &QEventLoop::exit, Qt::QueuedConnection);
  WalletLogger::info(tr("[Embedded node] InProcessNode initializing..."));
  std::error_code initResult;
  m_node->init([this, &initResult](std::error_code _errorCode) {
    WalletLogger::info(tr("[Embedded node] InProcessNode init result: %1").arg(_errorCode.value()));
    initResult = _errorCode;
    Q_EMIT nodeInitCompletedSignal(_errorCode.value());
  });

  WalletLogger::info(tr("[Embedded node] Waiting..."));
  waitLoop.exec();
  if (initResult) {
    return INIT_NODE_FAILED;
  }

  try {
    m_node->addObserver(this);
  } catch (const std::exception& _error) {
    WalletLogger::critical(tr("[Embedded node] InProcessNode add observer error: %1").arg(_error.what()));
    m_node->shutdown();
    return INIT_NODE_FAILED;
  }

  return INIT_SUCCESS;
}

CryptoNote::CoreConfig InProcessNodeWorker::makeCoreConfig() const {
  CryptoNote::CoreConfig config;
  boost::program_options::variables_map options;
  boost::any dataDir = std::string(Settings::instance().getDataDir().absolutePath().toLocal8Bit().data());
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  config.init(options);
  return config;
}

}
