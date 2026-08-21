@echo off
echo Building Melody...
if not exist build\obj mkdir build\obj

echo Compiling sqlite3...
gcc -std=c11 -O2 -w -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION ^
    -c -o build\obj\sqlite3.o src/stdlib/sqlite3.c
if %ERRORLEVEL% NEQ 0 goto :fail

echo Compiling Melody...
gcc -Wall -Wextra -std=c11 -O2 -Wno-unused-parameter ^
    -Isrc/core -Isrc/frontend -Isrc/runtime -Isrc/stdlib -Isrc/pm -Isrc/test_runner -Isrc/formatter ^
    -o build\melody.exe ^
    src/core/value.c src/core/env.c ^
    src/frontend/lexer.c src/frontend/parser.c ^
    src/runtime/interpreter.c src/runtime/repl.c ^
    src/stdlib/net.c src/stdlib/os_module.c ^
    src/stdlib/time_module.c src/stdlib/path_module.c ^
    src/stdlib/http_module.c src/stdlib/sqlite_module.c ^
    src/stdlib/cli_module.c ^
    src/stdlib/regex_engine.c src/stdlib/regex_module.c ^
    src/stdlib/crypto_module.c src/stdlib/uuid_module.c ^
    src/stdlib/url_module.c src/stdlib/log_module.c ^
    src/stdlib/env_module.c ^
    src/test_runner\test_runner.c ^
    src/formatter\formatter.c ^
    src/pm/pm.c ^
    src/main.c build\obj\sqlite3.o -lm -lws2_32
if %ERRORLEVEL% == 0 (
    echo Build successful: build\melody.exe
) else (
    goto :fail
)
goto :eof

:fail
echo Build failed!
