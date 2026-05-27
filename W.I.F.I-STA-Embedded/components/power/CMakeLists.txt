idf_component_register(
    SRCS "power.c"
    INCLUDE_DIRS "."
    REQUIRES freertos driver esp_hw_support log
)