#include "mitkPointSet.h"
#include <mitkOperation.h>
#include <mitkOperationActor.h>
#include <mitkPointOperation.h>
#include "mitkInteractionConst.h"
#include "mitkRenderWindow.h"

//##ModelId=3F0177E901BD
mitk::PointSet::PointSet()
{
  m_ItkData = DataType::New();
  PointDataContainer::Pointer pointData = PointDataContainer::New();
  m_ItkData->SetPointData(pointData);
  m_Geometry3D->Initialize();
}

//##ModelId=3F0177E901BE
mitk::PointSet::~PointSet()
{}

//##ModelId=3F0177E901C1
int mitk::PointSet::GetSize()
{
  return m_ItkData->GetNumberOfPoints();
}

//##ModelId=3F0177E901CC
mitk::PointSet::DataType::Pointer mitk::PointSet::GetPointSet() const
{
  return m_ItkData;
}

//##ModelId=3F0177E901DE
//##Documentation
//##@brief searches a point in the list with a given precision
int mitk::PointSet::SearchPoint(Point3D point, float distance )
{
//in is the given point to search
//out is the point which is checked to be the searched point
//pout is a pointer on out to work with itkPointSet
  PointType in, out;

  //converting point:
  in[0] = point[0];
  in[1] = point[1];
  in[2] = point[2];
  out.Fill(0);

  //searching the first point in the Set, that is +- distance far away from the given point
  unsigned int i;
  PointsContainer::Iterator it, end;
  end = m_ItkData->GetPoints()->End();
 	for (it = m_ItkData->GetPoints()->Begin(), i=0; it != end; it++, i++)
	{
    bool ok = m_ItkData->GetPoints()->GetElementIfIndexExists(it->Index(), &out);//could be optimized
    if (!ok)
      return -1;
    else if (in == out)//if totaly equal
      return it->Index();

    //distance calculation
		if ( ( in[0] >= ( out[0] - distance ) ) && ( in[0] <= ( out[0] + distance ) ) &&
			 ( in[1] >= ( out[1] - distance ) ) && ( in[1] <= ( out[1] + distance ) ) &&
			 ( in[2] >= ( out[2] - distance ) ) && ( in[2] <= ( out[2] + distance ) ) )
      return it->Index();
	}
	return -1;
}

//##ModelId=3F0177E901CE
//##Documentation
//##@brief check if index exists. If it doesn't exist, then return 0,0,0
const mitk::PointSet::PointType mitk::PointSet::GetPoint(int position)
{
  PointType out;
  out.Fill(0);

  if (m_ItkData->GetPoints()->IndexExists(position))
  {
    m_ItkData->GetPoint(position, &out);
    return out;
  }
  else
    return out;
}

const mitk::Point3D mitk::PointSet::GetItkPoint(int position)
{
	mitk::Point3D itkout;
  PointType out;
  out.Fill(0);
  
  //if the Index exists... if not, then return 0,0,0
  if (m_ItkData->GetPoints()->IndexExists(position))
  {
    m_ItkData->GetPoint(position, &out);
  }

  itkout[0] = out[0];
  itkout[1] = out[1];
  itkout[2] = out[2];
  return itkout;
}

bool mitk::PointSet::IndexExists(int position)
{
  return m_ItkData->GetPoints()->IndexExists(position);
}

//##ModelId=3F0177E901DC
bool mitk::PointSet::GetSelectInfo(int position)
{
  if (m_ItkData->GetPoints()->IndexExists(position))
	{
    PointDataType pointData = {0, false, PTUNDEFINED};
    m_ItkData->GetPointData(position, &pointData);
    return pointData.selected;
	}
	else
		return false;
}

//##ModelId=3F05B07B0147
const int mitk::PointSet::GetNumberOfSelected()
{
  int numberOfSelected = 0;
  for (PointDataIterator it = m_ItkData->GetPointData()->Begin(); it != m_ItkData->GetPointData()->End(); it++)
  {
    if (it->Value().selected == true)
      numberOfSelected++;
  }
  return numberOfSelected;
}

int mitk::PointSet::SearchSelectedPoint()
{
  for (PointDataIterator it = m_ItkData->GetPointData()->Begin(); it != m_ItkData->GetPointData()->End(); it++)
  {
    if (it->Value().selected == true)
      return it->Index();
  }
  return -1;
}

//##ModelId=3F0177E901BF
//##Documentation
//## @brief executes the given Operation
void mitk::PointSet::ExecuteOperation(Operation* operation)
{

	switch (operation->GetOperationType())
	{
	case OpNOTHING:
		break;
	case OpINSERT://inserts the point at the given position and selects it. 
		{
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);
      
      int position = pointOp->GetIndex();

      PointType pt;
      pt.CastFrom(pointOp->GetPoint());

      m_ItkData->GetPoints()->InsertElement(position, pt);

      PointDataType pointData = {pointOp->GetIndex(), pointOp->GetSelected(), pointOp->GetPointType()};
      m_ItkData->GetPointData()->InsertElement(position, pointData);
		}
		break;
	case OpMOVE://moves the point given by index
		{
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

			PointType pt;
      pt.CastFrom(pointOp->GetPoint());
      m_ItkData->SetPoint(pointOp->GetIndex(), pt);
		}
		break;
	case OpREMOVE://removes the point at given by position 
		{
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      m_ItkData->GetPoints()->DeleteIndex((unsigned)pointOp->GetIndex());
      m_ItkData->GetPointData()->DeleteIndex((unsigned)pointOp->GetIndex());
		}
		break;
  case OpSELECTPOINT://select the given point
		{
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      PointDataType pointData = {0, false, PTUNDEFINED};
      m_ItkData->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.selected = true;
      m_ItkData->SetPointData(pointOp->GetIndex(), pointData);
		}
		break;
	case OpDESELECTPOINT://unselect the given point
		{
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      PointDataType pointData = {0, false, PTUNDEFINED};
      m_ItkData->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.selected = false;
      m_ItkData->SetPointData(pointOp->GetIndex(), pointData);
		}
		break;
  case OpSETPOINTTYPE:
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);
      PointDataType pointData = {0, false, PTUNDEFINED};
      m_ItkData->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.pointSpec = pointOp->GetPointType();
      m_ItkData->SetPointData(pointOp->GetIndex(), pointData);
    }
    break;
	default:
		NULL;
	}
	//to tell the mappers, that the data is modifierd and has to be updated
	this->Modified();

  ((const itk::Object*)this)->InvokeEvent(itk::EndEvent());

  mitk::RenderWindow::UpdateAllInstances();
}

//##ModelId=3F0177E901EE
void mitk::PointSet::UpdateOutputInformation()
{
  if ( this->GetSource( ) )
  {
      this->GetSource( )->UpdateOutputInformation( );
  }
  const DataType::BoundingBoxType *bb = m_ItkData->GetBoundingBox();
  BoundingBox::BoundsArrayType itkBounds = bb->GetBounds();
  float mitkBounds[6];

  //for assignment see Geometry3d::SetBounds(const float bounds)
  mitkBounds[0] = itkBounds[0];
  mitkBounds[1] = itkBounds[1];
  mitkBounds[2] = itkBounds[2];
  mitkBounds[3] = itkBounds[3];
  mitkBounds[4] = itkBounds[4];
  mitkBounds[5] = itkBounds[5];

  m_Geometry3D->SetBounds(mitkBounds);
}

//##ModelId=3F0177E901FB
void mitk::PointSet::SetRequestedRegionToLargestPossibleRegion()
{
}
//##ModelId=3F0177E901FD
bool mitk::PointSet::RequestedRegionIsOutsideOfTheBufferedRegion()
{
    return false;
}
//##ModelId=3F0177E901FF
bool mitk::PointSet::VerifyRequestedRegion()
{
    return true;
}
//##ModelId=3F0177E9020B
void mitk::PointSet::SetRequestedRegion(itk::DataObject*)
{
}

