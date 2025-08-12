# SPDX-License-Identifier: MIT

set(SRC_FILES
    main.c
    ethernet.c
    nvram.c
)

idf_component_register(
    SRCS ${SRC_FILES}         # 메인 소스 파일
    INCLUDE_DIRS "."      # include 경로
)
