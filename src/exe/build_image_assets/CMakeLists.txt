cmake_minimum_required(VERSION 3.10)

project(build_image_assets)

set (SOURCES
	build_image_assets.cpp
)

add_executable (
	build_image_assets
	${SOURCES}
)

target_link_libraries(build_image_assets

	
)


