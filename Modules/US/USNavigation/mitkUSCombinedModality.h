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

#ifndef MITKUSCombinedModality_H_HEADER_INCLUDED_
#define MITKUSCombinedModality_H_HEADER_INCLUDED_

#include <MitkUSNavigationExports.h>
#include "mitkUSDevice.h"
#include "mitkImageSource.h"
#include "mitkAbstractUltrasoundTrackerDevice.h"
#include "mitkNavigationDataSource.h"

// Microservices
#include <mitkServiceInterface.h>
#include <usServiceRegistration.h>

namespace itk {
  template<class T> class SmartPointer;
}

namespace mitk {
  class USControlInterfaceBMode;
  class USControlInterfaceProbes;
  class USControlInterfaceDoppler;

  /**
   * \brief Combination of USDevice and NavigationDataSource.
   * This class can be used as an ImageSource subclass. Additionally tracking data be
   * retrieved from the NavigationDataSource returned by GetTrackingDevice().
   *
   * A calibration of the ultrasound image stream to the navigation datas can be set
   * for the currently active zoom level (of the ultrasound device) by SetCalibration().
   * The ultrasound images are transformed according to this calibration in the
   * GenerateData() method.
   */
  class MITKUSNAVIGATION_EXPORT USCombinedModality : public mitk::AbstractUltrasoundTrackerDevice
  {
  public:

    static const std::string DeviceClassIdentifier;
    static const char*       DefaultProbeIdentifier;
    static const char*       ProbeAndDepthSeperator;
    static const std::string US_INTERFACE_NAME;
    static const std::string US_PROPKEY_DEVICENAME;
    static const std::string US_PROPKEY_CLASS;
    static const std::string US_PROPKEY_ID;


    void UnregisterOnService();

    void RegisterAsMicroservice() override;

    mitkClassMacro(USCombinedModality, mitk::AbstractUltrasoundTrackerDevice);
    mitkNewMacro3Param(USCombinedModality, USDevice::Pointer, itk::SmartPointer<NavigationDataSource>, bool);


    /**
    * \brief Wrapper for returning custom control interface of the UltrasoundDevice.
    */
    virtual itk::SmartPointer<USAbstractControlInterface> GetControlInterfaceCustom() override;

    /**
    * \brief Wrapper for returning B mode control interface of the UltrasoundDevice.
    */
    virtual itk::SmartPointer<USControlInterfaceBMode> GetControlInterfaceBMode() override;

    /**
    * \brief Wrapper for returning probes control interface of the UltrasoundDevice.
    */
    virtual itk::SmartPointer<USControlInterfaceProbes> GetControlInterfaceProbes() override;

    /**
    * \brief Wrapper for returning doppler control interface of the UltrasoundDevice.
    */
    virtual itk::SmartPointer<USControlInterfaceDoppler> GetControlInterfaceDoppler() override;

  protected:
    USCombinedModality( USDevice::Pointer usDevice,
                        itk::SmartPointer<NavigationDataSource> trackingDevice,
                        bool trackedUltrasoundActive = false );
    virtual ~USCombinedModality();

    /**
    * \brief Initializes UltrasoundDevice.
    */
    //___virtual bool OnInitialization();

    /**
    * \brief Connects UltrasoundDevice.
    */
    //___virtual bool OnConnection();

    /**
    * \brief Disconnects UltrasoundDevice.
    */
    //___virtual bool OnDisconnection();

    /**
    * \brief Activates UltrasoundDevice.
    */
    //___virtual bool OnActivation();

    /**
    * \brief Deactivates UltrasoundDevice.
    */
   //___ virtual bool OnDeactivation();

    /**
    * \brief Freezes or unfreezes UltrasoundDevice.
    */
    //___virtual void OnFreeze(bool);



    /**
    * \brief Grabs the next frame from the input.
    * This method is called internally, whenever Update() is invoked by an Output.
    */
    void GenerateData() override;


    private:
      /**
      *  \brief The device's ServiceRegistration object that allows to modify it's Microservice registraton details.
      */
      us::ServiceRegistration<Self>           m_ServiceRegistration;

      /**
      * \brief Properties of the device's Microservice.
      */
      us::ServiceProperties                   m_ServiceProperties;
  };
} // namespace mitk
MITK_DECLARE_SERVICE_INTERFACE(mitk::USCombinedModality, "org.mitk.services.USCombinedModality")
#endif // MITKUSCombinedModality_H_HEADER_INCLUDED_
