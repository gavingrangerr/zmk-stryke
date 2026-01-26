if (CONFIG_SHIELD_STRYKE)
    target_sources(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/widgets/custom_status.c
    )
endif()