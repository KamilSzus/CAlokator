#
# DANTE :: System Automatycznych Testów Jednostkowych oraz Akceptacyjnych
# Tomasz Jaworski, 2018-2020
#
# Plik wygenerowany automatycznie
# Znacznik czasowy: 2022-12-02 18:06:21.757302
#

cmake_minimum_required(VERSION 3.16)
project(project1 C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)

# Ustaw opcje kompilatora (z raportu Dante)
add_compile_options(
      "-fdiagnostics-color"
      "-std=c11"
      "-Werror"
      "-Wextra"
      "-ggdb3"
      "-xc"
      "-fmax-errors=5"
      "-DINSIDE_DANTE"
      "-pedantic"
      "-Wno-error=parentheses"
      "-Wno-error=implicit-fallthrough"
      "-Wno-parentheses"
      "-Werror=vla"
      "-D_ANSI_OUTPUT"
      "-D_GNU_SOURCE"
      "-Wno-error=unused-parameter"
      "-Wall"

)

# Ustaw opcje konsolidatora (z raportu Dante)
add_link_options(
        "-Wl,-Map=main.map"
        "-Wl,-cref"
        "-Wl,-wrap,main"
        "-ggdb3"
)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(project1
        "heap.c"
        "main.c"
        "unit_helper_v2.c"
        "unit_test_v2.c"
        "rdebug.c"
        "memmanager.c"
)

# Dołącz niezbędne biblioteki
target_link_libraries(project1
        "pthread"
        "m"
)