cmake_minimum_required(VERSION 3.16)

# ESP-IDF `components/` klasörünü otomatik keşfeder — EXTRA_COMPONENT_DIRS yok.
# sk_core + sk_api vendor copy'dir (bkz. components/VENDORED.md).
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(syndimm)
