# vi:set sw=2 et:
project(surge-xt-tests)

add_executable(${PROJECT_NAME}
        main.cpp
        XTTestLifecycle.cpp
        XTTestOSC.cpp
        XTTestReferenceWorkspaces.cpp
        XTTestReferenceWavetables.cpp
        XTTestReferenceManifest.cpp
        XTTestReferencePreviews.cpp
  )

# Absolute source lookup keeps native skin tests runnable from any build directory.
# Revision metadata describes configure-time source, including whether it was dirty;
# it must not be mistaken for a post-capture release-commit attestation.
execute_process(COMMAND git rev-parse HEAD
  WORKING_DIRECTORY "${SURGE_SOURCE_DIR}"
  OUTPUT_VARIABLE SURGE_REFERENCE_SOURCE_REVISION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
execute_process(COMMAND git status --porcelain --untracked-files=normal
  WORKING_DIRECTORY "${SURGE_SOURCE_DIR}"
  OUTPUT_VARIABLE SURGE_REFERENCE_SOURCE_STATUS
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
if(SURGE_REFERENCE_SOURCE_STATUS STREQUAL "")
  set(SURGE_REFERENCE_SOURCE_DIRTY 0)
else()
  set(SURGE_REFERENCE_SOURCE_DIRTY 1)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
  SURGE_REFERENCE_SOURCE_DIR="${SURGE_SOURCE_DIR}"
  SURGE_REFERENCE_SOURCE_REVISION="${SURGE_REFERENCE_SOURCE_REVISION}"
  SURGE_REFERENCE_SOURCE_DIRTY=${SURGE_REFERENCE_SOURCE_DIRTY}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  surge-xt
  surge::catch2_v3

  # These are PRIVATE linked by the plugin for appropriate reason
  # but means we need to copy them here
        surge-common
        surge-platform
        surge-juce
        juce::juce_graphics
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_osc
        surge-xt-binary
        sst-filters-extras
)

if (${SURGE_INCLUDE_XT_TESTS_IN_CTEST})
    message(STATUS "Using CatchDiscoverTests on ${PROJECT_NAME}" )
    catch_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${SURGE_SOURCE_DIR})
endif()
