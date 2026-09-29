foreach(required DB_TEST MAP_PROBE GAME_DB RESOURCE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()

set(db_command ${TEST_EMULATOR} "${DB_TEST}" "${GAME_DB}" --roots)
execute_process(
  COMMAND ${db_command}
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

set(all_summaries "")
foreach(root IN LISTS roots)
  string(REPLACE "active_root_variant=" "" variant_id "${root}")
  set(map_command ${TEST_EMULATOR} "${MAP_PROBE}" "${GAME_DB}" "${RESOURCE_DIR}"
    "${variant_id}" --deterministic)
  execute_process(
    COMMAND ${map_command}
    RESULT_VARIABLE map_result
    OUTPUT_VARIABLE map_output
    ERROR_VARIABLE map_error
    TIMEOUT 600)
  if(NOT "${map_result}" STREQUAL "0" OR
     NOT map_output MATCHES "(^|\n)built=1 ")
    message(FATAL_ERROR
      "BuildMap failed for scenario root ${variant_id} (${map_result}):\n${map_output}\n${map_error}")
  endif()
  string(REGEX MATCH "built=1 [^\r\n]*" map_summary "${map_output}")
  string(REGEX MATCH "routes [^\r\n]*" route_summary "${map_output}")
  string(REGEX MATCH "behavior_digest=[0-9A-F]+" behavior_summary "${map_output}")
  if(map_summary STREQUAL "" OR route_summary STREQUAL "" OR
     behavior_summary STREQUAL "")
    message(FATAL_ERROR "Incomplete BuildMap output for scenario root ${variant_id}:\n${map_output}")
  endif()
  set(root_summary
    "${variant_id} ${map_summary}\n${route_summary}\n${behavior_summary}\n")
  string(APPEND all_summaries "${root_summary}")
  string(SHA256 root_digest "${root_summary}")
  message(STATUS "Scenario root ${variant_id}: built digest=${root_digest}")
endforeach()

string(SHA256 scenario_root_digest "${all_summaries}")
message(STATUS "Built all ${root_count} active scenario root variants; digest=${scenario_root_digest}")
set(expected_digest "c79a9c158d3f64204b1529ae576f4b74e59388dd7f9531ad725f2994dcf3894d")
if(NOT scenario_root_digest STREQUAL expected_digest)
  message(FATAL_ERROR "Scenario-root map digest differs from Windows/Linux x64 baseline: ${scenario_root_digest}")
endif()
