function(s2_build_network_identity target)
  if(NOT S2_GAME_DIR)
    message(STATUS "Set S2_GAME_DIR to generate the network compatibility manifest")
    return()
  endif()
  find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
  add_custom_target(S2NetworkIdentity
    COMMAND "${Python3_EXECUTABLE}" "${root}/diagnostics/Generate-NetworkIdentity.py"
      --game-data "${S2_GAME_DIR}" --build-id "${s2_network_build_id}"
      --output "$<TARGET_FILE_DIR:${target}>/network-version.txt" --incremental
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "$<TARGET_FILE_DIR:${target}>/network-version.txt" "${S2_GAME_DIR}/network-version.txt"
    COMMENT "Generating network compatibility manifest"
    VERBATIM)
  add_dependencies(${target} S2NetworkIdentity)
endfunction()
