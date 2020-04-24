set(SRCFILES lookup.c convolutional.c encode.c decode.c)
add_library(correct-convolutional-sse OBJECT ${SRCFILES})
