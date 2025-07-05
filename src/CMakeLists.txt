# Set the template VOL connector name
set (TVC_NAME template_vol_connector)

# Build the template VOL
add_library (${TVC_NAME} SHARED template_vol_connector.c)
set_target_properties (${TVC_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${TVC_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${TVC_NAME} PROPERTIES PUBLIC_HEADER "${TVC_NAME}.h")

