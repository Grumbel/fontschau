# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

execute_process(COMMAND "${APP}" --help
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 0 OR NOT output MATCHES "--font" OR
   NOT output MATCHES "--version" OR NOT output MATCHES "font-file")
    message(FATAL_ERROR "--help failed (${status}): ${output} ${errors}")
endif()

execute_process(COMMAND "${APP}" --version
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT status EQUAL 0 OR NOT output STREQUAL "fontschau ${VERSION}")
    message(FATAL_ERROR "--version failed (${status}): '${output}' (expected 'fontschau ${VERSION}') ${errors}")
endif()

# Run under the Qt offscreen platform plugin to avoid requiring a display.
set(ENV{QT_QPA_PLATFORM} offscreen)
execute_process(COMMAND "${APP}" --font "${INVALID_FONT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(status EQUAL 0 OR NOT errors MATCHES "Unsupported or invalid font file")
    message(FATAL_ERROR "Invalid font handling failed (${status}): ${output} ${errors}")
endif()

execute_process(COMMAND "${APP}" --font a.ttf b.ttf
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 2 OR NOT errors MATCHES "at most one font file")
    message(FATAL_ERROR "Conflicting input handling failed (${status}): ${output} ${errors}")
endif()

execute_process(COMMAND "${APP}" --renderer invalid
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 2 OR NOT errors MATCHES "--renderer must be qt or freetype")
    message(FATAL_ERROR "Invalid renderer handling failed (${status}): ${output} ${errors}")
endif()
