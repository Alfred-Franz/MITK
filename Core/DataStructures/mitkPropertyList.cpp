/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include "mitkPropertyList.h"
#include <mitkXMLWriter.h>
#include <mitkXMLReader.h>
#include <mitkMapClassIDToClassName.h>

const std::string mitk::PropertyList::XML_NODE_NAME = "propertyList";

//##ModelId=3D6A0E9E0029
mitk::BaseProperty::Pointer mitk::PropertyList::GetProperty(const char *propertyKey) const
{
    PropertyMap::const_iterator it;
    
    it=m_Properties.find( propertyKey );
    if(it!=m_Properties.end() &&  it->second.second )
      return it->second.first;
    else 
        return NULL;
}

//##ModelId=3D78B966005F
void mitk::PropertyList::SetProperty(const char* propertyKey, BaseProperty* property)
{
    PropertyMap::iterator it;
    
    it=m_Properties.find( propertyKey );
    //is a property with key @a propertyKey contained in the list?
    if(it!=m_Properties.end())
        //yes?
    {
        //is the property contained in the list identical to the new one?
        if( it->second.first == property) {
            // yes? do nothing and return.
            return;
  }
        //no? erase the old entry.
        it->second.first=NULL;
        m_Properties.erase(it);
    }

    //no? add/replace it.
  //std::pair<PropertyMap::iterator, bool> o=
       PropertyMapElementType newProp;
       newProp.first = propertyKey;
       newProp.second = std::pair<BaseProperty::Pointer,bool>(property,true);
       m_Properties.insert ( newProp );
       Modified();
}

//##ModelId=3E38FEFE0125
mitk::PropertyList::PropertyList()
{
}


//##ModelId=3E38FEFE0157
mitk::PropertyList::~PropertyList()
{
  Clear();
}

unsigned long mitk::PropertyList::GetMTime() const
{
    PropertyMap::const_iterator it = m_Properties.begin(), end = m_Properties.end();
  for(;it!=end;++it)
  {
    if(Superclass::GetMTime()<it->second.first->GetMTime())
    {
      Modified();
      break;
    }
  }
    return Superclass::GetMTime();
}
//##ModelId=3EF1B0160286
bool mitk::PropertyList::DeleteProperty(const char* propertyKey)
{
  PropertyMap::iterator it;  
  it=m_Properties.find( propertyKey );

  if(it!=m_Properties.end())
  {
    it->second.first=NULL;
    m_Properties.erase(it);
    Modified();
    return true;
  }
  return false;
}

mitk::PropertyList::Pointer mitk::PropertyList::Clone()
{
  mitk::PropertyList::Pointer newPropertyList = PropertyList::New();

  // copy the map
  newPropertyList->m_Properties = m_Properties;

  return newPropertyList.GetPointer();
}

void mitk::PropertyList::Clear()
{
  PropertyMap::iterator it = m_Properties.begin(), end = m_Properties.end();
  while(it!=end)
  {
    it->second.first = NULL;
    ++it;
  }
  m_Properties.clear();
}

bool mitk::PropertyList::WriteXMLData( mitk::XMLWriter& xmlWriter ) 
{
  const mitk::PropertyList::PropertyMap* map = GetMap();
  mitk::PropertyList::PropertyMap::const_iterator i = map->begin();
  const mitk::PropertyList::PropertyMap::const_iterator end = map->end(); 

  while ( i != end ) {
    xmlWriter.BeginNode( (*i).second.first->GetXMLNodeName() );
    xmlWriter.WriteProperty( XMLIO::CLASS_NAME, mitk::MapClassIDToClassName::MapIDToName(typeid( *(*i).second.first ).name()) );
    xmlWriter.WriteProperty( XMLReader::PROPERTY_KEY, (*i).first.c_str() );
    (*i).second.first->WriteXMLData( xmlWriter );
    if((*i).first==StringProperty::PATH)
      xmlWriter.SetOriginPath((*i).second.first->GetValueAsString());
    *i++;
    xmlWriter.EndNode();
  }

  return true;
}

bool mitk::PropertyList::ReadXMLData( XMLReader& xmlReader )
{

  if ( xmlReader.Goto( BaseProperty::XML_NODE_NAME ) ) {

    do {
      BaseProperty::Pointer property = dynamic_cast<BaseProperty*>( xmlReader.CreateObject().GetPointer() );

      if ( property.IsNotNull() ) 
      {
        property->ReadXMLData( xmlReader );
        std::string name;
        xmlReader.GetAttribute( XMLReader::PROPERTY_KEY, name );

        if ( !name.empty() )
          SetProperty( name.c_str(), property );
      }
    } while ( xmlReader.GotoNext() );
    xmlReader.GotoParent();
  }

  return true;
}

const std::string& mitk::PropertyList::GetXMLNodeName() const
{
  return XML_NODE_NAME;
}

bool mitk::PropertyList::IsEnabled(const char *propertyKey) {
  PropertyMap::iterator it = m_Properties.find( propertyKey );
  if (it != m_Properties.end() && it->second.second) {
   return true;
  } else {
    return false;
  }
}
void mitk::PropertyList::SetEnabled(const char *propertyKey,bool enabled) {
  PropertyMap::iterator it = m_Properties.find( propertyKey );
  if (it != m_Properties.end() && it->second.second != enabled) {
    it->second.second = enabled;
    this->Modified();
  }
}
