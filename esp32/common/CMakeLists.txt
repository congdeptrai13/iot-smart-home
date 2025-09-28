idf_component_register(
    SRCS "ring_buf.c" "UART_lib.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)