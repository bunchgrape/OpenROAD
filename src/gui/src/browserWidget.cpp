/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "browserWidget.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QEvent>
#include <QMouseEvent>

#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbInst*);
Q_DECLARE_METATYPE(odb::dbModule*);
Q_DECLARE_METATYPE(QStandardItem*);

namespace gui {

///////

BrowserWidget::BrowserWidget(const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>& modulesettings,
                             QWidget* parent)
    : QDockWidget("Hierarchy Browser", parent),
      block_(nullptr),
      modulesettings_(modulesettings),
      view_(new QTreeView(this)),
      model_(new QStandardItemModel(this)),
      ignore_selection_(false),
      menu_(new QMenu(this))
{
  setObjectName("hierarchy_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Instance", "Master", "Instances", "Macros", "Modules", "Area"});
  view_->setModel(model_);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);

  view_->viewport()->installEventFilter(this);

  makeMenu();

  QHeaderView* header = view_->header();
  header->setSectionsMovable(true);
  header->setStretchLastSection(false);
  header->setSectionResizeMode(Instance, QHeaderView::Interactive);
  header->setSectionResizeMode(Master, QHeaderView::Interactive);
  header->setSectionResizeMode(Instances, QHeaderView::Interactive);
  header->setSectionResizeMode(Macros, QHeaderView::Interactive);
  header->setSectionResizeMode(Modules, QHeaderView::Interactive);
  header->setSectionResizeMode(Area, QHeaderView::Interactive);

  setWidget(view_);

  connect(view_,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(clicked(const QModelIndex&)));

  connect(view_,
          SIGNAL(customContextMenuRequested(const QPoint&)),
          this,
          SLOT(itemContextMenu(const QPoint&)));

  connect(view_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(model_,
          SIGNAL(itemChanged(QStandardItem*)),
          this,
          SLOT(itemChanged(QStandardItem*)));
}

void BrowserWidget::makeMenu()
{
  connect(menu_->addAction("Select"),
          &QAction::triggered,
          [&](bool) {
            emit select({menu_item_});
          });
  connect(menu_->addAction("Select children"),
          &QAction::triggered,
          [&](bool) {
            emit select(getMenuItemChildren());
          });
  connect(menu_->addAction("Select all"),
          &QAction::triggered,
          [&](bool) {
            auto children = getMenuItemChildren();
            children.insert(menu_item_);
            emit select(children);
          });
  connect(menu_->addAction("Remove from selected"),
          &QAction::triggered,
          [&](bool) {
            emit removeSelect(menu_item_);
          });

  menu_->addSeparator();

  connect(menu_->addAction("Highlight"),
          &QAction::triggered,
          [&](bool) {
            emit highlight({menu_item_});
          });
  connect(menu_->addAction("Highlight children"),
          &QAction::triggered,
          [&](bool) {
            emit highlight(getMenuItemChildren());
          });
  connect(menu_->addAction("Highlight all"),
          &QAction::triggered,
          [&](bool) {
            auto children = getMenuItemChildren();
            children.insert(menu_item_);
            emit highlight(children);
          });
  connect(menu_->addAction("Remove from highlight"),
          &QAction::triggered,
          [&](bool) {
            emit removeHighlight(menu_item_);
          });

  menu_->addSeparator();

  connect(menu_->addAction("Change color"),
          &QAction::triggered,
          [&](bool) {
            auto* module = std::any_cast<odb::dbModule*>(menu_item_.getObject());
            if (module == nullptr) {
              return;
            }

            auto& setting = modulesettings_.at(module);
            QColor color = setting.color;

            color = QColorDialog::getColor(color, this, "Module color", QColorDialog::ShowAlphaChannel);
            if (color.isValid()) {
              emit updateModuleColor(module, color);
              modulesmap_[module]->setIcon(makeModuleIcon(color));
            }
          });
}

void BrowserWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  view_->header()->restoreState(settings->value("headers", view_->header()->saveState()).toByteArray());
  settings->endGroup();
}

void BrowserWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  settings->setValue("headers", view_->header()->saveState());
  settings->endGroup();
}

Selected BrowserWidget::getSelectedFromIndex(const QModelIndex& index)
{
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant data = item->data();
  if (data.isValid()) {
    auto* ref = data.value<QStandardItem*>();
    if (ref != nullptr) {
      data = ref->data();
    }

    auto* gui = Gui::get();
    auto* inst = data.value<odb::dbInst*>();
    if (inst != nullptr) {
      return gui->makeSelected(inst);
    } else {
      auto* module = data.value<odb::dbModule*>();
      if (module != nullptr) {
        return gui->makeSelected(module);
      }
    }
  }

  return Selected();
}

void BrowserWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  auto indexes = selected.indexes();
  if (indexes.isEmpty()) {
    return;
  }

  clicked(indexes.first());
}

void BrowserWidget::clicked(const QModelIndex& index)
{
  if (ignore_selection_) {
    return;
  }

  Selected sel = getSelectedFromIndex(index);

  if (sel) {
    emit select({sel});
  }
}

void BrowserWidget::setBlock(odb::dbBlock* block)
{
  block_ = block;
  addOwner(block_);
  updateModel();
}

void BrowserWidget::showEvent(QShowEvent* event)
{
  addOwner(block_);
  updateModel();
}

void BrowserWidget::hideEvent(QHideEvent* event)
{
  removeOwner();
  clearModel();
}

void BrowserWidget::updateModel()
{
  clearModel();

  if (block_ == nullptr) {
    return;
  }

  auto* root = model_->invisibleRootItem();
  addModuleItem(block_->getTopModule(), root, true);

  QStandardItem* physical = new QStandardItem("Physical only");
  physical->setEditable(false);
  physical->setSelectable(false);
  ModuleStats stats;
  for (auto* inst : block_->getInsts()) {
    if (inst->getModule() != nullptr) {
      continue;
    }

    stats += addInstanceItem(inst, physical);
  }
  makeRowItems(physical, "", stats, root, true);

  view_->header()->resizeSections(QHeaderView::ResizeToContents);
}

void BrowserWidget::clearModel()
{
  model_->removeRows(0, model_->rowCount());
  modulesmap_.clear();
}

BrowserWidget::ModuleStats BrowserWidget::populateModule(odb::dbModule* module, QStandardItem* parent)
{
  ModuleStats leaf_stats;

  auto* leafs = new QStandardItem("Leaf instances");
  leafs->setEditable(false);
  leafs->setSelectable(false);
  for (auto* inst : module->getInsts()) {
    leaf_stats += addInstanceItem(inst, leafs);
  }

  ModuleStats stats;
  for (auto* child : module->getChildren()) {
    stats += addModuleItem(child->getMaster(), parent, false);
  }
  stats.resetMacros();
  stats.resetInstances();

  makeRowItems(leafs, "", leaf_stats, parent, true);

  ModuleStats total = leaf_stats + stats;

  return total;
}

BrowserWidget::ModuleStats BrowserWidget::addInstanceItem(odb::dbInst* inst, QStandardItem* parent)
{
  QStandardItem* item = new QStandardItem(inst->getConstName());
  item->setEditable(false);
  item->setSelectable(true);
  item->setData(QVariant::fromValue(inst));

  auto* box = inst->getBBox();

  ModuleStats stats;
  stats.area = box->getDX() * box->getDY();

  if (inst->isBlock()) {
    stats.incrementMacros();
  } else {
    stats.incrementInstances();
  }

  makeRowItems(item, inst->getMaster()->getConstName(), stats, parent, true);

  return stats;
}

BrowserWidget::ModuleStats BrowserWidget::addModuleItem(odb::dbModule* module, QStandardItem* parent, bool expand)
{
  QStandardItem* item = new QStandardItem(QString::fromStdString(module->getHierarchicalName()));
  item->setEditable(false);
  item->setSelectable(true);
  item->setData(QVariant::fromValue(module));

  item->setCheckable(true);
  auto& settings = modulesettings_.at(module);
  item->setCheckState(settings.visible ? Qt::Checked : Qt::Unchecked);
  item->setIcon(makeModuleIcon(settings.color));

  modulesmap_[module] = item;

  ModuleStats stats = populateModule(module, item);

  makeRowItems(item, module->getName(), stats, parent, false);
  stats.resetModules();

  stats.incrementModules();

  if (expand) {
    view_->expand(item->index());
  }

  return stats;
}

void BrowserWidget::makeRowItems(QStandardItem* item,
                                 const std::string& master,
                                 const BrowserWidget::ModuleStats& stats,
                                 QStandardItem* parent,
                                 bool is_leaf) const
{
  double scale_to_um = block_->getDbUnitsPerMicron() * block_->getDbUnitsPerMicron();

  std::string units = "\u03BC"; // mu
  double disp_area = stats.area / scale_to_um;
  if (disp_area > 1e6) {
    disp_area /= (1e3 * 1e3);
    units = "m";
  }

  auto text = fmt::format("{:.3f}", disp_area) + " " + units + "m\u00B2"; // m2

  auto makeDataItem = [item](const QString& text, bool right_align = true) -> QStandardItem* {
    QStandardItem* data_item = new QStandardItem(text);
    data_item->setEditable(false);
    if (right_align) {
      data_item->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    }
    data_item->setData(QVariant::fromValue(item));
    return data_item;
  };

  auto makeHierText = [](int current, int total, bool is_leaf) -> QString {
    if (!is_leaf) {
      return QString::number(current) + "/" + QString::number(total);
    } else {
      return QString::number(total);
    }
  };

  QStandardItem* master_item = makeDataItem(QString::fromStdString(master), false);

  QStandardItem* area = makeDataItem(QString::fromStdString(text));

  QStandardItem* insts = makeDataItem(makeHierText(stats.hier_insts, stats.insts, is_leaf));

  QStandardItem* macros = makeDataItem(makeHierText(stats.hier_macros, stats.macros, is_leaf));

  QStandardItem* modules = makeDataItem(makeHierText(stats.hier_modules, stats.modules, is_leaf));

  parent->appendRow({item, master_item, insts, macros, modules, area});
}

void BrowserWidget::inDbInstCreate(odb::dbInst*)
{
  updateModel();
}

void BrowserWidget::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  updateModel();
}

void BrowserWidget::inDbInstDestroy(odb::dbInst*)
{
  updateModel();
}

void BrowserWidget::inDbInstSwapMasterAfter(odb::dbInst*)
{
  updateModel();
}

void BrowserWidget::itemContextMenu(const QPoint &point)
{
  const QModelIndex index = view_->indexAt(point);

  if (!index.isValid()) {
    return;
  }

  menu_item_ = getSelectedFromIndex(index);

  if (!menu_item_) {
    return;
  }

  menu_->popup(view_->viewport()->mapToGlobal(point));
}

SelectionSet BrowserWidget::getMenuItemChildren()
{
  if (!menu_item_) {
    return {};
  }

  auto* module = std::any_cast<odb::dbModule*>(menu_item_.getObject());
  if (module == nullptr) {
    return {};
  }

  auto* gui = Gui::get();
  SelectionSet children;
  for (auto* child : module->getChildren()) {
    children.insert(gui->makeSelected(child->getMaster()));
  }
  return children;
}

void BrowserWidget::itemChanged(QStandardItem* item)
{
  ignore_selection_ = true;
  const auto state = item->checkState();

  auto* module = item->data().value<odb::dbModule*>();

  if (module == nullptr) {
    return;
  }

  emit updateModuleVisibility(module, state == Qt::Checked);

  // toggle children
  if (state != Qt::PartiallyChecked) {
    for (int r = 0; r < item->rowCount(); r++) {
      QStandardItem* child = item->child(r, Instance);
      if (child->isCheckable()) {
        child->setCheckState(state);
      }
    }
  }

  toggleParent(item);
}

void BrowserWidget::toggleParent(QStandardItem* item)
{
  auto* parent = item->parent();

  if (parent == nullptr) {
    return;
  }

  std::vector<Qt::CheckState> childstates;
  for (int r = 0; r < parent->rowCount(); r++) {
    QStandardItem* child = parent->child(r, Instance);
    if (child->isCheckable()) {
      childstates.push_back(child->checkState());
    }
  }

  const bool all_on = std::all_of(childstates.begin(), childstates.end(), [](Qt::CheckState state) {
    return state == Qt::Checked;
  });
  const bool all_off = std::all_of(childstates.begin(), childstates.end(), [](Qt::CheckState state) {
    return state == Qt::Unchecked;
  });

  if (all_on) {
    parent->setCheckState(Qt::Checked);
  } else if (all_off) {
    parent->setCheckState(Qt::Unchecked);
  } else {
    parent->setCheckState(Qt::PartiallyChecked);
  }
}

bool BrowserWidget::eventFilter(QObject* obj, QEvent* event)
{
  if (obj == view_->viewport()) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
      if (mouse_event->button() == Qt::RightButton) {
        ignore_selection_ = true;
      } else {
        ignore_selection_ = false;
      }
    } else if (event->type() == QEvent::MouseButtonRelease) {
      ignore_selection_ = false;
    } else if (event->type() == QEvent::ContextMenu) {
      // reset because the context menu has popped up.
      ignore_selection_ = false;
    }
  }

  return QDockWidget::eventFilter(obj, event);
}

const QIcon BrowserWidget::makeModuleIcon(const QColor& color)
{
  QPixmap swatch(20, 20);
  swatch.fill(color);

  return QIcon(swatch);
}

}  // namespace gui
