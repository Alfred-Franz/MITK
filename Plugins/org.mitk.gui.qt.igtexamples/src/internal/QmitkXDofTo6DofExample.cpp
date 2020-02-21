/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "QmitkRenderWindow.h"

#include "QmitkXDofTo6DofExample.h"
#include "mitkNavigationDataObjectVisualizationFilter.h"



const std::string QmitkXDofTo6DofExample::VIEW_ID = "org.mitk.views.QmitkXDofTo6DofExample";

void QmitkXDofTo6DofExample::SetFocus()
{
}

QmitkXDofTo6DofExample::~QmitkXDofTo6DofExample()
{
}

void QmitkXDofTo6DofExample::CreateQtPartControl( QWidget *parent )
{

  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi( parent );

  // connect the widget items with the methods
  connect( m_Controls.butStart, SIGNAL(clicked()),
           this, SLOT(Start()) );
  //connect(m_Controls.but)

  //m_TrackingDeviceSource->SetTrackingDevice()
}


void QmitkXDofTo6DofExample::Start()
{

  static bool isFirstTime = true;
  if (isFirstTime)
  {
    //this->CreatePipeline();
    isFirstTime = false;
  }
/*
  m_Timer.setInterval(this->m_Controls.visualizationUpdateRateSpinBox->value());
  m_Timer.start();
  m_Controls.*/

  MITK_INFO << "Test";
  m_Source = m_Controls.NavigationDataSourceSelectionWidget->GetSelectedNavigationDataSource();
  //m_Controls.NavigationDataSourceSelectionWidget->GetSelectedNavigationDataSource()
  int toolID = m_Controls.NavigationDataSourceSelectionWidget->GetSelectedToolID();
  //double source1 = m_Controls.
  m_XDofTo6DofFilter = mitk::NavigationDataXDofTo6DofFilter::New();
  m_XDofTo6DofFilter->ConnectTo(m_Source);
  m_VisFilter = mitk::NavigationDataObjectVisualizationFilter::New();
  m_VisFilter->ConnectTo(m_XDofTo6DofFilter);
  //1. Parameter 0, wenn nur ein Tool
  m_VisFilter->SetRepresentationObject(0, m_Source->GetToolMetaDataCollection()->GetTool(0)->GetDataNode()->GetData());
}

//void QmitkXDofTo6DofExample::CreatePipeline()
//{
//  // create a visualization filter
//  m_VisFilter = mitk::NavigationDataObjectVisualizationFilter::New();
//
//}
