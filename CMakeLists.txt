cmake_minimum_required(VERSION 3.16)

# ESP-IDF의 CMake 빌드 스크립트 로드
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 추가 컴포넌트 경로 등록 (프로젝트 내 components/ 폴더 사용)
# set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components)

# 프로젝트 이름 지정
project(esp-idf-template)

