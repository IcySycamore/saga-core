cmake_minimum_required(VERSION 3.21)
project(TRPG LANGUAGES CXX)

# ===== CMake 策略兼容 =====

cmake_policy(SET CMP0167 NEW)  # 使用 Boost 自带的 CMake Config（FindBoost 已移除）

# ===== 使用 MSYS2 clang64 工具链 (Ninja) =====
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Boost_ROOT C:/msys64/clang64 CACHE PATH "Boost root")

# ===== 查找 Boost =====
find_package(Boost REQUIRED COMPONENTS json)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
endif()

# ===== 核心库（头文件为主，少量 .cpp） =====
set(CORE_SOURCES
    core/Item/ItemManager.cpp
    core/Item/Inventory.cpp
)

set(CORE_HEADERS
    core/Dice/Dice.h
    core/Dice/Dice20.h
    core/Entity/Entity.h
    core/Entity/Player.h
    core/Entity/GM.h
    core/Entity/Monster.h
    core/Event/Event.h
    core/Event/Check.h
    core/Event/CreatPlayer.h
    core/Event/Gameinit.h
    core/Event/LoadScene.h
    core/Event/Move.h
    core/Event/Plot.h
    core/Event/Time.h
    core/helper/RandomEng.h
    core/helper/UuidGen.h
    core/Item/ItemManager.h
    core/Item/ItemSlot.h
    core/Item/ItemType.h
)

add_library(trpg_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(trpg_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(trpg_core PUBLIC Boost::json)

# ===== 测试目标 =====
enable_testing()

add_executable(test_item_manager
    test/test_item_manager.cpp
)

target_link_libraries(test_item_manager PRIVATE trpg_core)
target_include_directories(test_item_manager PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(test_item_type
    test/test_item_type.cpp
)

target_include_directories(test_item_type PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_item_type PRIVATE Boost::json)

add_executable(test_item_slot_integration
    test/test_item_slot_integration.cpp
)

target_include_directories(test_item_slot_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_item_slot_integration PRIVATE Boost::json)

# 把测试 JSON 复制到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/test/test_items.json
    ${CMAKE_BINARY_DIR}/test/test_items.json
    COPYONLY
)

add_test(NAME ItemManagerTest COMMAND test_item_manager)
add_test(NAME ItemTypeTest COMMAND test_item_type)
add_test(NAME ItemSlotIntegrationTest COMMAND test_item_slot_integration)
