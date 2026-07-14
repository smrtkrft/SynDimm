idf_component_register(
    SRCS "src/sd_feedback.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES sd_buzzer
)
