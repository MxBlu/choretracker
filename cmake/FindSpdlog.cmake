find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h HINTS ${SPDLOG_ROOT_DIR})
find_library(SPDLOG_LIBRARY NAMES spdlog "libspdlog.a" HINTS ${SPDLOG_ROOT_DIR}) 
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog DEFAULT_MSG SPDLOG_LIBRARY SPDLOG_INCLUDE_DIR)