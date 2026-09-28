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
if(WIRE_AUDIT_MODE AND NOT PARTY_SAVE_MODE)
  message(FATAL_ERROR "WIRE_AUDIT_MODE requires PARTY_SAVE_MODE")
endif()
if(WIRE_AUDIT_MODE)
  # A package-only lab silently misses shipping loose overrides (including
  # Animations/960 and two animation IDs absent from Animations.res). Strict
  # wire claims must use the complete effective base-game resource tree.
  set(loose_dirs aibsptrees aigeometries animations buildings chapters
    Cursors Fonts heads Music sounds terrain textures units video)
  set(loose_patterns)
  foreach(dir IN LISTS loose_dirs)
    if(NOT IS_DIRECTORY "${RESOURCE_DIR}/${dir}")
      message(FATAL_ERROR "Missing shipping loose resource directory: ${RESOURCE_DIR}/${dir}")
    endif()
    list(APPEND loose_patterns "${RESOURCE_DIR}/${dir}/*")
  endforeach()
  file(GLOB_RECURSE loose_files LIST_DIRECTORIES false ${loose_patterns})
  list(LENGTH loose_files loose_count)
  if(NOT loose_count EQUAL 2421 OR
     NOT EXISTS "${RESOURCE_DIR}/FaceGenHead.gdp" OR
     NOT EXISTS "${RESOURCE_DIR}/FaceGenHead.mmt")
    message(FATAL_ERROR
      "Incomplete shipping loose resource mirror (${loose_count}/2421 files)")
  endif()
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
  set(audit_dir ".")
  if(PARTY_SAVE_MODE)
    if(NOT DEFINED SAVE_DIR OR "${SAVE_DIR}" STREQUAL "")
      message(FATAL_ERROR "PARTY_SAVE_MODE requires SAVE_DIR")
    endif()
    file(MAKE_DIRECTORY "${SAVE_DIR}")
    set(save_path "${SAVE_DIR}/root-${variant_id}.sav")
    if(WIRE_AUDIT_MODE)
      set(audit_dir "${SAVE_DIR}/wire-audit-${variant_id}")
      file(MAKE_DIRECTORY "${audit_dir}")
      # Auto-load Lua scripts are opened relative to the game's working
      # directory, not the absolute .res directory passed to the probe.
      if(NOT SCRIPT_DIR)
        set(SCRIPT_DIR "${RESOURCE_DIR}/../scripts")
      endif()
      file(MAKE_DIRECTORY "${audit_dir}/scripts")
      foreach(script IN ITEMS Constants.l TriggersManager.l Common.l Hint.l)
        file(COPY "${SCRIPT_DIR}/${script}" DESTINATION "${audit_dir}/scripts")
      endforeach()
      set(ENV{S2_WORLD_WIRE_AUDIT} "1")
    endif()
    if(TURN_MODE)
      set(probe_mode --mission-root-party-save-turn)
    else()
      set(probe_mode --mission-root-party-save)
    endif()
  elseif(PARTY_MODE)
    set(probe_mode --mission-root-party)
  else()
    set(probe_mode --mission)
  endif()
  execute_process(
    COMMAND ${TEST_EMULATOR} "${WORLD_PROBE}" "${GAME_DB}" "${RESOURCE_DIR}"
      ${probe_mode} "${variant_id}" ${save_path}
    WORKING_DIRECTORY "${audit_dir}"
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
    if(TURN_MODE AND NOT world_output MATCHES
        "root party second save after end turn restored and advanced:")
      message(FATAL_ERROR "Root ${variant_id} did not verify post-turn save")
    endif()
    if(WIRE_AUDIT_MODE)
      set(audit_path "${audit_dir}/_wireaudit.log")
      if(NOT EXISTS "${audit_path}")
        message(FATAL_ERROR "Root ${variant_id} produced no wire audit")
      endif()
      file(STRINGS "${audit_path}" raw_rows REGEX "^RAW-TYPE ")
      file(STRINGS "${audit_path}" raw_done REGEX "^RAW-TYPE-AUDIT-DONE")
      file(STRINGS "${audit_path}" wire_done REGEX "^WIRE-AUDIT-DONE")
      file(STRINGS "${audit_path}" wire_findings
        REGEX "^0x[0-9A-F]+ (SIZE|UNREAD|MISS)")
      if(WIRE_AUDIT_REPORT_ONLY)
        list(LENGTH raw_rows raw_row_count)
        message(STATUS "Scenario root ${variant_id}: ${raw_row_count} raw wire paths (${audit_path})")
      else()
        if(raw_rows OR NOT raw_done OR NOT wire_done)
          message(FATAL_ERROR "Root ${variant_id} has incomplete or raw wire audit: ${audit_path}")
        endif()
        foreach(row IN LISTS raw_done)
          if(NOT row STREQUAL "RAW-TYPE-AUDIT-DONE (0 distinct types/paths)")
            message(FATAL_ERROR "Root ${variant_id} retained raw types: ${audit_path}: ${row}")
          endif()
        endforeach()
        foreach(row IN LISTS wire_findings)
          # COcTreeNode reads the optional tag-1 v1.0 base coordinate, absent
          # from its v1.2 writer; tag 21 supplies the bound instead.
          if(NOT row MATCHES "^0x01071140 MISS +tag 1[.]1 +save=-1 dev=12 x[0-9]+$")
            message(FATAL_ERROR "Root ${variant_id} has unexpected wire divergence: ${audit_path}: ${row}")
          endif()
        endforeach()
        message(STATUS "Scenario root ${variant_id}: zero raw types and no unexpected wire divergences")
      endif()
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
