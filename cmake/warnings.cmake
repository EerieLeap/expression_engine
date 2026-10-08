# expression_engine_apply_warnings(<target>) adds the warning set as PRIVATE compile options, so
# consumers of the library never inherit it and nothing extra ends up in the export set.

set(EXPRESSION_ENGINE_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wold-style-cast
    -Wnon-virtual-dtor
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
)

function(expression_engine_apply_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:${EXPRESSION_ENGINE_WARNING_FLAGS}>
        $<$<AND:$<CXX_COMPILER_ID:GNU,Clang,AppleClang>,$<BOOL:${EXPRESSION_ENGINE_WERROR}>>:-Werror>
    )
endfunction()
