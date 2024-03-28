project(userver-samples-telegram_bot_service CXX)

add_executable(${PROJECT_NAME} "telegram_bot_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-telegram)

userver_sample_testsuite_add()
