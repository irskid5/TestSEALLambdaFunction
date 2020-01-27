cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(hello LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(SEAL 3.4 REQUIRED)
find_package(AWSSDK COMPONENTS dynamodb)

add_executable(${PROJECT_NAME} "main.cpp" "base64.cpp")

target_link_libraries(${PROJECT_NAME} PUBLIC AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES} SEAL::seal)

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")

aws_lambda_package_target(${PROJECT_NAME})
