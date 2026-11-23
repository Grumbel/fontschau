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

# All accepted syntaxes can carry two files, but a third must be rejected
# before the program attempts to open any file.
execute_process(COMMAND "${APP}" first.ttf second.ttf third.ttf
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 2 OR NOT errors MATCHES "at most two font files")
    message(FATAL_ERROR "Excess positional input handling failed (${status}): ${output} ${errors}")
endif()

execute_process(COMMAND "${APP}" -f first.ttf --font second.ttf third.ttf
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 2 OR NOT errors MATCHES "at most two font files")
    message(FATAL_ERROR "Excess mixed input handling failed (${status}): ${output} ${errors}")
endif()

# A pair of paths passes argument validation and reaches font-file loading.
# The first path should fail as an unreadable file (exit 1), not as an invalid
# combination of options (exit 2).
foreach(args IN ITEMS "first.ttf;second.ttf" "--font;first.ttf;--font;second.ttf" "-f;first.ttf;-f;second.ttf" "--font;first.ttf;second.ttf")
    execute_process(COMMAND "${APP}" ${args}
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT status EQUAL 1 OR NOT errors MATCHES "Cannot read font file: first.ttf")
        message(FATAL_ERROR "Two-file CLI syntax failed (${status}): ${args} ${output} ${errors}")
    endif()
endforeach()

execute_process(COMMAND "${APP}" --renderer invalid
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 2 OR NOT errors MATCHES "--renderer must be qt or freetype")
    message(FATAL_ERROR "Invalid renderer handling failed (${status}): ${output} ${errors}")
endif()
