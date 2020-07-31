cmake_minimum_required(VERSION 3.10)

set(SOURCES
	concurrent_fountain_decoder_sink.h
	FountainDecoder.h
	FountainEncoder.h
	FountainInit.h
	FountainMetadata.h
	fountain_decoder_sink.h
	fountain_decoder_stream.h
	fountain_encoder_stream.h
)

add_library(fountain INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

