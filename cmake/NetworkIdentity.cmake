function(s2_build_network_identity target)
  # Compatibility is versioned explicitly, independently of commit/OS/game data.
  add_custom_target(S2NetworkIdentity
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${root}/network-version.txt" "$<TARGET_FILE_DIR:${target}>/network-version.txt"
    DEPENDS "${root}/network-version.txt"
    COMMENT "Copying the network compatibility version"
    VERBATIM)
  add_dependencies(${target} S2NetworkIdentity)
endfunction()
