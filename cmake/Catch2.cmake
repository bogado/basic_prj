set(VB_FETCH_CATCH2_V3 OFF CACHE BOOL "Uses FetchContent to get the required version of catch2")

if(VB_FETCH_CATCH2_V3)
    Include(FetchContent)
    
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.4.0 # or a later release
      OVERRIDE_FIND_PACKAGE
    )

endif()
