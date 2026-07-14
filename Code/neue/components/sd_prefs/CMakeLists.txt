idf_component_register(
    SRCS "src/sd_prefs.c" "src/sd_quiet.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES nvs_flash
)
# sd_quiet.c pure modüldür (host testli). Burada yaşar çünkü prefs değer
# doğrulaması ile buzzer'ın çözümleyicisi AYNI gramer olmak zorunda
# (kod incelemesi: sscanf kopyası ıraksıyordu).
