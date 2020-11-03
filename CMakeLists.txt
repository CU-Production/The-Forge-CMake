cmake_minimum_required(VERSION 3.16)
project(TFhelloworld)

set(CMAKE_CXX_STANDARD 14)

set(THE_FORGE_PATH "E:/theforgeworkspace/The-Forge" CACHE PATH "The-Forge path")
set(THE_FORGE_USE_DX12 ON  CACHE BOOL "Link DX12")
set(THE_FORGE_USE_VK   OFF CACHE BOOL "Link Vulkan")

message(${THE_FORGE_PATH})
message(${THE_FORGE_USE_DX12})
message(${THE_FORGE_USE_VK})

include_directories(${THE_FORGE_PATH})

list(APPEND THE_FORGE_LIBS
        Xinput9_1_0
        ws2_32)

if (${THE_FORGE_USE_DX12})
    message("THE_FORGE_USE_DX12")
    link_directories(${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugDx)
    list(APPEND THE_FORGE_LIBS
#            amd_ags_x64
#            WinPixEventRunTime
#            dxcompiler
#            dxil
            gainputstatic
            OS
            RendererDX12
            )

    add_definitions(
            -DUSE_MEMORY_TRACKING
            -D_DEBUG
            -D_WINDOWS
            -DDIRECT3D12)

    add_custom_target(DX12copyDLL
            COMMAND ${CMAKE_COMMAND} -E copy ${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugDx/amd_ags_x64.dll          ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugDx/WinPixEventRuntime.dll   ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugDx/dxcompiler.dll           ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugDx/dxil.dll                 ${CMAKE_BINARY_DIR})

endif ()

if (${THE_FORGE_USE_VK})
    message("THE_FORGE_USE_VK")
    find_package(Vulkan REQUIRED)

    IF (NOT Vulkan_FOUND)
        message(FATAL_ERROR "Could not find Vulkan library!")
    ELSE()
        message(STATUS ${Vulkan_LIBRARY})
    ENDIF()

    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY})
    link_directories(${THE_FORGE_PATH}/Examples_3/Unit_Tests/PC\ Visual\ Studio\ 2017/x64/DebugVk)
    list(APPEND THE_FORGE_LIBS
            gainputstatic
            OS
            RendererVulkan
            SpirvTools
            ${Vulkan_LIBRARIES})
    add_definitions(
            -DUSE_MEMORY_TRACKING
            -D_DEBUG
            -D_WINDOWS
            -DVULKAN
    )
endif ()

#message(${THE_FORGE_LIBS})

add_executable(TFhelloworld triangle.cpp)
target_link_libraries(TFhelloworld ${THE_FORGE_LIBS})
set_target_properties(TFhelloworld PROPERTIES LINK_FLAGS "/ignore:4099")
if (${THE_FORGE_USE_DX12})
    add_dependencies(TFhelloworld DX12copyDLL)
endif ()

add_executable(01_Transformations Examples/01_Transformations.cpp)
target_link_libraries(01_Transformations ${THE_FORGE_LIBS})
set_target_properties(01_Transformations PROPERTIES LINK_FLAGS "/ignore:4099")
if (${THE_FORGE_USE_DX12})
    add_dependencies(01_Transformations DX12copyDLL)
endif ()
