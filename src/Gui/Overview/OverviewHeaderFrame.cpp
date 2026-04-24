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

#include <QAbstractItemModel>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "OverviewHeaderFrame.h"
#include "Gui/Common/WalletTextLabel.h"
#include "Models/NodeStateModel.h"
#include "Settings/Settings.h"
#include "Style/Style.h"
#include "ui_OverviewHeaderFrame.h"

namespace WalletGui {

namespace {

const char OVERVIEW_HEADER_STYLE_SHEET_TEMPLATE[] =
  "* {"
    "font-family: %fontFamily%;"
  "}"

  // The header sits flush with the rest of the overview — same panel bg —
  // and is separated from the transaction list below by a single hairline.
  // The old backgroundColorGray gave it a "secondary surface" look that
  // clashed with the toolbar/sidebar already using the gray shade.
  "WalletGui--OverviewHeaderFrame {"
    "border: none;"
    "border-bottom: 1px solid %borderColor%;"
    "background-color: %panelBackgroundColor%;"
    "min-height: 140px;"
    "max-height: 150px;"
  "}"

  "WalletGui--OverviewHeaderFrame #m_overviewNetworkColumn {"
    "border: none;"
    "border-right: 1px solid %borderColor%;"
  "}"

  "WalletGui--OverviewHeaderFrame QLabel[role=\"caption\"] {"
    "color: %fontColorGray%;"
    "font-size: %fontSizeTiny%;"
  "}"

  "WalletGui--OverviewHeaderFrame QLabel[role=\"value\"] {"
    "color: %primaryTextColor%;"
  "}";

// Local/loopback detection for RPC hosts — used to tell a local daemon
// (127.0.0.1 / localhost / ::1) apart from a remote one.
bool isLoopbackHost(const QString& _host) {
  if (_host.isEmpty()) {
    return false;
  }
  const QString lower = _host.toLower();
  return lower == QLatin1String("127.0.0.1")
      || lower == QLatin1String("localhost")
      || lower == QLatin1String("::1");
}

// "Time ago" formatter for block timestamps.
QString formatAge(qint64 _secondsAgo) {
  if (_secondsAgo < 0) {
    return QObject::tr("just now");
  }
  if (_secondsAgo < 60) {
    return QObject::tr("%n second(s) ago", nullptr, static_cast<int>(_secondsAgo));
  }
  if (_secondsAgo < 60 * 60) {
    return QObject::tr("%n minute(s) ago", nullptr, static_cast<int>(_secondsAgo / 60));
  }
  if (_secondsAgo < 24 * 60 * 60) {
    return QObject::tr("%n hour(s) ago", nullptr, static_cast<int>(_secondsAgo / 3600));
  }
  return QObject::tr("%n day(s) ago", nullptr, static_cast<int>(_secondsAgo / 86400));
}

// Build a single captioned row: caption on the left, value on the right, stretched.
void addStatRow(QGridLayout* _grid, int _row, const QString& _caption, QLabel* _valueLabel) {
  QLabel* captionLabel = new QLabel(_caption, _valueLabel->parentWidget());
  captionLabel->setProperty("role", "caption");
  _valueLabel->setProperty("role", "value");
  _valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  _grid->addWidget(captionLabel, _row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  _grid->addWidget(_valueLabel, _row, 1, Qt::AlignLeft | Qt::AlignVCenter);
}

}

OverviewHeaderFrame::OverviewHeaderFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::OverviewHeaderFrame),
  m_cryptoNoteAdapter(nullptr), m_mainWindow(nullptr), m_nodeStateModel(nullptr),
  m_lastBlockAgeTimer(new QTimer(this)),
  m_connectionStateLabel(nullptr), m_nodeTypeLabel(nullptr), m_peerCountLabel(nullptr),
  m_networkHashrateLabel(nullptr), m_heightLabel(nullptr), m_lastBlockAgeLabel(nullptr),
  m_difficultyLabel(nullptr) {
  m_ui->setupUi(this);
  buildUi();
  setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(OVERVIEW_HEADER_STYLE_SHEET_TEMPLATE));

  // Tick the "last block N min ago" label even when no model updates arrive —
  // we want it to roll forward when the chain is idle.
  m_lastBlockAgeTimer->setInterval(10 * 1000);
  connect(m_lastBlockAgeTimer, &QTimer::timeout, this, &OverviewHeaderFrame::refreshLastBlockAge);
  m_lastBlockAgeTimer->start();
}

OverviewHeaderFrame::~OverviewHeaderFrame() {
  if (m_cryptoNoteAdapter != nullptr) {
    m_cryptoNoteAdapter->removeObserver(this);
  }
}

void OverviewHeaderFrame::buildUi() {
  // Each column is a small grid (caption | value) under a bold section header.
  auto makeColumn = [this](const QString& _title, QGridLayout** _outGrid) -> QFrame* {
    QFrame* column = new QFrame(this);
    column->setFrameShape(QFrame::NoFrame);
    QVBoxLayout* layout = new QVBoxLayout(column);
    layout->setContentsMargins(30, 15, 30, 15);
    layout->setSpacing(6);

    WalletHeaderLabel* title = new WalletHeaderLabel(column);
    title->setText(_title);
    title->setIndent(0);
    layout->addWidget(title);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
    layout->addLayout(grid);
    layout->addStretch();

    *_outGrid = grid;
    return column;
  };

  QGridLayout* networkGrid = nullptr;
  QFrame* networkColumn = makeColumn(tr("Network"), &networkGrid);
  networkColumn->setObjectName("m_overviewNetworkColumn");

  m_connectionStateLabel = new QLabel(networkColumn);
  m_nodeTypeLabel = new QLabel(networkColumn);
  m_peerCountLabel = new QLabel(networkColumn);
  m_networkHashrateLabel = new QLabel(networkColumn);
  addStatRow(networkGrid, 0, tr("Status:"), m_connectionStateLabel);
  addStatRow(networkGrid, 1, tr("Node:"), m_nodeTypeLabel);
  addStatRow(networkGrid, 2, tr("Peers:"), m_peerCountLabel);
  addStatRow(networkGrid, 3, tr("Hashrate:"), m_networkHashrateLabel);

  QGridLayout* chainGrid = nullptr;
  QFrame* chainColumn = makeColumn(tr("Blockchain"), &chainGrid);
  chainColumn->setObjectName("m_overviewChainColumn");

  m_heightLabel = new QLabel(chainColumn);
  m_lastBlockAgeLabel = new QLabel(chainColumn);
  m_difficultyLabel = new QLabel(chainColumn);
  addStatRow(chainGrid, 0, tr("Height:"), m_heightLabel);
  addStatRow(chainGrid, 1, tr("Last block:"), m_lastBlockAgeLabel);
  addStatRow(chainGrid, 2, tr("Difficulty:"), m_difficultyLabel);

  m_ui->m_rootLayout->addWidget(networkColumn, 1);
  m_ui->m_rootLayout->addWidget(chainColumn, 1);
}

void OverviewHeaderFrame::updateStyle() {
  setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(OVERVIEW_HEADER_STYLE_SHEET_TEMPLATE));
}

void OverviewHeaderFrame::setCryptoNoteAdapter(ICryptoNoteAdapter* _cryptoNoteAdapter) {
  m_cryptoNoteAdapter = _cryptoNoteAdapter;
  m_cryptoNoteAdapter->addObserver(this);
  refreshFromModel();
}

void OverviewHeaderFrame::setMainWindow(QWidget* _mainWindow) {
  m_mainWindow = _mainWindow;
}

void OverviewHeaderFrame::setNodeStateModel(QAbstractItemModel* _model) {
  if (m_nodeStateModel == _model) {
    return;
  }
  if (m_nodeStateModel != nullptr) {
    disconnect(m_nodeStateModel, &QAbstractItemModel::dataChanged, this, nullptr);
  }
  m_nodeStateModel = _model;
  if (m_nodeStateModel != nullptr) {
    connect(m_nodeStateModel, &QAbstractItemModel::dataChanged,
            this, &OverviewHeaderFrame::nodeStateModelDataChanged);
  }
  refreshFromModel();
}

void OverviewHeaderFrame::cryptoNoteAdapterInitCompleted(int _status) {
  Q_UNUSED(_status);
  refreshFromModel();
}

void OverviewHeaderFrame::cryptoNoteAdapterDeinitCompleted() {
  refreshFromModel();
}

void OverviewHeaderFrame::refreshFromModel() {
  if (m_nodeStateModel == nullptr || m_connectionStateLabel == nullptr) {
    return;
  }

  const QModelIndex root = m_nodeStateModel->index(0, 0);
  if (!root.isValid()) {
    return;
  }

  // --- Network ---
  const bool connected = m_nodeStateModel->index(0, NodeStateModel::COLUMN_CONNECTION_STATE)
                             .data(NodeStateModel::ROLE_CONNECTION_STATE).toBool();
  m_connectionStateLabel->setText(connected ? tr("Connected") : tr("Disconnected"));

  // Node label: NodeType only tells us IN_PROCESS vs RPC. In AUTO mode the
  // adapter tries a local RPC daemon first and falls back to the built-in
  // node, so "RPC on 127.0.0.1" needs to be labelled "Local daemon" rather
  // than "Remote RPC". Inspect the live host, not Settings::getRemoteRpcUrl()
  // (which only holds the user's remote preference, not the actual target).
  INodeAdapter* nodeAdapter = m_cryptoNoteAdapter != nullptr ? m_cryptoNoteAdapter->getNodeAdapter() : nullptr;
  const NodeType nodeType = nodeAdapter != nullptr ? nodeAdapter->getNodeType() : NodeType::UNKNOWN;
  QString nodeDescription;
  if (nodeType == NodeType::IN_PROCESS) {
    nodeDescription = tr("Built-in node");
  } else if (nodeType == NodeType::RPC && nodeAdapter != nullptr) {
    const QString host = nodeAdapter->getNodeHost();
    const quint16 port = nodeAdapter->getNodePort();
    if (isLoopbackHost(host)) {
      nodeDescription = port != 0 ? tr("Local daemon (%1:%2)").arg(host).arg(port) : tr("Local daemon");
    } else if (!host.isEmpty()) {
      nodeDescription = port != 0 ? tr("Remote RPC (%1:%2)").arg(host).arg(port) : tr("Remote RPC (%1)").arg(host);
    } else {
      nodeDescription = tr("Remote RPC");
    }
  } else {
    nodeDescription = tr("Unknown");
  }
  m_nodeTypeLabel->setText(nodeDescription);

  const quint64 peers = m_nodeStateModel->index(0, NodeStateModel::COLUMN_PEER_COUNT)
                            .data(NodeStateModel::ROLE_PEER_COUNT).toULongLong();
  m_peerCountLabel->setText(QString::number(peers));

  // --- Blockchain ---
  // Pull the last block info straight from the adapter rather than the model.
  // The model's cached BlockHeaderInfo only refreshes when the node emits
  // localBlockchainUpdated (i.e. on new blocks); for an already-synced
  // built-in node opened on a quiet chain tip, that signal never fires and
  // the cached difficulty/timestamp read from before init completes can be
  // empty. Hitting the adapter here guarantees fresh numbers on every tick.
  CryptoNote::BlockHeaderInfo lastBlock{};
  if (nodeAdapter != nullptr) {
    lastBlock = nodeAdapter->getLastLocalBlockInfo();
  }
  const quint64 localTop = static_cast<quint64>(lastBlock.index);
  const quint64 knownCount = m_nodeStateModel->index(0, NodeStateModel::COLUMN_KNOWN_BLOCK_COUNT)
                                 .data(NodeStateModel::ROLE_KNOWN_BLOCK_COUNT).toULongLong();
  // knownCount is count (genesis-inclusive); top index is count - 1.
  const quint64 networkTop = knownCount > 0 ? knownCount - 1 : 0;
  if (networkTop > localTop && knownCount > 0) {
    m_heightLabel->setText(tr("%1 / %2 (syncing)").arg(localTop).arg(networkTop));
  } else {
    m_heightLabel->setText(QString::number(localTop));
  }

  const quint64 difficulty = static_cast<quint64>(lastBlock.difficulty);
  m_difficultyLabel->setText(difficulty == 0 ? tr("—") : QString::number(difficulty));

  // Hashrate = difficulty / blockTimeTargetSeconds. Format with SI prefix.
  const quint64 targetSeconds = m_cryptoNoteAdapter != nullptr
      ? qMax<quint64>(1, m_cryptoNoteAdapter->getTargetTime()) : 1;
  const quint64 hashrate = difficulty / targetSeconds;
  m_networkHashrateLabel->setText(hashrate == 0 ? tr("—") : NodeStateModel::formatHashRate(hashrate));

  refreshLastBlockAge();
}

void OverviewHeaderFrame::refreshLastBlockAge() {
  if (m_lastBlockAgeLabel == nullptr) {
    return;
  }
  // Poll the adapter rather than the model cache — same rationale as in
  // refreshFromModel(): on a quiet tip the model can hold a zero timestamp
  // because localBlockchainUpdated() only fires on new blocks.
  INodeAdapter* nodeAdapter = m_cryptoNoteAdapter != nullptr ? m_cryptoNoteAdapter->getNodeAdapter() : nullptr;
  if (nodeAdapter == nullptr) {
    m_lastBlockAgeLabel->setText(tr("Unknown"));
    return;
  }
  const quint64 ts = static_cast<quint64>(nodeAdapter->getLastLocalBlockInfo().timestamp);
  if (ts == 0) {
    m_lastBlockAgeLabel->setText(tr("Unknown"));
    return;
  }
  const qint64 now = QDateTime::currentSecsSinceEpoch();
  m_lastBlockAgeLabel->setText(formatAge(now - static_cast<qint64>(ts)));
}

void OverviewHeaderFrame::nodeStateModelDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                                                    const QVector<int>& _roles) {
  Q_UNUSED(_topLeft);
  Q_UNUSED(_bottomRight);
  Q_UNUSED(_roles);
  refreshFromModel();
}

}
