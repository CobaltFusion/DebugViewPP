project(pcre2 C)

# PCRE2 10.49 (https://github.com/PCRE2Project/PCRE2), BSD license, see LICENCE.md. The regular expressions of the filters.
# Vendored as it is, built the way its NON-AUTOTOOLS-BUILD describes: the 8-bit library with the JIT, config.h, pcre2.h and
# pcre2_chartables.c are the .generic/.dist files of the release under their final names. No Unicode (UTF) support: the messages are bytes.
# It is compiled without the warning options of the project.
add_library(${PROJECT_NAME} STATIC
    src/pcre2_auto_possess.c
    src/pcre2_chartables.c
    src/pcre2_chkdint.c
    src/pcre2_compile.c
    src/pcre2_compile_cgroup.c
    src/pcre2_compile_class.c
    src/pcre2_config.c
    src/pcre2_context.c
    src/pcre2_convert.c
    src/pcre2_dfa_match.c
    src/pcre2_error.c
    src/pcre2_extuni.c
    src/pcre2_find_bracket.c
    src/pcre2_jit_compile.c
    src/pcre2_maketables.c
    src/pcre2_match.c
    src/pcre2_match_data.c
    src/pcre2_match_next.c
    src/pcre2_newline.c
    src/pcre2_ord2utf.c
    src/pcre2_pattern_info.c
    src/pcre2_script_run.c
    src/pcre2_serialize.c
    src/pcre2_string_utils.c
    src/pcre2_study.c
    src/pcre2_substitute.c
    src/pcre2_substring.c
    src/pcre2_tables.c
    src/pcre2_ucd.c
    src/pcre2_valid_utf.c
    src/pcre2_xclass.c
)
add_library(dv::pcre2 ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME}
PRIVATE
    HAVE_CONFIG_H
    SUPPORT_JIT
    _CRT_SECURE_NO_WARNINGS
PUBLIC
    PCRE2_STATIC
    PCRE2_CODE_UNIT_WIDTH=8
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
