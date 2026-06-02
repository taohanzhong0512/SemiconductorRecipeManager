# proto/CMakeLists.txt
# 🌟 注意：这里不再需要任何 find_package！

# 获取 protoc 可执行文件路径
find_program(PROTOC_EXE protoc HINTS "${Protobuf_PROTOC_EXECUTABLE}")
if(NOT PROTOC_EXE AND TARGET protobuf::protoc)
    get_target_property(PROTOC_EXE protobuf::protoc IMPORTED_LOCATION_RELEASE)
    if(NOT PROTOC_EXE)
        get_target_property(PROTOC_EXE protobuf::protoc IMPORTED_LOCATION_DEBUG)
    endif()
endif()
if(NOT PROTOC_EXE)
    message(FATAL_ERROR "Could not find protoc executable!")
endif()

set(PROTO_FILES recipe.proto)
set(GENERATED_SRCS)
set(GENERATED_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GEN_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(GEN_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")

    add_custom_command(
            OUTPUT ${GEN_SRC} ${GEN_HDR}
            COMMAND ${PROTOC_EXE}
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
            COMMENT "Generating C++ code from ${PROTO_FILE}..."
    )
    list(APPEND GENERATED_SRCS ${GEN_SRC})
    list(APPEND GENERATED_HDRS ${GEN_HDR})
endforeach()

add_library(ProtoLib STATIC ${GENERATED_SRCS} ${GENERATED_HDRS})
target_include_directories(ProtoLib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# 🌟 直接链接根目录已经找到的 Target，CMake 会自动传递 absl 和 utf8_range 的依赖
target_link_libraries(ProtoLib PUBLIC protobuf::libprotobuf)
