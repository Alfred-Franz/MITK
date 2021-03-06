mitkFunctionAddExternalProject(NAME Poco        ON  COMPONENTS Foundation Net Util XML Zip)
mitkFunctionAddExternalProject(NAME DCMTK       ON  DOC "EXPERIMENTAL, superbuild only: Use DCMTK in MITK")
mitkFunctionAddExternalProject(NAME OpenIGTLink OFF)

mitkFunctionAddExternalProject(NAME tinyxml     ON  ADVANCED)
mitkFunctionAddExternalProject(NAME GDCM        ON  ADVANCED)
mitkFunctionAddExternalProject(NAME Eigen       ON  ADVANCED DOC "Use the Eigen library")
mitkFunctionAddExternalProject(NAME ANN         ON  ADVANCED DOC "Use Approximate Nearest Neighbor Library")
mitkFunctionAddExternalProject(NAME CppUnit     ON  ADVANCED DOC "Use CppUnit for unit tests")

mitkFunctionAddExternalProject(NAME PCRE        OFF ADVANCED NO_PACKAGE)

mitkFunctionAddExternalProject(NAME HDF5        ON ADVANCED)

if(APPLE)
  mitkFunctionAddExternalProject(NAME OpenMP    OFF)
else()
  mitkFunctionAddExternalProject(NAME OpenMP    ON)
endif()

# -----------------------------------------
# The following external projects must be
# ordered according to their
# inter-dependencies

mitkFunctionAddExternalProject(NAME SWIG      OFF ADVANCED NO_PACKAGE DEPENDS PCRE)
mitkFunctionAddExternalProject(NAME Python    OFF)
mitkFunctionAddExternalProject(NAME OpenCV    OFF)
mitkFunctionAddExternalProject(NAME Vigra     OFF          DEPENDS HDF5)

# These are "hard" dependencies and always set to ON
mitkFunctionAddExternalProject(NAME ITK       ON           NO_CACHE DEPENDS HDF5)
mitkFunctionAddExternalProject(NAME VTK       ON           NO_CACHE)
mitkFunctionAddExternalProject(NAME Boost     ON           NO_CACHE)
mitkFunctionAddExternalProject(NAME ZLIB      OFF          ADVANCED)
mitkFunctionAddExternalProject(NAME cpprestsdk OFF         DEPENDS Boost ZLIB ADVANCED)
mitkFunctionAddExternalProject(NAME ACVD      OFF          DOC "Use Approximated Centroidal Voronoi Diagrams")
mitkFunctionAddExternalProject(NAME CTK       ON           DEPENDS Qt5 DCMTK DOC "Use CTK in MITK")
mitkFunctionAddExternalProject(NAME DCMQI     ON           DEPENDS DCMTK ITK DOC "Use dcmqi in MITK")
mitkFunctionAddExternalProject(NAME MatchPoint  OFF ADVANCED  DEPENDS ITK DOC "Use the MatchPoint translation image registration library")

if(MITK_USE_Qt5)
  mitkFunctionAddExternalProject(NAME Qwt     ON  ADVANCED DEPENDS Qt5)
endif()
