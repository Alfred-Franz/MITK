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

#include "QmitkUSNavigationStepCtUsRegistration.h"
#include "ui_QmitkUSNavigationStepCtUsRegistration.h"

#include <QMessageBox>

#include "mitkNodeDisplacementFilter.h"
#include "../QmitkUSNavigationMarkerPlacement.h"



#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkLabelSetImage.h>
#include "mitkProperties.h"
#include <mitkImage.h>
#include <mitkImageCast.h>

#include <mitkRenderingManager.h>
#include <mitkStaticIGTHelperFunctions.h>

#include <vtkLandmarkTransform.h>
#include <vtkPoints.h>

#include <itkImageRegionIterator.h>
#include <itkGeometryUtilities.h>
#include <mitkImagePixelReadAccessor.h>

#include <itkZeroCrossingImageFilter.h>
#include <itkSimpleContourExtractorImageFilter.h>
#include <itkCannyEdgeDetectionImageFilter.h>

static const int NUMBER_FIDUCIALS_NEEDED = 8; 

QmitkUSNavigationStepCtUsRegistration::QmitkUSNavigationStepCtUsRegistration(QWidget *parent) :
  QmitkUSAbstractNavigationStep(parent),
  ui(new Ui::QmitkUSNavigationStepCtUsRegistration),
  m_PerformingGroundTruthProtocolEvaluation(false)
{
  this->UnsetFloatingImageGeometry();
  this->DefineDataStorageImageFilter();
  this->CreateQtPartControl(this);
}


QmitkUSNavigationStepCtUsRegistration::~QmitkUSNavigationStepCtUsRegistration()
{
  delete ui;
}

bool QmitkUSNavigationStepCtUsRegistration::OnStartStep()
{
  MITK_INFO << "OnStartStep()";
  return true;
}

bool QmitkUSNavigationStepCtUsRegistration::OnStopStep()
{
  MITK_INFO << "OnStopStep()";
  return true;
}

bool QmitkUSNavigationStepCtUsRegistration::OnFinishStep()
{
  MITK_INFO << "OnFinishStep()";
  return true;
}

bool QmitkUSNavigationStepCtUsRegistration::OnActivateStep()
{
  MITK_INFO << "OnActivateStep()";
  ui->floatingImageComboBox->SetDataStorage(this->GetDataStorage());
  ui->groundTruthImageComboBox->SetDataStorage(this->GetDataStorage());
  ui->ctImagesToChooseComboBox->SetDataStorage(this->GetDataStorage());
  return true;
}

bool QmitkUSNavigationStepCtUsRegistration::OnDeactivateStep()
{
  MITK_INFO << "OnDeactivateStep()";
  return true;
}

void QmitkUSNavigationStepCtUsRegistration::OnUpdate()
{
  if (m_NavigationDataSource.IsNull()) { return; }

  m_NavigationDataSource->Update();
}

void QmitkUSNavigationStepCtUsRegistration::OnSettingsChanged(const itk::SmartPointer<mitk::DataNode> settingsNode)
{
 
}

QString QmitkUSNavigationStepCtUsRegistration::GetTitle()
{
  return "CT-to-US registration";
}

QmitkUSAbstractNavigationStep::FilterVector QmitkUSNavigationStepCtUsRegistration::GetFilter()
{
  return FilterVector();
}

void QmitkUSNavigationStepCtUsRegistration::OnSetCombinedModality()
{
  mitk::AbstractUltrasoundTrackerDevice::Pointer combinedModality = this->GetCombinedModality(false);
  if (combinedModality.IsNotNull())
  {
    m_NavigationDataSource = combinedModality->GetNavigationDataSource();
  }

}

void QmitkUSNavigationStepCtUsRegistration::UnsetFloatingImageGeometry()
{
  m_ImageDimension[0] = 0;
  m_ImageDimension[1] = 0;
  m_ImageDimension[2] = 0;

  m_ImageSpacing[0] = 1;
  m_ImageSpacing[1] = 1;
  m_ImageSpacing[2] = 1;
}

void QmitkUSNavigationStepCtUsRegistration::SetFloatingImageGeometryInformation(mitk::Image * image)
{
  m_ImageDimension[0] = image->GetDimension(0);
  m_ImageDimension[1] = image->GetDimension(1);
  m_ImageDimension[2] = image->GetDimension(2);

  m_ImageSpacing[0] = image->GetGeometry()->GetSpacing()[0];
  m_ImageSpacing[1] = image->GetGeometry()->GetSpacing()[1];
  m_ImageSpacing[2] = image->GetGeometry()->GetSpacing()[2];
}

double QmitkUSNavigationStepCtUsRegistration::GetVoxelVolume()
{
  if (m_FloatingImage.IsNull())
  {
    return 0.0;
  }

  MITK_INFO << "ImageSpacing = " << m_ImageSpacing;
  return m_ImageSpacing[0] * m_ImageSpacing[1] * m_ImageSpacing[2];
}

double QmitkUSNavigationStepCtUsRegistration::GetFiducialVolume(double radius)
{
  return 1.333333333 * 3.141592 * (radius * radius * radius);
}

bool QmitkUSNavigationStepCtUsRegistration::FilterFloatingImage()
{
  if (m_FloatingImage.IsNull())
  {
    return false;
  }

  ImageType::Pointer itkImage1 = ImageType::New();
  mitk::CastToItkImage(m_FloatingImage, itkImage1);

  this->InitializeImageFilters();

  m_ThresholdFilter->SetInput(itkImage1);
  m_LaplacianFilter1->SetInput(m_ThresholdFilter->GetOutput());
  m_LaplacianFilter2->SetInput(m_LaplacianFilter1->GetOutput());
  m_BinaryThresholdFilter->SetInput(m_LaplacianFilter2->GetOutput());
  m_HoleFillingFilter->SetInput(m_BinaryThresholdFilter->GetOutput());
  m_BinaryImageToShapeLabelMapFilter->SetInput(m_HoleFillingFilter->GetOutput());
  m_BinaryImageToShapeLabelMapFilter->Update();

  ImageType::Pointer binaryImage = ImageType::New();
  binaryImage = m_HoleFillingFilter->GetOutput();

  this->EliminateTooSmallLabeledObjects(binaryImage);
  mitk::CastToMitkImage(binaryImage, m_FloatingImage);
  return true;
}

void QmitkUSNavigationStepCtUsRegistration::InitializeImageFilters()
{
  //Initialize threshold filters
  m_ThresholdFilter = itk::ThresholdImageFilter<ImageType>::New();
  m_ThresholdFilter->SetOutsideValue(0);
  m_ThresholdFilter->SetLower(500);
  m_ThresholdFilter->SetUpper(3200);

  //Initialize binary threshold filter 1
  m_BinaryThresholdFilter = BinaryThresholdImageFilterType::New();
  m_BinaryThresholdFilter->SetOutsideValue(0);
  m_BinaryThresholdFilter->SetInsideValue(1);
  m_BinaryThresholdFilter->SetLowerThreshold(350);
  m_BinaryThresholdFilter->SetUpperThreshold(10000);

  //Initialize laplacian recursive gaussian image filter
  m_LaplacianFilter1 = LaplacianRecursiveGaussianImageFilterType::New();
  m_LaplacianFilter2 = LaplacianRecursiveGaussianImageFilterType::New();

  //Initialize binary hole filling filter 
  m_HoleFillingFilter = VotingBinaryIterativeHoleFillingImageFilterType::New();
  VotingBinaryIterativeHoleFillingImageFilterType::InputSizeType radius;
  radius.Fill(1);
  m_HoleFillingFilter->SetRadius(radius);
  m_HoleFillingFilter->SetBackgroundValue(0);
  m_HoleFillingFilter->SetForegroundValue(1);
  m_HoleFillingFilter->SetMaximumNumberOfIterations(5);

  //Initialize binary image to shape label map filter
  m_BinaryImageToShapeLabelMapFilter = BinaryImageToShapeLabelMapFilterType::New();
  m_BinaryImageToShapeLabelMapFilter->SetInputForegroundValue(1);
}

double QmitkUSNavigationStepCtUsRegistration::GetCharacteristicDistanceAWithUpperMargin()
{
  switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
  {
    // case 0 is equal to fiducial marker configuration A (10mm distance)
    case 0:
      return 12.07;

    // case 1 is equal to fiducial marker configuration B (15mm distance)
    case 1:
      return 18.105;

    // case 2 is equal to fiducial marker configuration C (20mm distance)
    case 2:
      return 24.14;
  }
  return 0.0;
}

double QmitkUSNavigationStepCtUsRegistration::GetCharacteristicDistanceBWithLowerMargin()
{
  switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
  {
    // case 0 is equal to fiducial marker configuration A (10mm distance)
  case 0:
    return 12.07;

    // case 1 is equal to fiducial marker configuration B (15mm distance)
  case 1:
    return 18.105;

    // case 2 is equal to fiducial marker configuration C (20mm distance)
  case 2:
    return 24.14;
  }
  return 0.0;
}

double QmitkUSNavigationStepCtUsRegistration::GetCharacteristicDistanceBWithUpperMargin()
{
  switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
  {
    // case 0 is equal to fiducial marker configuration A (10mm distance)
  case 0:
    return 15.73;

    // case 1 is equal to fiducial marker configuration B (15mm distance)
  case 1:
    return 23.595;

    // case 2 is equal to fiducial marker configuration C (20mm distance)
  case 2:
    return 31.46;
  }
  return 0.0;
}

double QmitkUSNavigationStepCtUsRegistration::GetMinimalFiducialConfigurationDistance()
{
  switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
  {
    // case 0 is equal to fiducial marker configuration A (10mm distance)
  case 0:
    return 10.0;

    // case 1 is equal to fiducial marker configuration B (15mm distance)
  case 1:
    return 15.0;

    // case 2 is equal to fiducial marker configuration C (20mm distance)
  case 2:
    return 20.0;
  }
  return 0.0;
}

void QmitkUSNavigationStepCtUsRegistration::CreateMarkerModelCoordinateSystemPointSet()
{
  if (m_MarkerModelCoordinateSystemPointSet.IsNull())
  {
    m_MarkerModelCoordinateSystemPointSet = mitk::PointSet::New();
  }
  else
  {
    m_MarkerModelCoordinateSystemPointSet->Clear();
  }

  mitk::Point3D fiducial1;
  mitk::Point3D fiducial2;
  mitk::Point3D fiducial3;
  mitk::Point3D fiducial4;
  mitk::Point3D fiducial5;
  mitk::Point3D fiducial6;
  mitk::Point3D fiducial7;
  mitk::Point3D fiducial8;



  switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
  {
    // case 0 is equal to fiducial marker configuration A (10mm distance)
  case 0:
    fiducial1[0] = 0;
    fiducial1[1] = 0;
    fiducial1[2] = 0;

    fiducial2[0] = 0;
    fiducial2[1] = 10;
    fiducial2[2] = 0;

    fiducial3[0] = 10;
    fiducial3[1] = 0;
    fiducial3[2] = 0;

    fiducial4[0] = 20;
    fiducial4[1] = 20;
    fiducial4[2] = 0;

    fiducial5[0] = 0;
    fiducial5[1] = 20;
    fiducial5[2] = 10;

    fiducial6[0] = 10;
    fiducial6[1] = 20;
    fiducial6[2] = 10;

    fiducial7[0] = 20;
    fiducial7[1] = 10;
    fiducial7[2] = 10;

    fiducial8[0] = 20;
    fiducial8[1] = 0;
    fiducial8[2] = 10;
    break;
    // case 1 is equal to fiducial marker configuration B (15mm distance)
  case 1:
    fiducial1[0] = 0;
    fiducial1[1] = 0;
    fiducial1[2] = 0;

    fiducial2[0] = 0;
    fiducial2[1] = 15;
    fiducial2[2] = 0;

    fiducial3[0] = 15;
    fiducial3[1] = 0;
    fiducial3[2] = 0;

    fiducial4[0] = 30;
    fiducial4[1] = 30;
    fiducial4[2] = 0;

    fiducial5[0] = 0;
    fiducial5[1] = 30;
    fiducial5[2] = 15;

    fiducial6[0] = 15;
    fiducial6[1] = 30;
    fiducial6[2] = 15;

    fiducial7[0] = 30;
    fiducial7[1] = 15;
    fiducial7[2] = 15;

    fiducial8[0] = 30;
    fiducial8[1] = 0;
    fiducial8[2] = 15;
    break;
    // case 2 is equal to fiducial marker configuration C (20mm distance)
  case 2:
    fiducial1[0] = 0;
    fiducial1[1] = 0;
    fiducial1[2] = 0;

    fiducial2[0] = 0;
    fiducial2[1] = 20;
    fiducial2[2] = 0;

    fiducial3[0] = 20;
    fiducial3[1] = 0;
    fiducial3[2] = 0;

    fiducial4[0] = 40;
    fiducial4[1] = 40;
    fiducial4[2] = 0;

    fiducial5[0] = 0;
    fiducial5[1] = 40;
    fiducial5[2] = 20;

    fiducial6[0] = 20;
    fiducial6[1] = 40;
    fiducial6[2] = 20;

    fiducial7[0] = 40;
    fiducial7[1] = 20;
    fiducial7[2] = 20;

    fiducial8[0] = 40;
    fiducial8[1] = 0;
    fiducial8[2] = 20;
    break;
  }

  m_MarkerModelCoordinateSystemPointSet->InsertPoint(0, fiducial1);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(1, fiducial2);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(2, fiducial3);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(3, fiducial4);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(4, fiducial5);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(5, fiducial6);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(6, fiducial7);
  m_MarkerModelCoordinateSystemPointSet->InsertPoint(7, fiducial8);

  mitk::DataNode::Pointer node = this->GetDataStorage()->GetNamedNode("Marker Model Coordinate System Point Set");
  if (node == nullptr)
  {
    node = mitk::DataNode::New();
    node->SetName("Marker Model Coordinate System Point Set");
    node->SetData(m_MarkerModelCoordinateSystemPointSet);
    this->GetDataStorage()->Add(node);
  }
  else
  {
    node->SetData(m_MarkerModelCoordinateSystemPointSet);
    this->GetDataStorage()->Modified();
  }
}

void QmitkUSNavigationStepCtUsRegistration::EliminateTooSmallLabeledObjects(
  ImageType::Pointer binaryImage)
{
  BinaryImageToShapeLabelMapFilterType::OutputImageType::Pointer labelMap =
    m_BinaryImageToShapeLabelMapFilter->GetOutput();
  double voxelVolume = this->GetVoxelVolume();
  double fiducialVolume;
  unsigned int numberOfPixels;

  if (ui->fiducialDiameter3mmRadioButton->isChecked())
  {
    fiducialVolume = this->GetFiducialVolume(1.5);
    numberOfPixels = ceil(fiducialVolume / voxelVolume);
  }
  else
  {
    fiducialVolume = this->GetFiducialVolume(2.5);
    numberOfPixels = ceil(fiducialVolume / voxelVolume);
  }

  MITK_INFO << "Voxel Volume = " << voxelVolume << "; Fiducial Volume = " << fiducialVolume;
  MITK_INFO << "Number of pixels = " << numberOfPixels;

  labelMap = m_BinaryImageToShapeLabelMapFilter->GetOutput();
  // The output of this filter is an itk::LabelMap, which contains itk::LabelObject's
  MITK_INFO << "There are " << labelMap->GetNumberOfLabelObjects() << " objects.";

  // Loop over each region
  for (int i = labelMap->GetNumberOfLabelObjects() - 1; i >= 0; --i)
  {
    // Get the ith region
    BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = labelMap->GetNthLabelObject(i);
    MITK_INFO << "Object " << i << " contains " << labelObject->Size() << " pixel";

    //TODO: Threshold-Wert evtl. experimentell besser abstimmen,
    //      um zu verhindern, dass durch Threshold wahre Fiducial-Kandidaten elimiert werden.
    if (labelObject->Size() < numberOfPixels * 0.8)
    {
      for (unsigned int pixelId = 0; pixelId < labelObject->Size(); pixelId++)
      {
        binaryImage->SetPixel(labelObject->GetIndex(pixelId), 0);
      }
      labelMap->RemoveLabelObject(labelObject);
    }
  }
}

bool QmitkUSNavigationStepCtUsRegistration::EliminateFiducialCandidatesByEuclideanDistances()
{
  if (m_CentroidsOfFiducialCandidates.size() < NUMBER_FIDUCIALS_NEEDED)
  {
    return false;
  }

  for (int counter = 0; counter < m_CentroidsOfFiducialCandidates.size(); ++counter)
  {
    int amountOfAcceptedFiducials = 0;
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(counter));
    //Loop through all fiducial candidates and calculate the distance between the chosen fiducial
    // candidate and the other candidates. For each candidate with a right distance between
    // Configuration A: 7.93mm and 31.0mm (10 mm distance between fiducial centers) or
    // Configuration B: 11.895mm and 45.0mm (15 mm distance between fiducial centers) or
    // Configuration C: 15.86mm and 59.0mm (20 mm distance between fiducial centers)
    //
    // increase the amountOfAcceptedFiducials.
    for (int position = 0; position < m_CentroidsOfFiducialCandidates.size(); ++position)
    {
      if (position == counter)
      {
        continue;
      }
      mitk::Point3D otherCentroid(m_CentroidsOfFiducialCandidates.at(position));
      double distance = fiducialCentroid.EuclideanDistanceTo(otherCentroid);

      switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
      {
        // case 0 is equal to fiducial marker configuration A (10mm distance)
        case 0:
          if (distance > 7.93 && distance < 31.0)
          {
            ++amountOfAcceptedFiducials;
          }
          break;
        // case 1 is equal to fiducial marker configuration B (15mm distance)
        case 1:
          if (distance > 11.895 && distance < 45.0)
          {
            ++amountOfAcceptedFiducials;
          }
          break;
        // case 2 is equal to fiducial marker configuration C (20mm distance)
        case 2:
          if (distance > 15.86 && distance < 59.0)
          {
            ++amountOfAcceptedFiducials;
          }
          break;
      }
    }
    //The amountOfAcceptedFiducials must be at least 7. Otherwise delete the fiducial candidate
    // from the list of candidates.
    if (amountOfAcceptedFiducials < NUMBER_FIDUCIALS_NEEDED - 1)
    {
      MITK_INFO << "Deleting fiducial candidate at position: " <<
        m_CentroidsOfFiducialCandidates.at(counter);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + counter);
      if (m_CentroidsOfFiducialCandidates.size() < NUMBER_FIDUCIALS_NEEDED )
      {
        return false;
      }
      counter = -1;
    }
  }

  //Classify the rested fiducial candidates by its characteristic Euclidean distances
  // between the canidates and remove all candidates with a false distance configuration:
  this->ClassifyFiducialCandidates();
  return true;
}

void QmitkUSNavigationStepCtUsRegistration::ClassifyFiducialCandidates()
{
  MITK_INFO << "ClassifyFiducialCandidates()";
  std::vector<int> fiducialCandidatesToBeRemoved;
  std::vector<std::vector<double>> distanceVectorsFiducials;
  this->CalculateDistancesBetweenFiducials(distanceVectorsFiducials);

  for (int counter = 0; counter < distanceVectorsFiducials.size(); ++counter)
  {
    int distanceA = 0;      // => 10,00mm distance
    int distanceB = 0;      // => 14,14mm distance
    int distanceC = 0;      // => 17,32mm distance
    int distanceD = 0;      // => 22,36mm distance
    int distanceE = 0;      // => 24,49mm distance
    int distanceF = 0;      // => 28,28mm distance

    std::vector<double> &distances = distanceVectorsFiducials.at(counter);
    for (int number = 0; number < distances.size(); ++number)
    {
      double &distance = distances.at(number);
      switch (ui->fiducialMarkerConfigurationComboBox->currentIndex())
      {
        // case 0 is equal to fiducial marker configuration A (10mm distance)
        case 0:
          if (distance > 7.93 && distance <= 12.07)
          {
            ++distanceA;
          }
          else if (distance > 12.07 && distance <= 15.73)
          {
            ++distanceB;
          }
          else if (distance > 15.73 && distance <= 19.84)
          {
            ++distanceC;
          }
          else if (distance > 19.84 && distance <= 23.425)
          {
            ++distanceD;
          }
          else if (distance > 23.425 && distance <= 26.385)
          {
            ++distanceE;
          }
          else if (distance > 26.385 && distance <= 31.00)
          {
            ++distanceF;
          }
        break;

        // case 1 is equal to fiducial marker configuration B (15mm distance)
        case 1:
          if (distance > 11.895 && distance <= 18.105)
          {
            ++distanceA;
          }
          else if (distance > 18.105 && distance <= 23.595)
          {
            ++distanceB;
          }
          else if (distance > 23.595 && distance <= 29.76)
          {
            ++distanceC;
          }
          else if (distance > 29.76 && distance <= 35.1375)
          {
            ++distanceD;
          }
          else if (distance > 35.1375 && distance <= 39.5775)
          {
            ++distanceE;
          }
          else if (distance > 39.5775 && distance <= 45.00)
          {
            ++distanceF;
          }
        break;

        // case 2 is equal to fiducial marker configuration C (20mm distance)
        case 2:
          if (distance > 15.86 && distance <= 24.14)
          {
            ++distanceA;
          }
          else if (distance > 24.14 && distance <= 31.46)
          {
            ++distanceB;
          }
          else if (distance > 31.46 && distance <= 39.68)
          {
            ++distanceC;
          }
          else if (distance > 39.68 && distance <= 46.85)
          {
            ++distanceD;
          }
          else if (distance > 46.85 && distance <= 52.77)
          {
            ++distanceE;
          }
          else if (distance > 52.77 && distance <= 59.00)
          {
            ++distanceF;
          }
        break;
      }
    }// End for-loop distances-vector

    //Now, having looped through all distances of one fiducial candidate, check
    // if the combination of different distances is known. The >= is due to the
    // possible occurrence of other fiducial candidates that have an distance equal to
    // one of the distances A - E. However, false fiducial candidates outside
    // the fiducial marker does not have the right distance configuration:
    if ((distanceA >= 2 && distanceD >= 2 && distanceE >= 2 && distanceF >= 1 ||
      distanceA >= 1 && distanceB >= 2 && distanceC >= 1 && distanceD >= 2 && distanceE >= 1 ||
      distanceB >= 2 && distanceD >= 4 && distanceF >= 1 ||
      distanceA >= 1 && distanceB >= 1 && distanceD >= 3 && distanceE >= 1 && distanceF >= 1) == false)
    {
      MITK_INFO << "Detected fiducial candidate with unknown distance configuration.";
      fiducialCandidatesToBeRemoved.push_back(counter);
    }
  }
  for (int count = fiducialCandidatesToBeRemoved.size() - 1; count >= 0; --count)
  {
    MITK_INFO << "Removing fiducial candidate " << fiducialCandidatesToBeRemoved.at(count);
    m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin()
                                          + fiducialCandidatesToBeRemoved.at(count));
  }
}

void QmitkUSNavigationStepCtUsRegistration::GetCentroidsOfLabeledObjects()
{
  MITK_INFO << "GetCentroidsOfLabeledObjects()";
  BinaryImageToShapeLabelMapFilterType::OutputImageType::Pointer labelMap =
    m_BinaryImageToShapeLabelMapFilter->GetOutput();
  for (int i = labelMap->GetNumberOfLabelObjects() - 1; i >= 0; --i)
  {
    // Get the ith region
    BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = labelMap->GetNthLabelObject(i);
    MITK_INFO << "Object " << i << " contains " << labelObject->Size() << " pixel";

    mitk::Vector3D centroid;
    centroid[0] = labelObject->GetCentroid()[0];
    centroid[1] = labelObject->GetCentroid()[1];
    centroid[2] = labelObject->GetCentroid()[2];
    m_CentroidsOfFiducialCandidates.push_back(centroid);
  }
  //evtl. for later: itk::LabelMapOverlayImageFilter
}

void QmitkUSNavigationStepCtUsRegistration::CalculatePCA()
{
  //Step 1: Construct data matrix
  int columnSize = m_CentroidsOfFiducialCandidates.size();
  if (columnSize == 0)
  {
    MITK_INFO << "Cannot calculate PCA. There are no fiducial candidates.";
    return;
  }

  vnl_matrix<double> pointSetMatrix(3, columnSize, 0.0);
  for (int counter = 0; counter < columnSize; ++counter)
  {
    pointSetMatrix[0][counter] = m_CentroidsOfFiducialCandidates.at(counter)[0];
    pointSetMatrix[1][counter] = m_CentroidsOfFiducialCandidates.at(counter)[1];
    pointSetMatrix[2][counter] = m_CentroidsOfFiducialCandidates.at(counter)[2];
  }

  //Step 2: Remove average for each row (Mittelwertbefreiung)
  for (int counter = 0; counter < columnSize; ++counter)
  {
    m_MeanCentroidFiducialCandidates += mitk::Vector3D(pointSetMatrix.get_column(counter));
  }
  //TODO: f�r sp�ter �berpr�fen, ob Division durch integer nicht zu Rechenproblemen f�hrt.
  m_MeanCentroidFiducialCandidates /= columnSize;
  for (int counter = 0; counter < columnSize; ++counter)
  {
    pointSetMatrix.get_column(counter) -= m_MeanCentroidFiducialCandidates;
  }

  //Step 3: Compute covariance matrix
  vnl_matrix<double> covarianceMatrix = (1.0 / (columnSize - 1.0)) * pointSetMatrix * pointSetMatrix.transpose();

  //Step 4: Singular value composition
  vnl_svd<double> svd(covarianceMatrix);

  //Storing results:
  for (int counter = 0; counter < 3; ++counter)
  {
    mitk::Vector3D eigenVector = svd.U().get_column(counter);
    double eigenValue = sqrt(svd.W(counter));
    m_EigenVectorsFiducialCandidates[eigenValue] = eigenVector;
    m_EigenValuesFiducialCandidates.push_back(eigenValue);
  }
  std::sort( m_EigenValuesFiducialCandidates.begin(), m_EigenValuesFiducialCandidates.end() );

  mitk::DataNode::Pointer axis1Node = mitk::DataNode::New();
  axis1Node->SetName("Eigenvector 1");
  mitk::PointSet::Pointer axis1 = mitk::PointSet::New();
  axis1->InsertPoint(0, m_MeanCentroidFiducialCandidates);
  axis1->InsertPoint(1, (m_MeanCentroidFiducialCandidates + m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(2))*m_EigenValuesFiducialCandidates.at(2)));
  axis1Node->SetData(axis1);
  axis1Node->SetBoolProperty("show contour", true);
  axis1Node->SetColor(1, 0, 0);
  this->GetDataStorage()->Add(axis1Node);

  mitk::DataNode::Pointer axis2Node = mitk::DataNode::New();
  axis2Node->SetName("Eigenvector 2");
  mitk::PointSet::Pointer axis2 = mitk::PointSet::New();
  axis2->InsertPoint(0, m_MeanCentroidFiducialCandidates);
  axis2->InsertPoint(1, (m_MeanCentroidFiducialCandidates + m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(1))*m_EigenValuesFiducialCandidates.at(1)));
  axis2Node->SetData(axis2);
  axis2Node->SetBoolProperty("show contour", true);
  axis2Node->SetColor(2, 0, 0);
  this->GetDataStorage()->Add(axis2Node);

  mitk::DataNode::Pointer axis3Node = mitk::DataNode::New();
  axis3Node->SetName("Eigenvector 3");
  mitk::PointSet::Pointer axis3 = mitk::PointSet::New();
  axis3->InsertPoint(0, m_MeanCentroidFiducialCandidates);
  axis3->InsertPoint(1, (m_MeanCentroidFiducialCandidates + m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(0))*m_EigenValuesFiducialCandidates.at(0)));
  axis3Node->SetData(axis3);
  axis3Node->SetBoolProperty("show contour", true);
  axis3Node->SetColor(3, 0, 0);
  this->GetDataStorage()->Add(axis3Node);

  MITK_INFO << "Mean: " << m_MeanCentroidFiducialCandidates;

  MITK_INFO << "Eigenvektor 1: " << m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(2));
  MITK_INFO << "Eigenvektor 2: " << m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(1));
  MITK_INFO << "Eigenvektor 3: " << m_EigenVectorsFiducialCandidates.at(m_EigenValuesFiducialCandidates.at(0));

  MITK_INFO << "Eigenwert 1: " << m_EigenValuesFiducialCandidates.at(2);
  MITK_INFO << "Eigenwert 2: " << m_EigenValuesFiducialCandidates.at(1);
  MITK_INFO << "Eigenwert 3: " << m_EigenValuesFiducialCandidates.at(0);

}

void QmitkUSNavigationStepCtUsRegistration::NumerateFiducialMarks()
{
  MITK_INFO << "NumerateFiducialMarks()";
  bool successFiducialNo1;
  bool successFiducialNo4;
  bool successFiducialNo2And3;
  bool successFiducialNo5;
  bool successFiducialNo8;
  bool successFiducialNo6;
  bool successFiducialNo7;

  std::vector<std::vector<double>> distanceVectorsFiducials;
  this->CalculateDistancesBetweenFiducials(distanceVectorsFiducials);
  successFiducialNo1 = this->FindFiducialNo1(distanceVectorsFiducials);
  successFiducialNo4 = this->FindFiducialNo4(distanceVectorsFiducials);
  successFiducialNo2And3 = this->FindFiducialNo2And3();
  successFiducialNo5 = this->FindFiducialNo5();
  successFiducialNo8 = this->FindFiducialNo8();
  successFiducialNo6 = this->FindFiducialNo6();
  successFiducialNo7 = this->FindFiducialNo7();

  if (!successFiducialNo1 || !successFiducialNo4 || !successFiducialNo2And3 ||
    !successFiducialNo5 || !successFiducialNo8 || !successFiducialNo6 || !successFiducialNo7)
  {
    QMessageBox msgBox;
    msgBox.setText("Cannot numerate/localize all fiducials successfully.");
    msgBox.exec();
    return;
  }

  if( ui->optimizeFiducialPositionsCheckBox->isChecked())
  {
    this->OptimizeFiducialPositions();
  }

  if (m_MarkerFloatingImageCoordinateSystemPointSet.IsNull())
  {
    m_MarkerFloatingImageCoordinateSystemPointSet = mitk::PointSet::New();
  }
  else if (m_MarkerFloatingImageCoordinateSystemPointSet->GetSize() != 0)
  {
    m_MarkerFloatingImageCoordinateSystemPointSet->Clear();
  }

  for (int counter = 1; counter <= m_FiducialMarkerCentroids.size(); ++counter)
  {
    m_MarkerFloatingImageCoordinateSystemPointSet->InsertPoint(counter - 1, m_FiducialMarkerCentroids.at(counter));
  }
  if( !m_PerformingGroundTruthProtocolEvaluation )
  {
    mitk::DataNode::Pointer node = mitk::DataNode::New();
    node->SetData(m_MarkerFloatingImageCoordinateSystemPointSet);
    node->SetName("MarkerFloatingImageCSPointSet");
    //node->SetFloatProperty("pointsize", 5.0);
    this->GetDataStorage()->Add(node);
  }
}

void QmitkUSNavigationStepCtUsRegistration::ShowGroundTruthMarkerEdges()
{
  mitk::Point3D edge1;
  mitk::Point3D edge2;
  mitk::Point3D edge3;
  mitk::Point3D edge4;
  mitk::Point3D edge5;
  mitk::Point3D edge6;
  mitk::Point3D edge7;
  mitk::Point3D edge8;

  edge1[0] = -5;
  edge1[1] = -5;
  edge1[2] = -7.5;

  edge2[0] = -5;
  edge2[1] = 25;
  edge2[2] = -7.5;

  edge3[0] = 25;
  edge3[1] = 25;
  edge3[2] = -7.5;

  edge4[0] = 25;
  edge4[1] = -5;
  edge4[2] = -7.5;

  edge5[0] = -5;
  edge5[1] = -5;
  edge5[2] = 15;

  edge6[0] = -5;
  edge6[1] = 25;
  edge6[2] = 15;

  edge7[0] = 25;
  edge7[1] = 25;
  edge7[2] = 15;

  edge8[0] = 25;
  edge8[1] = -5;
  edge8[2] = 15;

  mitk::PointSet::Pointer edgePointSet = mitk::PointSet::New();
  edgePointSet->InsertPoint(0, edge1);
  edgePointSet->InsertPoint(1, edge2);
  edgePointSet->InsertPoint(2, edge3);
  edgePointSet->InsertPoint(3, edge4);
  edgePointSet->InsertPoint(4, edge1);
  edgePointSet->InsertPoint(5, edge5);
  edgePointSet->InsertPoint(6, edge6);
  edgePointSet->InsertPoint(7, edge7);
  edgePointSet->InsertPoint(8, edge8);
  edgePointSet->InsertPoint(9, edge5);
  edgePointSet->InsertPoint(10, edge6);
  edgePointSet->InsertPoint(11, edge2);
  edgePointSet->InsertPoint(12, edge6);
  edgePointSet->InsertPoint(13, edge7);
  edgePointSet->InsertPoint(14, edge3);
  edgePointSet->InsertPoint(15, edge7);
  edgePointSet->InsertPoint(16, edge8);
  edgePointSet->InsertPoint(17, edge4);

  mitk::DataNode::Pointer node = mitk::DataNode::New();
  node->SetData(edgePointSet);
  node->SetName("Marker Edges PointSet");
  node->SetBoolProperty("show contour", true);
  node->SetFloatProperty("pointsize", 0.25);
  this->GetDataStorage()->Add(node);

  mitk::PointSet* pointSet_orig = edgePointSet;
  mitk::PointSet::Pointer pointSet_moved = mitk::PointSet::New();

  for (int i = 0; i < pointSet_orig->GetSize(); i++)
  {
    pointSet_moved->InsertPoint(m_TransformMarkerCSToFloatingImageCS->TransformPoint(pointSet_orig->GetPoint(i)));
  }

  pointSet_orig->Clear();
  for (int i = 0; i < pointSet_moved->GetSize(); i++)
    pointSet_orig->InsertPoint(pointSet_moved->GetPoint(i));

  //Do a global reinit
  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
}

void QmitkUSNavigationStepCtUsRegistration::CalculateDistancesBetweenFiducials(std::vector<std::vector<double>>& distanceVectorsFiducials)
{
  std::vector<double> distancesBetweenFiducials;

  for (int i = 0; i < m_CentroidsOfFiducialCandidates.size(); ++i)
  {
    distancesBetweenFiducials.clear();
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(i));
    for (int n = 0; n < m_CentroidsOfFiducialCandidates.size(); ++n)
    {
      mitk::Point3D otherCentroid(m_CentroidsOfFiducialCandidates.at(n));
      distancesBetweenFiducials.push_back(fiducialCentroid.EuclideanDistanceTo(otherCentroid));
    }
    //Sort the distances from low to big numbers
    std::sort(distancesBetweenFiducials.begin(), distancesBetweenFiducials.end());
    //First entry of the distance vector must be 0, so erase it
    if (distancesBetweenFiducials.at(0) == 0.0)
    {
      distancesBetweenFiducials.erase(distancesBetweenFiducials.begin());
    }
    //Add the distance vector to the collecting distances vector
    distanceVectorsFiducials.push_back(distancesBetweenFiducials);
  }

  for (int i = 0; i < distanceVectorsFiducials.size(); ++i)
  {
    MITK_INFO << "Vector " << i << ":";
    for (int k = 0; k < distanceVectorsFiducials.at(i).size(); ++k)
    {
      MITK_INFO << distanceVectorsFiducials.at(i).at(k);
    }
  }
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo1(std::vector<std::vector<double>>& distanceVectorsFiducials)
{
  for (int i = 0; i < distanceVectorsFiducials.size(); ++i)
  {
    std::vector<double> &distances = distanceVectorsFiducials.at(i);
    if (distances.size() < NUMBER_FIDUCIALS_NEEDED - 1 )
    {
      MITK_WARN << "Cannot find fiducial 1, there aren't found enough fiducial candidates.";
      return false;
    }
    double characteristicDistanceAWithUpperMargin = this->GetCharacteristicDistanceAWithUpperMargin();

    if (distances.at(0) <= characteristicDistanceAWithUpperMargin &&
        distances.at(1) <= characteristicDistanceAWithUpperMargin)
    {
      MITK_INFO << "Found Fiducial 1 (PointSet number " << i << ")";
      m_FiducialMarkerCentroids.insert( std::pair<int,mitk::Vector3D>(1, m_CentroidsOfFiducialCandidates.at(i)));
      distanceVectorsFiducials.erase(distanceVectorsFiducials.begin() + i);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + i);
      return true;
    }
  }
  return false;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo2And3()
{
  if (m_FiducialMarkerCentroids.find(1) == m_FiducialMarkerCentroids.end() )
  {
    MITK_WARN << "Cannot find fiducial No 2 and 3. Before must be found fiducial No 1.";
    return false;
  }

  mitk::Point3D fiducialNo1(m_FiducialMarkerCentroids.at(1));
  mitk::Vector3D fiducialVectorA;
  mitk::Vector3D fiducialVectorB;
  mitk::Point3D fiducialPointA;
  mitk::Point3D fiducialPointB;
  bool foundFiducialA = false;
  bool foundFiducialB = false;
  mitk::Vector3D vectorFiducial1ToFiducialA;
  mitk::Vector3D vectorFiducial1ToFiducialB;

  for (int i = 0; i < m_CentroidsOfFiducialCandidates.size(); ++i)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(i));
    double distance = fiducialNo1.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= this->GetCharacteristicDistanceAWithUpperMargin())
    {
      fiducialVectorA = m_CentroidsOfFiducialCandidates.at(i);
      fiducialPointA = fiducialCentroid;
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + i);
      foundFiducialA = true;
      break;
    }
  }

  for (int i = 0; i < m_CentroidsOfFiducialCandidates.size(); ++i)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(i));
    double distance = fiducialNo1.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= this->GetCharacteristicDistanceAWithUpperMargin())
    {
      fiducialVectorB = m_CentroidsOfFiducialCandidates.at(i);
      fiducialPointB = fiducialCentroid;
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + i);
      foundFiducialB = true;
      break;
    }
  }

  if (!foundFiducialA || !foundFiducialB)
  {
    MITK_WARN << "Cannot identify fiducial candidates 2 and 3";
    return false;
  }
  else if (m_CentroidsOfFiducialCandidates.size() == 0)
  {
    MITK_WARN << "Too less fiducials detected. Cannot identify fiducial candidates 2 and 3";
    return false;
  }

  vectorFiducial1ToFiducialA = fiducialVectorA - m_FiducialMarkerCentroids.at(1);
  vectorFiducial1ToFiducialB = fiducialVectorB - m_FiducialMarkerCentroids.at(1);

  vnl_vector<double> crossProductVnl = vnl_cross_3d(vectorFiducial1ToFiducialA.GetVnlVector(), vectorFiducial1ToFiducialB.GetVnlVector());
  mitk::Vector3D crossProduct;
  crossProduct.SetVnlVector(crossProductVnl);

  mitk::Vector3D vectorFiducial1ToRandomLeftFiducial = m_CentroidsOfFiducialCandidates.at(0) - m_FiducialMarkerCentroids.at(1);

  double scalarProduct = (crossProduct * vectorFiducial1ToRandomLeftFiducial) /
                         (crossProduct.GetNorm() * vectorFiducial1ToRandomLeftFiducial.GetNorm());

  double alpha = acos(scalarProduct) * 57.29578; //Transform into degree
  MITK_INFO << "Scalar Product = " << alpha;

  if (alpha <= 90)
  {
    m_FiducialMarkerCentroids[3] = fiducialVectorA;
    m_FiducialMarkerCentroids[2] = fiducialVectorB;
  }
  else
  {
    m_FiducialMarkerCentroids[2] = fiducialVectorA;
    m_FiducialMarkerCentroids[3] = fiducialVectorB;
  }

  MITK_INFO << "Found Fiducial 2, PointSet: " << m_FiducialMarkerCentroids.at(2);
  MITK_INFO << "Found Fiducial 3, PointSet: " << m_FiducialMarkerCentroids.at(3);

  return true;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo4(std::vector<std::vector<double>>& distanceVectorsFiducials)
{
  double characteristicDistanceBWithLowerMargin = this->GetCharacteristicDistanceBWithLowerMargin();
  double characteristicDistanceBWithUpperMargin = this->GetCharacteristicDistanceBWithUpperMargin();

  for (int i = 0; i < distanceVectorsFiducials.size(); ++i)
  {
    std::vector<double> &distances = distanceVectorsFiducials.at(i);
    if (distances.size() < NUMBER_FIDUCIALS_NEEDED - 1)
    {
      MITK_WARN << "Cannot find fiducial 4, there aren't found enough fiducial candidates.";
      return false;
    }

    if (distances.at(0) > characteristicDistanceBWithLowerMargin &&
        distances.at(0) <= characteristicDistanceBWithUpperMargin &&
        distances.at(1) > characteristicDistanceBWithLowerMargin &&
        distances.at(1) <= characteristicDistanceBWithUpperMargin)
    {
      MITK_INFO << "Found Fiducial 4 (PointSet number " << i << ")";
      m_FiducialMarkerCentroids.insert(std::pair<int, mitk::Vector3D>(4, m_CentroidsOfFiducialCandidates.at(i)));
      distanceVectorsFiducials.erase(distanceVectorsFiducials.begin() + i);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + i);
      return true;
    }
  }
  return false;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo5()
{
  if (m_FiducialMarkerCentroids.find(2) == m_FiducialMarkerCentroids.end())
  {
    MITK_WARN << "To find fiducial No 5, fiducial No 2 has to be found before.";
    return false;
  }

  double characteristicDistanceBWithUpperMargin = this->GetCharacteristicDistanceBWithUpperMargin();

  mitk::Point3D fiducialNo2(m_FiducialMarkerCentroids.at(2));

  for (int counter = 0; counter < m_CentroidsOfFiducialCandidates.size(); ++counter)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(counter));
    double distance = fiducialNo2.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= characteristicDistanceBWithUpperMargin)
    {
      m_FiducialMarkerCentroids[5] = m_CentroidsOfFiducialCandidates.at(counter);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + counter);
      MITK_INFO << "Found Fiducial No 5, PointSet: " << m_FiducialMarkerCentroids[5];
      return true;
    }
  }

  MITK_WARN << "Cannot find fiducial No 5.";
  return false;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo6()
{
  if (m_FiducialMarkerCentroids.find(5) == m_FiducialMarkerCentroids.end())
  {
    MITK_WARN << "To find fiducial No 6, fiducial No 5 has to be found before.";
    return false;
  }

  double characteristicDistanceAWithUpperMargin = this->GetCharacteristicDistanceAWithUpperMargin();

  mitk::Point3D fiducialNo5(m_FiducialMarkerCentroids.at(5));

  for (int counter = 0; counter < m_CentroidsOfFiducialCandidates.size(); ++counter)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(counter));
    double distance = fiducialNo5.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= characteristicDistanceAWithUpperMargin)
    {
      m_FiducialMarkerCentroids[6] = m_CentroidsOfFiducialCandidates.at(counter);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + counter);
      MITK_INFO << "Found Fiducial No 6, PointSet: " << m_FiducialMarkerCentroids[6];
      return true;
    }
  }

  MITK_WARN << "Cannot find fiducial No 6.";
  return false;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo7()
{
  if (m_FiducialMarkerCentroids.find(8) == m_FiducialMarkerCentroids.end())
  {
    MITK_WARN << "To find fiducial No 7, fiducial No 8 has to be found before.";
    return false;
  }

  double characteristicDistanceAWithUpperMargin = this->GetCharacteristicDistanceAWithUpperMargin();

  mitk::Point3D fiducialNo8(m_FiducialMarkerCentroids.at(8));

  for (int counter = 0; counter < m_CentroidsOfFiducialCandidates.size(); ++counter)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(counter));
    double distance = fiducialNo8.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= characteristicDistanceAWithUpperMargin)
    {
      m_FiducialMarkerCentroids[7] = m_CentroidsOfFiducialCandidates.at(counter);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + counter);
      MITK_INFO << "Found Fiducial No 7, PointSet: " << m_FiducialMarkerCentroids[7];
      return true;
    }
  }

  MITK_WARN << "Cannot find fiducial No 7.";
  return false;
}

bool QmitkUSNavigationStepCtUsRegistration::FindFiducialNo8()
{
  if (m_FiducialMarkerCentroids.find(3) == m_FiducialMarkerCentroids.end())
  {
    MITK_WARN << "To find fiducial No 8, fiducial No 3 has to be found before.";
    return false;
  }

  double characteristicDistanceBWithUpperMargin = this->GetCharacteristicDistanceBWithUpperMargin();

  mitk::Point3D fiducialNo3(m_FiducialMarkerCentroids.at(3));

  for (int counter = 0; counter < m_CentroidsOfFiducialCandidates.size(); ++counter)
  {
    mitk::Point3D fiducialCentroid(m_CentroidsOfFiducialCandidates.at(counter));
    double distance = fiducialNo3.EuclideanDistanceTo(fiducialCentroid);
    if (distance <= characteristicDistanceBWithUpperMargin)
    {
      m_FiducialMarkerCentroids[8] = m_CentroidsOfFiducialCandidates.at(counter);
      m_CentroidsOfFiducialCandidates.erase(m_CentroidsOfFiducialCandidates.begin() + counter);
      MITK_INFO << "Found Fiducial No 8, PointSet: " << m_FiducialMarkerCentroids[8];
      return true;
    }
  }

  MITK_WARN << "Cannot find fiducial No 8.";
  return false;
}

void QmitkUSNavigationStepCtUsRegistration::OptimizeFiducialPositions()
{
  MITK_INFO << "OptimizeFiducialPositions()";
  //Initialization for planes 1 - 2 - 3 - 4 and 5 - 6 - 7 - 8
  mitk::PlaneFit::Pointer planeFit1234 = mitk::PlaneFit::New();
  mitk::PlaneFit::Pointer planeFit5678 = mitk::PlaneFit::New();
  mitk::PointSet::Pointer pointSet1234 = mitk::PointSet::New();
  mitk::PointSet::Pointer pointSet5678 = mitk::PointSet::New();
  mitk::PlaneGeometry::Pointer planeGeometry1234 = mitk::PlaneGeometry::New();
  mitk::PlaneGeometry::Pointer planeGeometry5678 = mitk::PlaneGeometry::New();
  for (int counter = 1; counter <= 4; ++counter)
  {
    pointSet1234->InsertPoint(counter - 1, m_FiducialMarkerCentroids.at(counter));
  }
  for (int counter = 5; counter <= 8; ++counter)
  {
    pointSet5678->InsertPoint(counter - 5, m_FiducialMarkerCentroids.at(counter));
  }

  //Make planes parallel
  this->CreateParallelPlanes(planeFit1234, planeFit5678,
                             pointSet1234, pointSet5678,
                             planeGeometry1234, planeGeometry5678,
                             true);


  this->MovePlanes(planeGeometry1234, planeGeometry5678, this->GetMinimalFiducialConfigurationDistance());

  //Move the points into the parallel planes
  for (int counter = 1; counter <= 4; ++counter)
  {
    this->MovePoint(planeGeometry1234, counter);
  }
  for (int counter = 5; counter <= 8; ++counter)
  {
    this->MovePoint(planeGeometry5678, counter);
  }
  MITK_INFO << "NormalVector plane 1234 = " << planeGeometry1234->GetNormal();
  MITK_INFO << "NormalVector plane 5678 = " << planeGeometry5678->GetNormal();
  MITK_INFO << "Are parallel: " << planeGeometry1234->IsParallel(planeGeometry5678);

  //Optimize now the positions of Fiducials 1 - 2 - 5 and 4 - 7 - 8
  //Initialization for planes 1 - 2 - 5 and 4 - 7 - 8
  mitk::PlaneFit::Pointer planeFit125 = mitk::PlaneFit::New();
  mitk::PlaneFit::Pointer planeFit478 = mitk::PlaneFit::New();
  mitk::PointSet::Pointer pointSet125 = mitk::PointSet::New();
  mitk::PointSet::Pointer pointSet478 = mitk::PointSet::New();
  mitk::PlaneGeometry::Pointer planeGeometry125 = mitk::PlaneGeometry::New();
  mitk::PlaneGeometry::Pointer planeGeometry478 = mitk::PlaneGeometry::New();

  pointSet125->InsertPoint(0, m_FiducialMarkerCentroids.at(1));
  pointSet125->InsertPoint(1, m_FiducialMarkerCentroids.at(2));
  pointSet125->InsertPoint(2, m_FiducialMarkerCentroids.at(5));
  //Add the points projected onto the opposite (parallel) plane to the pointset
  // By means of these projected points the calculated plane 
  // is orthogonal  to the plane 1234 and 5678.
  pointSet125->InsertPoint(3, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(1)));
  pointSet125->InsertPoint(4, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(2)));
  pointSet125->InsertPoint(5, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(5)));

  pointSet478->InsertPoint(0, m_FiducialMarkerCentroids.at(4));
  pointSet478->InsertPoint(1, m_FiducialMarkerCentroids.at(7));
  pointSet478->InsertPoint(2, m_FiducialMarkerCentroids.at(8));
  //Add the points projected onto the opposite (parallel) plane to the pointset
  // By means of these projected points the calculated plane 
  // is orthogonal  to the plane 1234 and 5678.
  pointSet478->InsertPoint(3, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(4)));
  pointSet478->InsertPoint(4, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(7)));
  pointSet478->InsertPoint(5, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(8)));

  //Make planes parallel
  this->CreateParallelPlanes(planeFit125, planeFit478,
                             pointSet125, pointSet478,
                             planeGeometry125, planeGeometry478,
                             false);

  this->MovePlanes(planeGeometry125, planeGeometry478, (2 * this->GetMinimalFiducialConfigurationDistance()));

  //Move the points into the parallel planes
  this->MovePoint(planeGeometry125, 1);
  this->MovePoint(planeGeometry125, 2);
  this->MovePoint(planeGeometry125, 5);
  this->MovePoint(planeGeometry478, 4);
  this->MovePoint(planeGeometry478, 7);
  this->MovePoint(planeGeometry478, 8);
  MITK_INFO << "NormalVector plane 125 = " << planeGeometry125->GetNormal();
  MITK_INFO << "NormalVector plane 478 = " << planeGeometry478->GetNormal();
  MITK_INFO << "Are parallel: " << planeGeometry125->IsParallel(planeGeometry478);


  //Optimize now the positions of Fiducials 1 - 3 - 8 and 4 - 5 - 6
  //Initialization for planes 1 - 3 - 8 and 4 - 5 - 6
  mitk::PlaneFit::Pointer planeFit138 = mitk::PlaneFit::New();
  mitk::PlaneFit::Pointer planeFit456 = mitk::PlaneFit::New();
  mitk::PointSet::Pointer pointSet138 = mitk::PointSet::New();
  mitk::PointSet::Pointer pointSet456 = mitk::PointSet::New();
  mitk::PlaneGeometry::Pointer planeGeometry138 = mitk::PlaneGeometry::New();
  mitk::PlaneGeometry::Pointer planeGeometry456 = mitk::PlaneGeometry::New();

  pointSet138->InsertPoint(0, m_FiducialMarkerCentroids.at(1));
  pointSet138->InsertPoint(1, m_FiducialMarkerCentroids.at(3));
  pointSet138->InsertPoint(2, m_FiducialMarkerCentroids.at(8));
  //Add the points projected onto the opposite (parallel) plane to the pointset
  // By means of these projected points the calculated plane 
  // is orthogonal  to the plane 1234 and 5678.
  pointSet138->InsertPoint(3, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(1)));
  pointSet138->InsertPoint(4, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(3)));
  pointSet138->InsertPoint(5, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(8)));

  pointSet456->InsertPoint(0, m_FiducialMarkerCentroids.at(4));
  pointSet456->InsertPoint(1, m_FiducialMarkerCentroids.at(5));
  pointSet456->InsertPoint(2, m_FiducialMarkerCentroids.at(6));
  //Add the points projected onto the opposite (parallel) plane to the pointset
  // By means of these projected points the calculated plane 
  // is orthogonal  to the plane 1234 and 5678.
  pointSet456->InsertPoint(3, planeGeometry5678->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(4)));
  pointSet456->InsertPoint(4, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(5)));
  pointSet456->InsertPoint(5, planeGeometry1234->ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(6)));

  //Make planes parallel
  this->CreateParallelPlanes(planeFit138, planeFit456,
                             pointSet138, pointSet456,
                             planeGeometry138, planeGeometry456,
                             false);

  this->MovePlanes(planeGeometry138, planeGeometry456, (2 * this->GetMinimalFiducialConfigurationDistance()));

  //Move the points into the parallel planes
  this->MovePoint(planeGeometry138, 1);
  this->MovePoint(planeGeometry138, 3);
  this->MovePoint(planeGeometry138, 8);
  this->MovePoint(planeGeometry456, 4);
  this->MovePoint(planeGeometry456, 5);
  this->MovePoint(planeGeometry456, 6);
  MITK_INFO << "NormalVector plane 138 = " << planeGeometry138->GetNormal();
  MITK_INFO << "NormalVector plane 456 = " << planeGeometry456->GetNormal();
  MITK_INFO << "Are parallel: " << planeGeometry138->IsParallel(planeGeometry456);

}

void QmitkUSNavigationStepCtUsRegistration::CreateParallelPlanes(
  mitk::PlaneFit::Pointer planeA, mitk::PlaneFit::Pointer planeB, 
  mitk::PointSet::Pointer pointSetA, mitk::PointSet::Pointer pointSetB, 
  mitk::PlaneGeometry::Pointer planeGeometryA, mitk::PlaneGeometry::Pointer planeGeometryB,
  bool minimizeInfluenceOutliers )
{
  planeA->SetInput(pointSetA);
  planeA->Update();
  mitk::PlaneGeometry::Pointer geometryA = dynamic_cast<mitk::PlaneGeometry *>(planeA->GetOutput()->GetGeometry());
  planeB->SetInput(pointSetB);
  planeB->Update();
  mitk::PlaneGeometry::Pointer geometryB = dynamic_cast<mitk::PlaneGeometry *>(planeB->GetOutput()->GetGeometry());
  mitk::DataNode::Pointer node1 = mitk::DataNode::New();


  MITK_INFO << "Angle before = " << geometryA->Angle(geometryB) * 57.29578; //Transform into degree
  //Minimize influence of outliers concerning the inclination angle of the plane:
  if (minimizeInfluenceOutliers)
  {
    bool minimizeA = true;
    bool minimizeB = true;
    while(minimizeA)
    {
      MITK_INFO << "Minimize A";
      std::vector<std::pair<double, int>> distancesA;
      distancesA.push_back(std::pair<double, int>(geometryA->Distance(m_FiducialMarkerCentroids.at(1)), 1));
      distancesA.push_back(std::pair<double, int>(geometryA->Distance(m_FiducialMarkerCentroids.at(2)), 2));
      distancesA.push_back(std::pair<double, int>(geometryA->Distance(m_FiducialMarkerCentroids.at(3)), 3));
      distancesA.push_back(std::pair<double, int>(geometryA->Distance(m_FiducialMarkerCentroids.at(4)), 4));
      std::sort(distancesA.begin(), distancesA.end());
      for (std::vector<std::pair<double, int>>::iterator it = distancesA.begin(); it != distancesA.end(); ++it)
      {
        MITK_INFO << "First = " << (*it).first << " Second = " << (*it).second;
      }
      if ((*distancesA.rbegin()).first < 0.005)
      {
        minimizeA = false;
        break;
      }
      std::vector<std::pair<double, int>>::reverse_iterator it = distancesA.rbegin();
      int fiducialNoToBeMovedToPlane1 = (*it).second;
      ++it;
      int fiducialNoToBeMovedToPlane2 = (*it).second;
      this->MovePoint(geometryA, fiducialNoToBeMovedToPlane1);
      this->MovePoint(geometryA, fiducialNoToBeMovedToPlane2);
      pointSetA->Clear();
      for (int counter = 1; counter <= 4; ++counter)
      {
        pointSetA->InsertPoint(counter - 1, m_FiducialMarkerCentroids.at(counter));
      }
      planeA = mitk::PlaneFit::New();
      planeA->SetInput(pointSetA);
      planeA->Update();
      geometryA = dynamic_cast<mitk::PlaneGeometry *>(planeA->GetOutput()->GetGeometry());

    }
    while (minimizeB)
    {
      MITK_INFO << "Minimize B";
      std::vector<std::pair<double, int>> distancesB;
      distancesB.push_back(std::pair<double, int>(geometryB->Distance(m_FiducialMarkerCentroids.at(5)), 5));
      distancesB.push_back(std::pair<double, int>(geometryB->Distance(m_FiducialMarkerCentroids.at(6)), 6));
      distancesB.push_back(std::pair<double, int>(geometryB->Distance(m_FiducialMarkerCentroids.at(7)), 7));
      distancesB.push_back(std::pair<double, int>(geometryB->Distance(m_FiducialMarkerCentroids.at(8)), 8));
      std::sort(distancesB.begin(), distancesB.end());
      for (std::vector<std::pair<double, int>>::iterator it = distancesB.begin(); it != distancesB.end(); ++it)
      {
        MITK_INFO << "First = " << (*it).first << " Second = " << (*it).second;
      }
      if ((*distancesB.rbegin()).first < 0.005)
      {
        minimizeB = false;
        break;
      }
      std::vector<std::pair<double, int>>::reverse_iterator it = distancesB.rbegin();
      int fiducialNoToBeMovedToPlane1 = (*it).second;
      ++it;
      int fiducialNoToBeMovedToPlane2 = (*it).second;
      this->MovePoint(geometryB, fiducialNoToBeMovedToPlane1);
      this->MovePoint(geometryB, fiducialNoToBeMovedToPlane2);
      pointSetB->Clear();
      for (int counter = 5; counter <= 8; ++counter)
      {
        pointSetB->InsertPoint(counter - 5, m_FiducialMarkerCentroids.at(counter));
      }
      planeB = mitk::PlaneFit::New();
      planeB->SetInput(pointSetB);
      planeB->Update();
      geometryB = dynamic_cast<mitk::PlaneGeometry *>(planeB->GetOutput()->GetGeometry());
    }
    MITK_INFO << "Angle after = " << geometryA->Angle(geometryB) * 57.29578; //Transform into degree
  }
  // End outliers minimization

  mitk::Point3D originPlaneA = geometryA->GetOrigin();
  mitk::Point3D originPlaneB = geometryB->GetOrigin();
  mitk::Vector3D normalPlaneA = geometryA->GetNormal();
  mitk::Vector3D normalPlaneB = geometryB->GetNormal();

  double scalarProduct = (normalPlaneA * normalPlaneB) /
    (normalPlaneA.GetNorm() * normalPlaneB.GetNorm());

  double alpha = acos(scalarProduct) * 57.29578; //Transform into degree
  if (alpha > 90)
  {
    normalPlaneB *= -1;
  }
  mitk::Vector3D combinedNormalPlaneA_B = 0.5 * (normalPlaneA + normalPlaneB);
  combinedNormalPlaneA_B.Normalize();

  planeGeometryA->InitializePlane(originPlaneA, combinedNormalPlaneA_B);
  planeGeometryB->InitializePlane(originPlaneB, combinedNormalPlaneA_B);
}

void QmitkUSNavigationStepCtUsRegistration::MovePlanes(mitk::PlaneGeometry::Pointer planeA, mitk::PlaneGeometry::Pointer planeB, double referenceDistance)
{
  const mitk::PlaneGeometry *constPlaneB = planeB;
  double distanceBetweenPlanes = planeA->DistanceFromPlane(constPlaneB);
  MITK_INFO << "Distance between planes before moving = " << distanceBetweenPlanes;

  //If distanceBetweenPlanes is > referenceDistance, the result will be negative,
  // otherwise it will be positive:
  double distanceToMove = 0.5 * (referenceDistance - distanceBetweenPlanes);
  const mitk::Vector3D movingVectorPositive = distanceToMove * planeA->GetNormal();
  const mitk::Vector3D movingVectorNegative = distanceToMove * planeA->GetNormal() * -1;

  //Check, whether the planeB is above planeA
  if (planeA->IsAbove(planeB->GetOrigin()))
  {
    //planeB is above planeA
    planeA->Translate(movingVectorNegative);
    planeB->Translate(movingVectorPositive);
  }
  else
  {
    //planeB is below planeA
    planeA->Translate(movingVectorPositive);
    planeB->Translate(movingVectorNegative);
  }

  const mitk::PlaneGeometry *constPlaneBMoved = planeB;
  MITK_INFO << "Distance between planes after moving = " << planeA->DistanceFromPlane(constPlaneBMoved);
}

void QmitkUSNavigationStepCtUsRegistration::MovePoint(
  mitk::PlaneGeometry::Pointer planeGeometry, int fiducialNo)
{
  MITK_INFO << "Moved Point from " << m_FiducialMarkerCentroids.at(fiducialNo);
  //Move all points outside the plane to the plane (finally all points lie into the plane):
  mitk::Point3D movedPoint = planeGeometry->
    ProjectPointOntoPlane(m_FiducialMarkerCentroids.at(fiducialNo));
  m_FiducialMarkerCentroids.at(fiducialNo)[0] = movedPoint[0];
  m_FiducialMarkerCentroids.at(fiducialNo)[1] = movedPoint[1];
  m_FiducialMarkerCentroids.at(fiducialNo)[2] = movedPoint[2];
  MITK_INFO << " to " << m_FiducialMarkerCentroids.at(fiducialNo);
}

void QmitkUSNavigationStepCtUsRegistration::DefineDataStorageImageFilter()
{
  m_IsAPointSetPredicate = mitk::TNodePredicateDataType<mitk::PointSet>::New();
  mitk::TNodePredicateDataType<mitk::Image>::Pointer isImage = mitk::TNodePredicateDataType<mitk::Image>::New();

  auto isSegmentation = mitk::NodePredicateDataType::New("Segment");

  mitk::NodePredicateOr::Pointer validImages = mitk::NodePredicateOr::New();
  validImages->AddPredicate(mitk::NodePredicateAnd::New(isImage, mitk::NodePredicateNot::New(isSegmentation)));

  mitk::NodePredicateNot::Pointer isNotAHelperObject = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object", mitk::BoolProperty::New(true)));

  m_IsOfTypeImagePredicate = mitk::NodePredicateAnd::New(validImages, isNotAHelperObject);

  mitk::NodePredicateProperty::Pointer isBinaryPredicate = mitk::NodePredicateProperty::New("binary", mitk::BoolProperty::New(true));
  mitk::NodePredicateNot::Pointer isNotBinaryPredicate = mitk::NodePredicateNot::New(isBinaryPredicate);

  mitk::NodePredicateAnd::Pointer isABinaryImagePredicate = mitk::NodePredicateAnd::New(m_IsOfTypeImagePredicate, isBinaryPredicate);
  mitk::NodePredicateAnd::Pointer isNotABinaryImagePredicate = mitk::NodePredicateAnd::New(m_IsOfTypeImagePredicate, isNotBinaryPredicate);

  m_IsASegmentationImagePredicate = mitk::NodePredicateOr::New(isABinaryImagePredicate, mitk::TNodePredicateDataType<mitk::LabelSetImage>::New());
  m_IsAPatientImagePredicate = mitk::NodePredicateAnd::New(isNotABinaryImagePredicate, mitk::NodePredicateNot::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New()));
}

void QmitkUSNavigationStepCtUsRegistration::CreateQtPartControl(QWidget *parent)
{
  ui->setupUi(parent);
  ui->floatingImageComboBox->SetPredicate(m_IsAPatientImagePredicate);
  ui->groundTruthImageComboBox->SetPredicate(m_IsAPatientImagePredicate);
  ui->ctImagesToChooseComboBox->SetPredicate(m_IsAPatientImagePredicate);

  // create signal/slot connections
  connect(ui->floatingImageComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode*)),
    this, SLOT(OnFloatingImageComboBoxSelectionChanged(const mitk::DataNode*)));
  connect(ui->groundTruthImageComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode*)),
    this, SLOT(OnGroundTruthImageComboBoxSelectionChanged(const mitk::DataNode*)));
  connect(ui->doRegistrationMarkerToImagePushButton, SIGNAL(clicked()),
    this, SLOT(OnRegisterMarkerToFloatingImageCS()));
  connect(ui->localizeFiducialMarkerPushButton, SIGNAL(clicked()),
    this, SLOT(OnLocalizeFiducials()));
  connect(ui->filterImageGroundTruthEvaluationPushButton, SIGNAL(clicked()),
    this, SLOT(OnFilterGroundTruthImage()));
  connect(ui->addCtImagePushButton, SIGNAL(clicked()),
    this, SLOT(OnAddCtImageClicked()));
  connect(ui->removeCtImagePushButton, SIGNAL(clicked()),
    this, SLOT(OnRemoveCtImageClicked()));
  connect(ui->evaluateProtocolPushButton, SIGNAL(clicked()),
    this, SLOT(OnEvaluateGroundTruthFiducialLocalizationProtocol()));
}

void QmitkUSNavigationStepCtUsRegistration::OnFloatingImageComboBoxSelectionChanged(const mitk::DataNode* node)
{
  MITK_INFO << "OnFloatingImageComboBoxSelectionChanged()";

  if (m_FloatingImage.IsNotNull())
  {
    //TODO: Define, what will happen if the imageCT is not null...
  }

  if (node == nullptr)
  {
    this->UnsetFloatingImageGeometry();
    m_FloatingImage = nullptr;
    return;
  }

  mitk::DataNode* selectedFloatingImage = ui->floatingImageComboBox->GetSelectedNode();
  if (selectedFloatingImage == nullptr)
  {
    this->UnsetFloatingImageGeometry();
    m_FloatingImage = nullptr;
    return;
  }

  mitk::Image::Pointer floatingImage = dynamic_cast<mitk::Image*>(selectedFloatingImage->GetData());
  if (floatingImage.IsNull())
  {
    MITK_WARN << "Failed to cast selected segmentation node to mitk::Image*";
    this->UnsetFloatingImageGeometry();
    m_FloatingImage = nullptr;
    return;
  }

  m_FloatingImage = floatingImage;
  this->SetFloatingImageGeometryInformation(floatingImage.GetPointer());
}

void QmitkUSNavigationStepCtUsRegistration::OnGroundTruthImageComboBoxSelectionChanged(const mitk::DataNode* node)
{
  MITK_INFO << "OnGroundTruthImageComboBoxSelectionChanged()";

  if (m_GroundTruthImage.IsNotNull())
  {
    //TODO: Define, what will happen if the imageCT is not null...
  }

  if (node == nullptr)
  {
    m_GroundTruthImage = nullptr;
    return;
  }

  mitk::DataNode* selectedGroundTruthImage = ui->groundTruthImageComboBox->GetSelectedNode();
  if (selectedGroundTruthImage == nullptr)
  {
    m_GroundTruthImage = nullptr;
    return;
  }

  mitk::Image::Pointer groundTruthImage = dynamic_cast<mitk::Image*>(selectedGroundTruthImage->GetData());
  if (groundTruthImage.IsNull())
  {
    MITK_WARN << "Failed to cast selected groundTruth image node to mitk::Image*";
    m_GroundTruthImage = nullptr;
    return;
  }

  m_GroundTruthImage = groundTruthImage;
}



void QmitkUSNavigationStepCtUsRegistration::OnRegisterMarkerToFloatingImageCS()
{
  this->CreateMarkerModelCoordinateSystemPointSet();

  //Check for initialization
  if( m_MarkerModelCoordinateSystemPointSet.IsNull() ||
      m_MarkerFloatingImageCoordinateSystemPointSet.IsNull() )
  {
    MITK_WARN << "Fiducial Landmarks are not initialized yet, cannot register";
    return;
  }

  //Retrieve fiducials
  if (m_MarkerFloatingImageCoordinateSystemPointSet->GetSize() != m_MarkerModelCoordinateSystemPointSet->GetSize())
  {
    MITK_WARN << "Not the same number of fiducials, cannot register";
    return;
  }
  else if (m_MarkerFloatingImageCoordinateSystemPointSet->GetSize() < 3)
  {
    MITK_WARN << "Need at least 3 fiducials, cannot register";
    return;
  }

  //############### conversion to vtk data types (we will use the vtk landmark based transform) ##########################
  //convert point sets to vtk poly data
  vtkSmartPointer<vtkPoints> sourcePoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> targetPoints = vtkSmartPointer<vtkPoints>::New();
  for (int i = 0; i<m_MarkerModelCoordinateSystemPointSet->GetSize(); i++)
  {
    double point[3] = { m_MarkerModelCoordinateSystemPointSet->GetPoint(i)[0],
                        m_MarkerModelCoordinateSystemPointSet->GetPoint(i)[1],
                        m_MarkerModelCoordinateSystemPointSet->GetPoint(i)[2] };
    sourcePoints->InsertNextPoint(point);

    double point_targets[3] = { m_MarkerFloatingImageCoordinateSystemPointSet->GetPoint(i)[0],
                                m_MarkerFloatingImageCoordinateSystemPointSet->GetPoint(i)[1],
                                m_MarkerFloatingImageCoordinateSystemPointSet->GetPoint(i)[2] };
    targetPoints->InsertNextPoint(point_targets);
  }

  //########################### here, the actual transform is computed ##########################
  //compute transform
  vtkSmartPointer<vtkLandmarkTransform> transform = vtkSmartPointer<vtkLandmarkTransform>::New();
  transform->SetSourceLandmarks(sourcePoints);
  transform->SetTargetLandmarks(targetPoints);
  transform->SetModeToRigidBody();
  transform->Modified();
  transform->Update();
  //compute FRE of transform

  double FRE = mitk::StaticIGTHelperFunctions::ComputeFRE(m_MarkerModelCoordinateSystemPointSet, m_MarkerFloatingImageCoordinateSystemPointSet, transform);
  MITK_INFO << "FRE: " << FRE << " mm";
  if (m_PerformingGroundTruthProtocolEvaluation)
  {
    m_GroundTruthProtocolFRE.push_back(FRE);
  }
  //#############################################################################################

  //############### conversion back to itk/mitk data types ##########################
  //convert from vtk to itk data types
  itk::Matrix<float, 3, 3> rotationFloat = itk::Matrix<float, 3, 3>();
  itk::Vector<float, 3> translationFloat = itk::Vector<float, 3>();
  itk::Matrix<double, 3, 3> rotationDouble = itk::Matrix<double, 3, 3>();
  itk::Vector<double, 3> translationDouble = itk::Vector<double, 3>();

  vtkSmartPointer<vtkMatrix4x4> m = transform->GetMatrix();
  for (int k = 0; k<3; k++) for (int l = 0; l<3; l++)
  {
    rotationFloat[k][l] = m->GetElement(k, l);
    rotationDouble[k][l] = m->GetElement(k, l);

  }
  for (int k = 0; k<3; k++)
  {
    translationFloat[k] = m->GetElement(k, 3);
    translationDouble[k] = m->GetElement(k, 3);
  }
  //create mitk affine transform 3D and save it to the class member
  m_TransformMarkerCSToFloatingImageCS = mitk::AffineTransform3D::New();
  m_TransformMarkerCSToFloatingImageCS->SetMatrix(rotationDouble);
  m_TransformMarkerCSToFloatingImageCS->SetOffset(translationDouble);
  MITK_INFO << m_TransformMarkerCSToFloatingImageCS;
  //################################################################

  //############### object is transformed ##########################
  //transform surface/image
  //only move image if we have one. Sometimes, this widget is used just to register point sets without images.

  /*if (m_ImageNode.IsNotNull())
  {
    //first we have to store the original ct image transform to compose it with the new transform later
    mitk::AffineTransform3D::Pointer imageTransform = m_ImageNode->GetData()->GetGeometry()->GetIndexToWorldTransform();
    imageTransform->Compose(mitkTransform);
    mitk::AffineTransform3D::Pointer newImageTransform = mitk::AffineTransform3D::New(); //create new image transform... setting the composed directly leads to an error
    itk::Matrix<mitk::ScalarType, 3, 3> rotationFloatNew = imageTransform->GetMatrix();
    itk::Vector<mitk::ScalarType, 3> translationFloatNew = imageTransform->GetOffset();
    newImageTransform->SetMatrix(rotationFloatNew);
    newImageTransform->SetOffset(translationFloatNew);
    m_ImageNode->GetData()->GetGeometry()->SetIndexToWorldTransform(newImageTransform);
  }*/

  //If this option is set, each point will be transformed and the acutal coordinates of the points change.

  if( !m_PerformingGroundTruthProtocolEvaluation )
  {
    mitk::PointSet* pointSet_orig = m_MarkerModelCoordinateSystemPointSet;
    mitk::PointSet::Pointer pointSet_moved = mitk::PointSet::New();

    for (int i = 0; i < pointSet_orig->GetSize(); i++)
    {
      pointSet_moved->InsertPoint(m_TransformMarkerCSToFloatingImageCS->TransformPoint(pointSet_orig->GetPoint(i)));
    }

    pointSet_orig->Clear();
    for (int i = 0; i < pointSet_moved->GetSize(); i++)
      pointSet_orig->InsertPoint(pointSet_moved->GetPoint(i));

    //Do a global reinit
    mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
  }

}

void QmitkUSNavigationStepCtUsRegistration::OnLocalizeFiducials()
{
  m_FiducialMarkerCentroids.clear();
  m_CentroidsOfFiducialCandidates.clear();
  if (m_MarkerFloatingImageCoordinateSystemPointSet.IsNotNull())
  {
    m_MarkerFloatingImageCoordinateSystemPointSet->Clear();
  }

  if (!this->FilterFloatingImage())
  {
    QMessageBox msgBox;
    msgBox.setText("Cannot perform filtering of the image. The floating image = nullptr.");
    msgBox.exec();
    return;
  }

  this->GetCentroidsOfLabeledObjects();

  if (!this->EliminateFiducialCandidatesByEuclideanDistances() ||
      m_CentroidsOfFiducialCandidates.size() != NUMBER_FIDUCIALS_NEEDED)
  {
    QMessageBox msgBox;
    QString text = QString("Have found %1 instead of 8 fiducial candidates.\
      Cannot perform fiducial localization procedure.").arg(m_CentroidsOfFiducialCandidates.size());
    msgBox.setText(text);
    msgBox.exec();
    return;
  }

  //Before calling NumerateFiducialMarks it must be sure,
  // that there rested only 8 fiducial candidates.
  this->NumerateFiducialMarks();
}

void QmitkUSNavigationStepCtUsRegistration::OnFilterGroundTruthImage()
{
  /*if (m_GroundTruthImage.IsNull())
  {
    return;
  }*/

  this->ShowGroundTruthMarkerEdges();

}

void QmitkUSNavigationStepCtUsRegistration::OnAddCtImageClicked()
{
  mitk::DataNode* selectedCtImage = ui->ctImagesToChooseComboBox->GetSelectedNode();
  if (selectedCtImage == nullptr)
  {
    return;
  }

  mitk::Image::Pointer ctImage = dynamic_cast<mitk::Image*>(selectedCtImage->GetData());
  if (ctImage.IsNull())
  {
    MITK_WARN << "Failed to cast selected segmentation node to mitk::Image*";
    return;
  }
  QString name = QString::fromStdString(selectedCtImage->GetName());

  for( int counter = 0; counter < ui->chosenCtImagesListWidget->count(); ++counter)
  {
    MITK_INFO << ui->chosenCtImagesListWidget->item(counter)->text() << " - " << counter;
    MITK_INFO << m_ImagesGroundTruthProtocol.at(counter).GetPointer();
    if (ui->chosenCtImagesListWidget->item(counter)->text().compare(name) == 0)
    {
      MITK_INFO << "CT image already exist in list of chosen CT images. Do not add the image.";
      return;
    }
  }

  ui->chosenCtImagesListWidget->addItem(name);
  m_ImagesGroundTruthProtocol.push_back(ctImage);
}

void QmitkUSNavigationStepCtUsRegistration::OnRemoveCtImageClicked()
{
  int position = ui->chosenCtImagesListWidget->currentRow();
  if (ui->chosenCtImagesListWidget->count() == 0 || position < 0)
  {
    return;
  }

  m_ImagesGroundTruthProtocol.erase(m_ImagesGroundTruthProtocol.begin() + position);
  QListWidgetItem *item = ui->chosenCtImagesListWidget->currentItem();
  ui->chosenCtImagesListWidget->removeItemWidget(item);
  delete item;
}

void QmitkUSNavigationStepCtUsRegistration::OnEvaluateGroundTruthFiducialLocalizationProtocol()
{
  m_GroundTruthProtocolFRE.clear();
  if (m_ImagesGroundTruthProtocol.size() != 6)
  {
    QMessageBox msgBox;
    msgBox.setText("For evaluating the Ground-Truth-Fiducial-Localization-Protocol there must be loaded 6 different CT images.");
    msgBox.exec();
    return;
  }

  m_PerformingGroundTruthProtocolEvaluation = true;
  for (int cycleNo = 0; cycleNo < m_ImagesGroundTruthProtocol.size(); ++cycleNo)
  {
    m_FloatingImage = m_ImagesGroundTruthProtocol.at(cycleNo);
    this->SetFloatingImageGeometryInformation(m_FloatingImage.GetPointer());

    this->OnLocalizeFiducials();
    this->OnRegisterMarkerToFloatingImageCS();
  }

  m_PerformingGroundTruthProtocolEvaluation = false;
}
