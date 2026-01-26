set(CUSTOM_WIDGET_SOURCE "${CMAKE_CURRENT_LIST_DIR}/widgets/custom_status.c")

if(EXISTS ${CUSTOM_WIDGET_SOURCE})
    target_sources(app PRIVATE ${CUSTOM_WIDGET_SOURCE})
else()
    message(FATAL_ERROR "CUSTOM UI ERROR: File not found at ${CUSTOM_WIDGET_SOURCE}. Check folder name casing and git commit.")
endif()