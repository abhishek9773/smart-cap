idf_component_register(
    SRCS "axp2101.c"
    INCLUDE_DIRS "include"
    REQUIRES board_support i2c_bus log
)
