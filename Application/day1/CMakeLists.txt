project(iio_apps)

add_executable(hello_iio hello_iio.c)
add_executable(ctx_attr ctx_attr.c)
add_executable(ctx_find ctx_find.c)
add_executable(ctx_read_attr ctx_read_attr.c)
add_executable(ctx_chn_attr ctx_temp_read.c)
add_executable(ctx_temp_read ctx_temp_read.c)

target_link_libraries(hello_iio iio)
target_link_libraries(ctx_attr iio)
target_link_libraries(ctx_find iio)
target_link_libraries(ctx_read_attr iio)
target_link_libraries(ctx_chn_attr iio)
target_link_libraries(ctx_temp_read iio)
