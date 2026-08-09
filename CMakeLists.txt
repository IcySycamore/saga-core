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

# ===== 查找 SDL3（渲染窗口/输入，zlib 许可） =====
set(SDL3_DIR C:/msys64/clang64/lib/cmake/SDL3 CACHE PATH "SDL3 cmake dir")
find_package(SDL3 REQUIRED)
if(SDL3_FOUND)
    message(STATUS "SDL3 found: ${SDL3_VERSION}")
endif()

# ===== 核心库（头文件为主，少量 .cpp） =====
set(CORE_SOURCES
    core/Entity/EntityManager.cpp
    # core/Entity/Inventory.cpp  # TODO: 待适配新组件体系后重新加入（依赖旧 API getArcheType/setCharges/m_val_0 等）
)

set(CORE_HEADERS
    core/Dice/Dice.h
    core/Dice/Dice20.h
    core/Event/Event.h
    # core/Event/Check.h  # TODO: 依赖已删除的旧 Entity 类（getStatue/Statue::GM），待重构
    core/Event/CreatPlayer.h
    core/Event/Gameinit.h
    core/Event/LoadScene.h
    core/Event/Move.h
    core/Event/Plot.h
    core/helper/RandomEng.h
    core/helper/UuidGen.h
    core/Entity/EntityManager.h
    core/Entity/EntityType.h
    core/Entity/ItemSlot.h
    core/math/Math.h
    core/math/Matrix3x3.h
    core/math/Matrix4x4.h
    core/math/Quaternion.h
    core/math/TransformTools.h
    core/math/Vector2.h
    core/math/Vector3.h
    core/render/Camera.h
    core/render/OpenGLDevice.h
    core/render/RenderDevice.h
    core/render/SDL3Device.h
    core/render/SoftwareDevice.h
    core/Time/Clock.h
)

add_library(trpg_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(trpg_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(trpg_core PUBLIC Boost::json)

# 代表物语义优化：构建时命令行可选参数，默认开启
# 关闭方式：cmake -DTRPG_ENABLE_REP_SEMANTIC_OPTIMIZATION=OFF
option(TRPG_ENABLE_REP_SEMANTIC_OPTIMIZATION
       "Enable representative semantic optimization" ON)
if(TRPG_ENABLE_REP_SEMANTIC_OPTIMIZATION)
    target_compile_definitions(trpg_core PRIVATE REP_SEMANTIC_OPTIMIZATION)
endif()

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

add_executable(test_math
    test/test_math.cpp
)

target_include_directories(test_math PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_math PRIVATE Boost::json)

# 把测试 JSON 复制到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/test/test_items.json
    ${CMAKE_BINARY_DIR}/test/test_items.json
    COPYONLY
)

add_test(NAME ItemManagerTest COMMAND test_item_manager)
add_test(NAME ItemTypeTest COMMAND test_item_type)
add_test(NAME ItemSlotIntegrationTest COMMAND test_item_slot_integration)
add_test(NAME MathTest COMMAND test_math)
add_test(NAME TimeTest COMMAND test_time)
add_test(NAME RandomTest COMMAND test_random)
add_test(NAME RenderTest COMMAND test_render)

add_executable(test_time
    test/test_time.cpp
)

target_include_directories(test_time PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_time PRIVATE Boost::json)

add_executable(test_random
    test/test_random.cpp
)

target_include_directories(test_random PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_random PRIVATE Boost::json)

add_executable(test_render
    test/test_render.cpp
)

target_include_directories(test_render PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_render PRIVATE Boost::json)

# ===== DJ 工具（命令行音乐电台，播放音乐到虚拟麦克风） =====
add_executable(dj
    tools/dj/dj.cpp
)
target_include_directories(dj PRIVATE ${CMAKE_SOURCE_DIR}/tools/dj)

if(WIN32)
    # miniaudio(WASAPI backend) + CommandLineToArgvW
    target_link_libraries(dj PRIVATE ole32 oleaut32 winmm user32 shell32)
endif()

# ===== 渲染演示（M1-M5：SDL3 窗口 + RenderDevice 抽象 + 三后端） =====
add_executable(render_demo
    tools/render_demo.cpp
)
target_include_directories(render_demo PRIVATE ${CMAKE_SOURCE_DIR})
# SDL3 提供窗口/事件/渲染；若存在 SDL3main（WinMain 入口）则一并链接
if(TARGET SDL3::SDL3main)
    target_link_libraries(render_demo PRIVATE SDL3::SDL3main SDL3::SDL3)
else()
    target_link_libraries(render_demo PRIVATE SDL3::SDL3)
endif()
# OpenGL（OpenGLDevice 用固定管线；Windows 上链接 opengl32）
find_package(OpenGL REQUIRED)
if(OpenGL_FOUND)
    message(STATUS "OpenGL found: ${OPENGL_LIBRARIES}")
    target_link_libraries(render_demo PRIVATE OpenGL::GL)
endif()
