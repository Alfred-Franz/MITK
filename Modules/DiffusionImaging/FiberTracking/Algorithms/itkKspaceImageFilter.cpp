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

#ifndef __itkKspaceImageFilter_txx
#define __itkKspaceImageFilter_txx

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "itkKspaceImageFilter.h"
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkImageRegionIterator.h>
#include <itkImageFileWriter.h>

#define _USE_MATH_DEFINES
#include <math.h>

namespace itk {

template< class TPixelType >
KspaceImageFilter< TPixelType >
::KspaceImageFilter()
    : m_Z(0)
    , m_UseConstantRandSeed(false)
    , m_SpikesPerSlice(0)
    , m_IsBaseline(true)
{
    m_DiffusionGradientDirection.Fill(0.0);

    m_CoilPosition.Fill(0.0);
}

template< class TPixelType >
void KspaceImageFilter< TPixelType >
::BeforeThreadedGenerateData()
{
    m_Spike = vcl_complex<double>(0,0);

    typename OutputImageType::Pointer outputImage = OutputImageType::New();
    itk::ImageRegion<2> region; region.SetSize(0, m_OutSize[0]);  region.SetSize(1, m_OutSize[1]);
    outputImage->SetLargestPossibleRegion( region );
    outputImage->SetBufferedRegion( region );
    outputImage->SetRequestedRegion( region );
    outputImage->Allocate();
    outputImage->FillBuffer(m_Spike);

    m_KSpaceImage = InputImageType::New();
    m_KSpaceImage->SetLargestPossibleRegion( region );
    m_KSpaceImage->SetBufferedRegion( region );
    m_KSpaceImage->SetRequestedRegion( region );
    m_KSpaceImage->Allocate();
    m_KSpaceImage->FillBuffer(0.0);

    m_Gamma = 42576000;    // Gyromagnetic ratio in Hz/T
    if ( m_Parameters.m_SignalGen.m_EddyStrength>0 && m_DiffusionGradientDirection.GetNorm()>0.001)
    {
        m_DiffusionGradientDirection.Normalize();
        m_DiffusionGradientDirection = m_DiffusionGradientDirection *  m_Parameters.m_SignalGen.m_EddyStrength/1000 *  m_Gamma;
        m_IsBaseline = false;
    }

    this->SetNthOutput(0, outputImage);

    m_Transform =  m_Parameters.m_SignalGen.m_ImageDirection;
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            m_Transform[i][j] *=  m_Parameters.m_SignalGen.m_ImageSpacing[j];

    double a = m_Parameters.m_SignalGen.m_ImageRegion.GetSize(0)*m_Parameters.m_SignalGen.m_ImageSpacing[0];
    double b = m_Parameters.m_SignalGen.m_ImageRegion.GetSize(1)*m_Parameters.m_SignalGen.m_ImageSpacing[1];
    double diagonal = sqrt(a*a+b*b)/1000;   // image diagonal in m

    switch (m_Parameters.m_SignalGen.m_CoilSensitivityProfile)
    {
    case SignalGenerationParameters::COIL_CONSTANT:
    {
        m_CoilSensitivityFactor = 1;
        break;
    }
    case SignalGenerationParameters::COIL_LINEAR:
    {
        m_CoilSensitivityFactor = -1/diagonal;
        break;
    }
    case SignalGenerationParameters::COIL_EXPONENTIAL:
    {
        m_CoilSensitivityFactor = -log(0.1)/diagonal;
        break;
    }
    }
}

template< class TPixelType >
double KspaceImageFilter< TPixelType >::CoilSensitivity(DoubleVectorType& pos)
{
    // *************************************************************************
    // Coil ring is moving with excited slice (FIX THIS SOMETIME)
    m_CoilPosition[2] = pos[2];
    // *************************************************************************

    switch (m_Parameters.m_SignalGen.m_CoilSensitivityProfile)
    {
    case SignalGenerationParameters::COIL_CONSTANT:
        return 1;
    case SignalGenerationParameters::COIL_LINEAR:
    {
        DoubleVectorType diff = pos-m_CoilPosition;
        double sens = diff.GetNorm()*m_CoilSensitivityFactor + 1;
        if (sens<0)
            sens = 0;
        return sens;
    }
    case SignalGenerationParameters::COIL_EXPONENTIAL:
    {
        DoubleVectorType diff = pos-m_CoilPosition;
        double dist = diff.GetNorm();
        return exp(-dist*m_CoilSensitivityFactor);
    }
    default:
        return 1;
    }
}

template< class TPixelType >
void KspaceImageFilter< TPixelType >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, ThreadIdType threadID)
{
    itk::Statistics::MersenneTwisterRandomVariateGenerator::Pointer randGen = itk::Statistics::MersenneTwisterRandomVariateGenerator::New();
    randGen->SetSeed();
    if (m_UseConstantRandSeed)  // always generate the same random numbers?
        randGen->SetSeed(threadID*100);
    else
        randGen->SetSeed();

    typename OutputImageType::Pointer outputImage = static_cast< OutputImageType * >(this->ProcessObject::GetOutput(0));

    ImageRegionIterator< OutputImageType > oit(outputImage, outputRegionForThread);

    typedef ImageRegionConstIterator< InputImageType > InputIteratorType;

    double kxMax = outputImage->GetLargestPossibleRegion().GetSize(0);  // k-space size in x-direction
    double kyMax = outputImage->GetLargestPossibleRegion().GetSize(1);  // k-space size in y-direction
    double xMax = m_CompartmentImages.at(0)->GetLargestPossibleRegion().GetSize(0); // scanner coverage in x-direction
    double yMax = m_CompartmentImages.at(0)->GetLargestPossibleRegion().GetSize(1); // scanner coverage in y-direction

    double numPix = kxMax*kyMax;
    double dt =  m_Parameters.m_SignalGen.m_tLine/kxMax;
    double fromMaxEcho = -m_Parameters.m_SignalGen.m_tLine*kyMax/2-dt*kxMax/2;

    double upsampling = xMax/kxMax;     //  discrepany between k-space resolution and image resolution
    double yMaxFov = kyMax*upsampling;  //  actual FOV in y-direction (in x-direction xMax==FOV)

    int xRingingOffset = xMax-kxMax;
    int yRingingOffset = yMaxFov-kyMax;

    double noiseVar = m_Parameters.m_SignalGen.m_PartialFourier*m_Parameters.m_SignalGen.m_NoiseVariance/(yMaxFov*kxMax); // adjust noise variance since it is the intended variance in physical space and not in k-space

    while( !oit.IsAtEnd() )
    {
        itk::Index< 2 > kIdx;
        kIdx[0] = oit.GetIndex()[0];
        kIdx[1] = oit.GetIndex()[1];

        // dephasing time
        double t= fromMaxEcho + ((double)kIdx[1]*kxMax+(double)kIdx[0])*dt;

        // readout time
        double tall = 0;
//        if (!m_Parameters.m_SignalGen.m_ReversePhase)
            tall = ((double)kIdx[1]*kxMax+(double)kIdx[0])*dt;
//        else
//            tall = ((double)(kyMax-1-kIdx[1])*kxMax+(double)kIdx[0])*dt;

        // calculate eddy current decay factor
        double eddyDecay = 0;
        if ( m_Parameters.m_SignalGen.m_EddyStrength>0)
            eddyDecay = exp(-tall/m_Parameters.m_SignalGen.m_Tau );

        // calcualte signal relaxation factors
        std::vector< double > relaxFactor;
        if ( m_Parameters.m_SignalGen.m_DoSimulateRelaxation)
            for (unsigned int i=0; i<m_CompartmentImages.size(); i++)
                relaxFactor.push_back( exp(-( m_Parameters.m_SignalGen.m_tEcho+t)/m_T2.at(i) -fabs(t)/ m_Parameters.m_SignalGen.m_tInhom)*(1.0-exp(-m_Parameters.m_SignalGen.m_tRep/m_T1.at(i))) );

        // reverse phase
        if (!m_Parameters.m_SignalGen.m_ReversePhase)
            kIdx[1] = kyMax-1-kIdx[1];

        // partial fourier
        bool pf = false;
        if (kIdx[1]>kyMax*m_Parameters.m_SignalGen.m_PartialFourier)
            pf = true;

        // reverse readout direction
        if (oit.GetIndex()[1]%2 == 1)
            kIdx[0] = kxMax-kIdx[0]-1;

//        if (!pf)
//            m_KSpaceImage->SetPixel(kIdx, t );

        // rearrange slice
        if( kIdx[0] <  kxMax/2 )
            kIdx[0] = kIdx[0] + kxMax/2;
        else
            kIdx[0] = kIdx[0] - kxMax/2;

        if( kIdx[1] <  kyMax/2 )
            kIdx[1] = kIdx[1] + kyMax/2;
        else
            kIdx[1] = kIdx[1] - kyMax/2;

        // add ghosting
        double kx = kIdx[0];
        double ky = kIdx[1];
        if (oit.GetIndex()[1]%2 == 1)
            kx -= m_Parameters.m_SignalGen.m_KspaceLineOffset;    // add gradient delay induced offset
        else
            kx += m_Parameters.m_SignalGen.m_KspaceLineOffset;    // add gradient delay induced offset

        if (!pf)
        {
            // add gibbs ringing offset (cropps k-space)
            if (kx>=kxMax/2)
                kx += xRingingOffset;
            if (ky>=kyMax/2)
                ky += yRingingOffset;

            vcl_complex<double> s(0,0);
            InputIteratorType it(m_CompartmentImages.at(0), m_CompartmentImages.at(0)->GetLargestPossibleRegion() );
            while( !it.IsAtEnd() )
            {
                double x = it.GetIndex()[0]-xMax/2;
                double y = it.GetIndex()[1]-yMax/2;

                DoubleVectorType pos; pos[0] = x; pos[1] = y; pos[2] = m_Z;
                pos = m_Transform*pos/1000;   // vector from image center to current position (in meter)

                vcl_complex<double> f(0, 0);

                // sum compartment signals and simulate relaxation
                for (unsigned int i=0; i<m_CompartmentImages.size(); i++)
                    if ( m_Parameters.m_SignalGen.m_DoSimulateRelaxation)
                        f += std::complex<double>( m_CompartmentImages.at(i)->GetPixel(it.GetIndex()) * relaxFactor.at(i) *  m_Parameters.m_SignalGen.m_SignalScale, 0);
                    else
                        f += std::complex<double>( m_CompartmentImages.at(i)->GetPixel(it.GetIndex()) *  m_Parameters.m_SignalGen.m_SignalScale );

                if (m_Parameters.m_SignalGen.m_CoilSensitivityProfile!=SignalGenerationParameters::COIL_CONSTANT)
                    f *= CoilSensitivity(pos);

                // simulate eddy currents and other distortions
                double omega = 0;   // frequency offset
                if (  m_Parameters.m_SignalGen.m_EddyStrength>0 && !m_IsBaseline)
                {
                    omega += (m_DiffusionGradientDirection[0]*pos[0]+m_DiffusionGradientDirection[1]*pos[1]+m_DiffusionGradientDirection[2]*pos[2]) * eddyDecay;
                }
                if (m_Parameters.m_SignalGen.m_FrequencyMap.IsNotNull()) // simulate distortions
                {
                    itk::Point<double, 3> point3D;
                    ItkDoubleImgType::IndexType index; index[0] = it.GetIndex()[0]; index[1] = it.GetIndex()[1]; index[2] = m_Zidx;
                    if (m_Parameters.m_SignalGen.m_DoAddMotion)
                    {
                        m_Parameters.m_SignalGen.m_FrequencyMap->TransformIndexToPhysicalPoint(index, point3D);
                        point3D = m_FiberBundle->TransformPoint(point3D.GetVnlVector(), -m_Rotation[0],-m_Rotation[1],-m_Rotation[2],-m_Translation[0],-m_Translation[1],-m_Translation[2]);
                        m_Parameters.m_SignalGen.m_FrequencyMap->TransformPhysicalPointToIndex(point3D, index);
                        if (m_Parameters.m_SignalGen.m_FrequencyMap->GetLargestPossibleRegion().IsInside(index))
                            omega += m_Parameters.m_SignalGen.m_FrequencyMap->GetPixel(index);
                    }
                    else
                    {
                        omega += m_Parameters.m_SignalGen.m_FrequencyMap->GetPixel(index);
                    }
                }

                if (y<-yMaxFov/2)
                    y += yMaxFov;
                else if (y>=yMaxFov/2)
                    y -= yMaxFov;

                // actual DFT term
                s += f * exp( std::complex<double>(0, 2 * M_PI * (kx*x/xMax + ky*y/yMaxFov + omega*t/1000 )) );

                ++it;
            }
            s /= numPix;

            if (m_SpikesPerSlice>0 && sqrt(s.imag()*s.imag()+s.real()*s.real()) > sqrt(m_Spike.imag()*m_Spike.imag()+m_Spike.real()*m_Spike.real()) )
                m_Spike = s;

            if (m_Parameters.m_SignalGen.m_NoiseVariance>0)
                s = vcl_complex<double>(s.real()+randGen->GetNormalVariate(0,noiseVar), s.imag()+randGen->GetNormalVariate(0,noiseVar));

            outputImage->SetPixel(kIdx, s);
            m_KSpaceImage->SetPixel(oit.GetIndex(), sqrt(s.imag()*s.imag()+s.real()*s.real()) );
//            m_KSpaceImage->SetPixel(kIdx, sqrt(s.imag()*s.imag()+s.real()*s.real()) );
        }

        ++oit;
    }
}

template< class TPixelType >
void KspaceImageFilter< TPixelType >
::AfterThreadedGenerateData()
{
    typename OutputImageType::Pointer outputImage = static_cast< OutputImageType * >(this->ProcessObject::GetOutput(0));
    double kxMax = outputImage->GetLargestPossibleRegion().GetSize(0);  // k-space size in x-direction
    double kyMax = outputImage->GetLargestPossibleRegion().GetSize(1);  // k-space size in y-direction

    ImageRegionIterator< OutputImageType > oit(outputImage, outputImage->GetLargestPossibleRegion());
    while( !oit.IsAtEnd() ) // use hermitian k-space symmetry to fill empty k-space parts resulting from partial fourier acquisition
    {
        itk::Index< 2 > kIdx;
        kIdx[0] = oit.GetIndex()[0];
        kIdx[1] = oit.GetIndex()[1];

        // reverse phase
        if (!m_Parameters.m_SignalGen.m_ReversePhase)
            kIdx[1] = kyMax-1-kIdx[1];

        if (kIdx[1]>kyMax*m_Parameters.m_SignalGen.m_PartialFourier)
        {
            // reverse readout direction
            if (oit.GetIndex()[1]%2 == 1)
                kIdx[0] = kxMax-kIdx[0]-1;

            // flip k-space
            if( kIdx[0] <  kxMax/2 )
                kIdx[0] = kIdx[0] + kxMax/2;
            else
                kIdx[0] = kIdx[0] - kxMax/2;

            if( kIdx[1] <  kyMax/2 )
                kIdx[1] = kIdx[1] + kyMax/2;
            else
                kIdx[1] = kIdx[1] - kyMax/2;

            itk::Index< 2 > kIdx2;
            kIdx2[0] = (int)(kxMax-kIdx[0])%(int)kxMax;
            kIdx2[1] = (int)(kyMax-kIdx[1])%(int)kyMax;

            vcl_complex<double> s = outputImage->GetPixel(kIdx2);
            s = vcl_complex<double>(s.real(), -s.imag());
            outputImage->SetPixel(kIdx, s);

            m_KSpaceImage->SetPixel(oit.GetIndex(), sqrt(s.imag()*s.imag()+s.real()*s.real()) );
//            m_KSpaceImage->SetPixel(kIdx, sqrt(s.imag()*s.imag()+s.real()*s.real()) );
        }
        ++oit;
    }

    itk::Statistics::MersenneTwisterRandomVariateGenerator::Pointer randGen = itk::Statistics::MersenneTwisterRandomVariateGenerator::New();
    randGen->SetSeed();
    if (m_UseConstantRandSeed)  // always generate the same random numbers?
        randGen->SetSeed(0);
    else
        randGen->SetSeed();

    m_Spike *=  m_Parameters.m_SignalGen.m_SpikeAmplitude;
    itk::Index< 2 > spikeIdx;
    for (unsigned int i=0; i<m_SpikesPerSlice; i++)
    {
        spikeIdx[0] = randGen->GetIntegerVariate()%(int)kxMax;
        spikeIdx[1] = randGen->GetIntegerVariate()%(int)kyMax;
        outputImage->SetPixel(spikeIdx, m_Spike);
    }
}

}
#endif
