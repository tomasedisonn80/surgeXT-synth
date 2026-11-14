# SPDX-License-Identifier: GPL-3.0-or-later
# The bank is installed by the existing recursive factory-data packaging rules.
# Check at configure and again for builds/packages, so data edits after configure
# cannot silently produce an incomplete factory bank.
# SURGE_SOURCE_DIR follows this project when it is embedded with add_subdirectory;
# CMAKE_SOURCE_DIR would instead name the unrelated outer project.
if(NOT DEFINED SURGE_SOURCE_DIR)
  get_filename_component(SURGE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-scoring-bank.cmake"
  RESULT_VARIABLE _scoring_bank_result)
if(NOT _scoring_bank_result EQUAL 0)
  message(FATAL_ERROR "Scoring 1000 factory bank verification failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-embedded-factory.cmake"
  RESULT_VARIABLE _embedded_factory_result)
if(NOT _embedded_factory_result EQUAL 0)
  message(FATAL_ERROR "Embedded factory content verification failed")
endif()

add_custom_target(check-scoring-bank ALL
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-scoring-bank.cmake"
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-embedded-factory.cmake"
  COMMENT "Verifying the complete Scoring 1000 factory bank"
  VERBATIM)

# A small data-only staging target lets CI inspect exactly the same relative
# factory layout without building a platform installer or copying other assets.
set(SURGE_SCORING_STAGE_DATA "${CMAKE_CURRENT_BINARY_DIR}/scoring-bank-stage/share/surge-xt")
add_custom_target(stage-scoring-bank
  COMMAND "${CMAKE_COMMAND}" -E rm -rf
    "${SURGE_SCORING_STAGE_DATA}/patches_factory/Scoring 1000"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${SURGE_SCORING_STAGE_DATA}/patches_factory"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${SURGE_SOURCE_DIR}/resources/data/patches_factory/Scoring 1000"
    "${SURGE_SCORING_STAGE_DATA}/patches_factory/Scoring 1000"
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    "-DSURGE_SCORING_DATA_DIR=${SURGE_SCORING_STAGE_DATA}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-scoring-bank.cmake"
  DEPENDS check-scoring-bank
  COMMENT "Staging and verifying all 1000 factory presets"
  VERBATIM)

file(GLOB _scoring_reference_skins "${SURGE_SOURCE_DIR}/resources/data/skins/reference-*.surge-skin")
foreach(_skin IN LISTS _scoring_reference_skins)
  get_filename_component(_skin_name "${_skin}" NAME)
  add_custom_command(TARGET stage-scoring-bank POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_skin}"
      "${SURGE_SCORING_STAGE_DATA}/skins/${_skin_name}"
    VERBATIM)
endforeach()
add_custom_command(TARGET stage-scoring-bank POST_BUILD
  COMMAND "${CMAKE_COMMAND}" "-DSURGE_SCORING_SOURCE_DIR=${SURGE_SOURCE_DIR}"
    "-DSURGE_EMBEDDED_DATA_DIR=${SURGE_SCORING_STAGE_DATA}"
    -P "${SURGE_SOURCE_DIR}/cmake/verify-embedded-factory.cmake"
  VERBATIM)

# cmake --install does not build targets; check source integrity there as well.
install(SCRIPT "${SURGE_SOURCE_DIR}/cmake/verify-scoring-bank.cmake")
install(SCRIPT "${SURGE_SOURCE_DIR}/cmake/verify-embedded-factory.cmake")
