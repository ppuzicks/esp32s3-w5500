cmake_minimum_required(VERSION 3.16)

# ESP-IDF의 CMake 빌드 스크립트 로드
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 추가 컴포넌트 경로 등록 (프로젝트 내 components/ 폴더 사용)
# set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components)

# 기본값: menuconfig 설정
set(PROJECT_VER ${CONFIG_PROJECT_VERSION})

# Git 태그 자동 감지
execute_process(
    COMMAND git describe --tags --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND date "+%Y-%m-%d %H:%M:%S"
    OUTPUT_VARIABLE BUILD_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Git 버전이 감지되면 덮어쓰기
if (GIT_VER)
    set(PROJECT_VER ${GIT_VER})
endif()

message(STATUS "Firmware Version: ${PROJECT_VER}")
message(STATUS "Git Commit: ${GIT_COMMIT}")
message(STATUS "Build Date: ${BUILD_DATE}")

# 전처리 매크로로 버전 전달
idf_build_set_property(COMPILE_DEFINITIONS
    FW_VER=\"${PROJECT_VER}\"
    GIT_COMMIT_HASH=\"${GIT_COMMIT}\"
    BUILD_DATE=\"${BUILD_DATE}\"
    APPEND
)

# 프로젝트 이름 지정
project(esp32s-w5500)
