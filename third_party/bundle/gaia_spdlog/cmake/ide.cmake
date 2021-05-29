# ---------------------------------------------------------------------------------------
# IDE support for headers
# ---------------------------------------------------------------------------------------
set(GAIA_SPDLOG_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/../include")

file(GLOB GAIA_SPDLOG_TOP_HEADERS "${GAIA_SPDLOG_HEADERS_DIR}/spdlog/*.h")
file(GLOB GAIA_SPDLOG_DETAILS_HEADERS "${GAIA_SPDLOG_HEADERS_DIR}/spdlog/details/*.h")
file(GLOB GAIA_SPDLOG_SINKS_HEADERS "${GAIA_SPDLOG_HEADERS_DIR}/spdlog/sinks/*.h")
file(GLOB GAIA_SPDLOG_FMT_HEADERS "${GAIA_SPDLOG_HEADERS_DIR}/spdlog/fmt/*.h")
file(GLOB GAIA_SPDLOG_FMT_BUNDELED_HEADERS "${GAIA_SPDLOG_HEADERS_DIR}/spdlog/fmt/bundled/*.h")
set(GAIA_SPDLOG_ALL_HEADERS ${GAIA_SPDLOG_TOP_HEADERS} ${GAIA_SPDLOG_DETAILS_HEADERS} ${GAIA_SPDLOG_SINKS_HEADERS} ${GAIA_SPDLOG_FMT_HEADERS}
                       ${GAIA_SPDLOG_FMT_BUNDELED_HEADERS})

source_group("Header Files\\spdlog" FILES ${GAIA_SPDLOG_TOP_HEADERS})
source_group("Header Files\\spdlog\\details" FILES ${GAIA_SPDLOG_DETAILS_HEADERS})
source_group("Header Files\\spdlog\\sinks" FILES ${GAIA_SPDLOG_SINKS_HEADERS})
source_group("Header Files\\spdlog\\fmt" FILES ${GAIA_SPDLOG_FMT_HEADERS})
source_group("Header Files\\spdlog\\fmt\\bundled\\" FILES ${GAIA_SPDLOG_FMT_BUNDELED_HEADERS})
