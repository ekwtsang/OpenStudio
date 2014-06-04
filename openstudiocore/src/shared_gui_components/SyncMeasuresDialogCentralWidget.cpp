/**********************************************************************
 *  Copyright (c) 2008-2014, Alliance for Sustainable Energy.  
 *  All rights reserved.
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **********************************************************************/

#include <shared_gui_components/SyncMeasuresDialogCentralWidget.hpp>

#include <shared_gui_components/CollapsibleComponent.hpp>
#include <shared_gui_components/CollapsibleComponentHeader.hpp>
#include <shared_gui_components/CollapsibleComponentList.hpp>
#include <shared_gui_components/Component.hpp>
#include <shared_gui_components/ComponentList.hpp>
#include <shared_gui_components/SyncMeasuresDialog.hpp>

#include <utilities/bcl/BCL.hpp>
#include <utilities/bcl/LocalBCL.hpp>
#include <utilities/bcl/RemoteBCL.hpp>
#include <utilities/data/Attribute.hpp>
#include <utilities/core/Assert.hpp>

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>

namespace openstudio {

SyncMeasuresDialogCentralWidget::SyncMeasuresDialogCentralWidget(QWidget * parent)
  : QWidget(parent),
  m_collapsibleComponentList(NULL),
  m_componentList(NULL), // TODO cruft to be removed
  m_progressBar(NULL),
  m_pendingDownloads(std::set<std::string>()),
  m_pageIdx(0),
  m_searchString(QString()),
  m_showNewComponents(false)
{
  init();
}

void SyncMeasuresDialogCentralWidget::init()
{
  createLayout();
}

void SyncMeasuresDialogCentralWidget::createLayout()
{
  bool isConnected = false;

  QPushButton * upperPushButton = new QPushButton("Check All");
  isConnected = connect(upperPushButton, SIGNAL(clicked()),
                        this, SLOT(upperPushButtonClicked()));
  OS_ASSERT(isConnected);

  QHBoxLayout * upperLayout = new QHBoxLayout();
  upperLayout->addStretch();
  upperLayout->addWidget(upperPushButton);

  m_collapsibleComponentList = new CollapsibleComponentList();

  isConnected = connect(m_collapsibleComponentList, SIGNAL(headerClicked(bool)),
                        this, SIGNAL(headerClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(headerClicked(bool)),
                        this, SLOT(on_headerClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(componentClicked(bool)),
                        this, SIGNAL(componentClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(componentClicked(bool)),
                        this, SLOT(on_componentClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(collapsibleComponentClicked(bool)),
                        this, SIGNAL(collapsibleComponentClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(collapsibleComponentClicked(bool)),
                        this, SLOT(on_collapsibleComponentClicked(bool)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(getComponentsByPage(int)),
                        this, SIGNAL(getComponentsByPage(int)));
  OS_ASSERT(isConnected);

  isConnected = connect(m_collapsibleComponentList, SIGNAL(getComponentsByPage(int)),
                        this, SLOT(on_getComponentsByPage(int)));
  OS_ASSERT(isConnected);

  //*******************************************************************
  // Hack code to be removed (TODO)
  m_componentList = new ComponentList();  // TODO refactor and remove

  CollapsibleComponentHeader * collapsibleComponentHeader = NULL;
  collapsibleComponentHeader = new CollapsibleComponentHeader("Updates",100,5);

  CollapsibleComponent * collapsibleComponent = NULL;
  collapsibleComponent = new CollapsibleComponent(collapsibleComponentHeader,m_componentList);

  m_collapsibleComponentList->addCollapsibleComponent(collapsibleComponent);
  //*******************************************************************
  
  m_progressBar = new QProgressBar(this);
  m_progressBar->setVisible(false);

  QPushButton * lowerPushButton = new QPushButton("Download");
  isConnected = connect(lowerPushButton, SIGNAL(clicked()),
                        this, SLOT(lowerPushButtonClicked()));
  OS_ASSERT(isConnected);

  QHBoxLayout * lowerLayout = new QHBoxLayout();
  lowerLayout->addStretch();
  lowerLayout->addWidget(m_progressBar);
  lowerLayout->addWidget(lowerPushButton);

  QVBoxLayout * mainLayout = new QVBoxLayout();
  mainLayout->addLayout(upperLayout);

  mainLayout->addWidget(m_collapsibleComponentList,0,Qt::AlignTop);
  mainLayout->addLayout(lowerLayout);
  setLayout(mainLayout);
}

int SyncMeasuresDialogCentralWidget::pageIdx()
{
  return m_pageIdx;
}

void SyncMeasuresDialogCentralWidget::setMeasures(std::vector<BCLMeasure> & measures)
{
  m_collapsibleComponentList->firstPage();

  std::vector<Component *> components = m_componentList->components();

  for( std::vector<Component *>::iterator it = components.begin();
       it != components.end();
       ++it )
  {
    delete *it;
  }

  for( std::vector<BCLMeasure>::iterator it = measures.begin();
       it != measures.end();
       ++it )
  {
    Component * component = new Component(*it);
    
    // TODO replace with a componentList owned by m_collapsibleComponentList
    m_componentList->addComponent(component);
  }

  // the total number of results
  m_collapsibleComponentList->setNumResults(measures.size());

  // the number of pages of results
  int numResultPages = measures.size() / 10;
  if (measures.size() % 2 != 0 ){
    numResultPages++;
  }
  m_collapsibleComponentList->setNumPages(numResultPages);

  // make sure the header is expanded
  if(m_collapsibleComponentList->checkedCollapsibleComponent()){
    m_collapsibleComponentList->checkedCollapsibleComponent()->setExpanded(true);
  }

  // select the first component
  if(m_componentList->firstComponent()){
    m_componentList->firstComponent()->setChecked(true);
  }
  else{
    emit noComponents();
  }

  emit componentsReady();

}

///! Slots

void SyncMeasuresDialogCentralWidget::upperPushButtonClicked()
{
  Q_FOREACH(Component* component, m_collapsibleComponentList->components()){
    if (component->checkBox()->isEnabled()){
      component->checkBox()->setChecked(true);
    }
  }
}

void SyncMeasuresDialogCentralWidget::lowerPushButtonClicked()
{
  Q_FOREACH(Component* component, m_collapsibleComponentList->components()){
    if (component->checkBox()->isChecked() && component->checkBox()->isEnabled()){
      
      RemoteBCL* remoteBCL = new RemoteBCL();

      if (m_filterType == "components")
      {
        bool isConnected = connect(remoteBCL, SIGNAL(componentDownloaded(const std::string&, const boost::optional<BCLComponent>&)),
                                   this, SLOT(componentDownloadComplete(const std::string&, const boost::optional<BCLComponent>&)));
        OS_ASSERT(isConnected);

        bool downloadStarted = remoteBCL->downloadComponent(component->uid());
        if (downloadStarted){

          component->checkBox()->setEnabled(false);
          component->msg()->setHidden(true);
          m_pendingDownloads.insert(component->uid());

          // show busy
          m_progressBar->setValue(1);
          m_progressBar->setMinimum(0);
          m_progressBar->setMaximum(0);
          m_progressBar->setVisible(true);

        }else{

          delete remoteBCL;

          // todo: show error

        }
      }
      else if (m_filterType == "measures")
      {
        bool isConnected = connect(remoteBCL, SIGNAL(measureDownloaded(const std::string&, const boost::optional<BCLMeasure>&)),
                                   this, SLOT(measureDownloadComplete(const std::string&, const boost::optional<BCLMeasure>&)));
        OS_ASSERT(isConnected);

        bool downloadStarted = remoteBCL->downloadMeasure(component->uid());
        if (downloadStarted){

          component->checkBox()->setEnabled(false);
          component->msg()->setHidden(true);
          m_pendingDownloads.insert(component->uid());

          // show busy
          m_progressBar->setValue(1);
          m_progressBar->setMinimum(0);
          m_progressBar->setMaximum(0);
          m_progressBar->setVisible(true);

        }else{

          delete remoteBCL;

          // todo: show error

        }
      }
    }
  }
}

void SyncMeasuresDialogCentralWidget::comboBoxIndexChanged(const QString & text)
{
}

void SyncMeasuresDialogCentralWidget::componentDownloadComplete(const std::string& uid, const boost::optional<BCLComponent>& component)
{
  QObject* sender = this->sender();
  if (sender){
    sender->deleteLater();
  }

  if (component){
    // good
    // remove old component
    boost::optional<BCLComponent> oldComponent = LocalBCL::instance().getComponent(component->uid());
    if (oldComponent && oldComponent->versionId() != component->versionId()){
      LocalBCL::instance().removeComponent(*oldComponent);
    }
  }else{
    // error downloading component
    // find component in list by uid and re-enable
    Q_FOREACH(Component* component, m_collapsibleComponentList->components()){
      if (component->uid() == uid){
        component->checkBox()->setEnabled(true);
        break;
      }
    }
  }

  m_pendingDownloads.erase(uid);
  if (m_pendingDownloads.empty()){
    // show not busy
    m_progressBar->setValue(0);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_progressBar->setVisible(false);
    m_showNewComponents = true;
  }
}

void SyncMeasuresDialogCentralWidget::measureDownloadComplete(const std::string& uid, const boost::optional<BCLMeasure>& measure)
{
  QObject* sender = this->sender();
  if (sender){
    sender->deleteLater();
  }

  if (measure){
    // good
    // remove old measure
    boost::optional<BCLMeasure> oldMeasure = LocalBCL::instance().getMeasure(measure->uid());
    if (oldMeasure && oldMeasure->versionId() != measure->versionId()){
      LocalBCL::instance().removeMeasure(*oldMeasure);
    }
  }else{
    // error downloading measure
    // find measure in list by uid and re-enable
    Q_FOREACH(Component* component, m_collapsibleComponentList->components()){
      if (component->uid() == uid){
        component->checkBox()->setEnabled(true);
        break;
      }
    }
  }

  m_pendingDownloads.erase(uid);
  if (m_pendingDownloads.empty()){
    // show not busy
    m_progressBar->setValue(0);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_progressBar->setVisible(false);
    m_showNewComponents = true;
  }
}

Component * SyncMeasuresDialogCentralWidget::checkedComponent() const
{
  return m_collapsibleComponentList->checkedComponent();
}

bool SyncMeasuresDialogCentralWidget::showNewComponents()
{
  return m_showNewComponents;
}

void SyncMeasuresDialogCentralWidget::setShowNewComponents(bool showNewComponents)
{
  m_showNewComponents = showNewComponents;
}

///! SLOTS

void SyncMeasuresDialogCentralWidget::on_headerClicked(bool checked)
{
}

void SyncMeasuresDialogCentralWidget::on_componentClicked(bool checked)
{
}

void SyncMeasuresDialogCentralWidget::on_collapsibleComponentClicked(bool checked)
{
}

void SyncMeasuresDialogCentralWidget::on_getComponentsByPage(int pageIdx)
{
  m_pageIdx = pageIdx;
}

} // namespace openstudio
