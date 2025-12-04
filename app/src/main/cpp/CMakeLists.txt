cmake_minimum_required(VERSION 3.4.1)

# Define onde estão os fontes do core e do android
file(GLOB CORE_SRC "core/*.cpp")
file(GLOB ANDROID_SRC "android/*.cpp")

# --- CORREÇÃO: Adiciona a pasta 'include' ao caminho de busca ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(nes-emu SHARED ${CORE_SRC} ${ANDROID_SRC})

find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(nes-emu ${log-lib} ${android-lib} ${jnigraphics-lib})
