cmake_minimum_required(VERSION 2.8)
project(webRTC_denoise) 
add_library(webDenoise SHARED webRTC.c
        noise_suppression.c
        )
target_link_libraries(webDenoise -lm)
