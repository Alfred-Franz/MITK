#-----------------------------------------------------------------------------
# CTK
#-----------------------------------------------------------------------------

if(MITK_USE_CTK)

  # Sanity checks
  if(DEFINED CTK_DIR AND NOT EXISTS ${CTK_DIR})
    message(FATAL_ERROR "CTK_DIR variable is defined but corresponds to non-existing directory")
  endif()

  set(proj CTK)
  set(proj_DEPENDENCIES DCMTK)
  set(CTK_DEPENDS ${proj})

  if(NOT DEFINED CTK_DIR)

    set(revision_tag da3fd76)

    set(ctk_optional_cache_args )
    if(MITK_USE_Python)
      list(APPEND ctk_optional_cache_args
           -DCTK_LIB_Scripting/Python/Widgets:BOOL=ON
           -DCTK_ENABLE_Python_Wrapping:BOOL=OFF
           -DCTK_APP_ctkSimplePythonShell:BOOL=OFF
           "-DPYTHON_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}"
           "-DPYTHON_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR}"
           "-DPYTHON_LIBRARY:FILEPATH=${Python3_LIBRARY}"
      )
    else()
      list(APPEND ctk_optional_cache_args
           -DCTK_LIB_Scripting/Python/Widgets:BOOL=OFF
           -DCTK_ENABLE_Python_Wrapping:BOOL=OFF
           -DCTK_APP_ctkSimplePythonShell:BOOL=OFF
      )
    endif()

    if(NOT MITK_USE_Python)
      list(APPEND ctk_optional_cache_args
        -DDCMTK_CMAKE_DEBUG_POSTFIX:STRING=d
      )
    endif()

    if(CTEST_USE_LAUNCHERS)
      list(APPEND ctk_optional_cache_args
        "-DCMAKE_PROJECT_${proj}_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake"
      )
    endif()

    FOREACH(type RUNTIME ARCHIVE LIBRARY)
      IF(DEFINED CTK_PLUGIN_${type}_OUTPUT_DIRECTORY)
        LIST(APPEND mitk_optional_cache_args -DCTK_PLUGIN_${type}_OUTPUT_DIRECTORY:PATH=${CTK_PLUGIN_${type}_OUTPUT_DIRECTORY})
      ENDIF()
    ENDFOREACH()

    mitk_query_custom_ep_vars()

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL ${MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL}/CTK_${revision_tag}.tar.gz
      URL_MD5 A413BE11D3E9970E0EBB71181A5F2B90
      UPDATE_COMMAND ""
      INSTALL_COMMAND ""
      CMAKE_GENERATOR ${gen}
      CMAKE_GENERATOR_PLATFORM ${gen_platform}
      CMAKE_ARGS
        ${ep_common_args}
        ${ctk_optional_cache_args}
        # The CTK PluginFramework cannot cope with
        # a non-empty CMAKE_DEBUG_POSTFIX for the plugin
        # libraries yet.
        -DCMAKE_DEBUG_POSTFIX:STRING=
        -DCTK_QT_VERSION:STRING=5
        -DQt5_DIR=${Qt5_DIR}
        -DGit_EXECUTABLE:FILEPATH=${GIT_EXECUTABLE}
        -DGIT_EXECUTABLE:FILEPATH=${GIT_EXECUTABLE}
        -DCTK_BUILD_QTDESIGNER_PLUGINS:BOOL=OFF
        -DCTK_LIB_CommandLineModules/Backend/LocalProcess:BOOL=ON
        -DCTK_LIB_CommandLineModules/Frontend/QtGui:BOOL=ON
        -DCTK_LIB_PluginFramework:BOOL=ON
        -DCTK_LIB_DICOM/Widgets:BOOL=ON
        -DCTK_LIB_XNAT/Core:BOOL=ON
        -DCTK_PLUGIN_org.commontk.eventadmin:BOOL=ON
        -DCTK_PLUGIN_org.commontk.configadmin:BOOL=ON
        -DCTK_USE_GIT_PROTOCOL:BOOL=OFF
        -DDCMTK_DIR:PATH=${DCMTK_DIR}
        -DPythonQt_URL:STRING=${MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL}/PythonQt_fae23012.tar.gz
        ${${proj}_CUSTOM_CMAKE_ARGS}
      CMAKE_CACHE_ARGS
        ${ep_common_cache_args}
        ${${proj}_CUSTOM_CMAKE_CACHE_ARGS}
      CMAKE_CACHE_DEFAULT_ARGS
        ${ep_common_cache_default_args}
        ${${proj}_CUSTOM_CMAKE_CACHE_DEFAULT_ARGS}
      DEPENDS ${proj_DEPENDENCIES}
     )

    ExternalProject_Get_Property(${proj} binary_dir)
    set(CTK_DIR ${binary_dir})

  else()

    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

  endif()

endif()
