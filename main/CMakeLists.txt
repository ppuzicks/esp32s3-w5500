# SPDX-License-Identifier: MIT

set(SRC_FILES
    main.c
    ethernet.c
    event_handler.c
    http_client.c
    prov.c
    nvram.c
    utils.c
)

set(PKGS
    xxhash
    esp_netif
    esp_eth
    esp_http_client
    nvs_flash
    mbedtls
)

idf_component_register(
    SRCS ${SRC_FILES}         # 메인 소스 파일
    INCLUDE_DIRS "."      # include 경로
    PRIV_REQUIRES ${PKGS}
    # EMBED_TXTFILES AmazonRootCA1.pem
)
