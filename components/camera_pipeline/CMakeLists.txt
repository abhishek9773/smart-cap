idf_component_register(
    SRCS "camera_pipeline.c"
    INCLUDE_DIRS "include"
    REQUIRES camera_driver image_preprocessor
)
