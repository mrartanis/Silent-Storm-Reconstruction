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
  execute_process(
    COMMAND ${TEST_EMULATOR} "${WORLD_PROBE}" "${GAME_DB}" "${RESOURCE_DIR}"
      --mission "${variant_id}"
    RESULT_VARIABLE world_result
    OUTPUT_VARIABLE world_output
    ERROR_VARIABLE world_error
    TIMEOUT 600)
  if(NOT "${world_result}" STREQUAL "0" OR
     NOT world_output MATCHES "world mission variant ${variant_id} post-init completed")
    message(FATAL_ERROR
      "World startup failed for scenario root ${variant_id} (${world_result}):\n${world_output}\n${world_error}")
  endif()
  message(STATUS "Scenario root ${variant_id}: world post-init completed")
  math(EXPR tested_count "${tested_count} + 1")
endforeach()

message(STATUS "Initialized ${tested_count} of ${root_count} active scenario root worlds")
