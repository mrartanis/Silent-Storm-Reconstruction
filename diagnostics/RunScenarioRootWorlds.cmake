foreach(required DB_TEST WORLD_PROBE GAME_DB RESOURCE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()

execute_process(
  COMMAND ${TEST_EMULATOR} "${DB_TEST}" "${GAME_DB}" --roots
  RESULT_VARIABLE roots_result
  OUTPUT_VARIABLE roots_output
  ERROR_VARIABLE roots_error
  TIMEOUT 600)
if(NOT "${roots_result}" STREQUAL "0")
  message(FATAL_ERROR "Scenario root discovery failed (${roots_result}): ${roots_error}")
endif()

string(REGEX MATCHALL "active_root_variant=[0-9]+" roots "${roots_output}")
list(LENGTH roots root_count)
if(NOT root_count EQUAL 52)
  message(FATAL_ERROR "Expected 52 active scenario root variants, found ${root_count}")
endif()

set(tested_count 0)
foreach(root IN LISTS roots)
  string(REPLACE "active_root_variant=" "" variant_id "${root}")
  if(DEFINED START_VARIANT AND variant_id LESS START_VARIANT)
    continue()
  endif()
  if(DEFINED END_VARIANT AND variant_id GREATER END_VARIANT)
    continue()
  endif()
  if(PARTY_SAVE_MODE)
    if(NOT DEFINED SAVE_DIR OR "${SAVE_DIR}" STREQUAL "")
      message(FATAL_ERROR "PARTY_SAVE_MODE requires SAVE_DIR")
    endif()
    file(MAKE_DIRECTORY "${SAVE_DIR}")
    set(save_path "${SAVE_DIR}/root-${variant_id}.sav")
    set(probe_mode --mission-root-party-save)
  elseif(PARTY_MODE)
    set(probe_mode --mission-root-party)
  else()
    set(probe_mode --mission)
  endif()
  execute_process(
    COMMAND ${TEST_EMULATOR} "${WORLD_PROBE}" "${GAME_DB}" "${RESOURCE_DIR}"
      ${probe_mode} "${variant_id}" ${save_path}
    RESULT_VARIABLE world_result
    OUTPUT_VARIABLE world_output
    ERROR_VARIABLE world_error
    TIMEOUT 600)
  if(NOT "${world_result}" STREQUAL "0" OR
     NOT world_output MATCHES "world mission variant ${variant_id} post-init completed")
    message(FATAL_ERROR
      "World startup failed for scenario root ${variant_id} (${world_result}):\n${world_output}\n${world_error}")
  endif()
  if(PARTY_SAVE_MODE)
    if(NOT world_output MATCHES "root party save restored and advanced:")
      message(FATAL_ERROR "Root ${variant_id} did not confirm save round-trip")
    endif()
    if(NOT world_output MATCHES "root party deployment spots restored: [0-9]+")
      message(FATAL_ERROR "Root ${variant_id} did not verify deployment spots")
    endif()
    file(REMOVE "${save_path}")
    message(STATUS "Scenario root ${variant_id}: party save round-trip and resumed ticks completed")
  elseif(PARTY_MODE)
    message(STATUS "Scenario root ${variant_id}: world post-init and party ticks completed")
  else()
    message(STATUS "Scenario root ${variant_id}: world post-init completed")
  endif()
  math(EXPR tested_count "${tested_count} + 1")
endforeach()

message(STATUS "Initialized ${tested_count} of ${root_count} active scenario root worlds")
