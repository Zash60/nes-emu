cmake_minimum_required(VERSION 3.4.1)

# Define onde estão os fontes
file(GLOB CORE_SRC "core/*.cpp")
file(GLOB ANDROID_SRC "android/*.cpp")

# Adiciona a pasta 'include' (onde criamos os fakes) ao path do compilador
include_directories(include)

add_library(nes-emu SHARED ${CORE_SRC} ${ANDROID_SRC})

find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(nes-emu ${log-lib} ${android-lib} ${jnigraphics-lib})
