ez_cmake_init()

ez_build_filter_foundation()

ez_requires(EZ_3RDPARTY_ZLIB_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)
else()
  ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)
endif()


target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZLIB_SUPPORT)


if(MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)

  if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      OneCore.lib
    )

  elseif(EZ_COMPILE_ENGINE_AS_DLL)

    target_compile_definitions(${PROJECT_NAME} PRIVATE ZLIB_DLL_EXPORT=1)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ZLIB_DLL_IMPORT=1)

  endif()

endif()



