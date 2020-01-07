set (ORIGINAL_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
set (ORIGINAL_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
set (CMAKE_C_FLAGS "-std=c99 -Wall -g -O2")
set (CMAKE_EXE_LINKER_FLAGS "-lm")

set(INDEXLIB_FILES
  handIndex/hand-isomorphism/src/deck.c
  handIndex/hand-isomorphism/src/deck.h
  handIndex/hand-isomorphism/src/hand_index.c
  handIndex/hand-isomorphism/src/hand_index.h
  handIndex/hand-isomorphism/src/hand_index-impl.h
)
add_library(universal_poker_indexlib OBJECT ${INDEXLIB_FILES})
set_target_properties(universal_poker_indexlib PROPERTIES POSITION_INDEPENDENT_CODE ON)

set (CMAKE_C_FLAGS ${ORIGINAL_CMAKE_C_FLAGS})
set (CMAKE_EXE_LINKER_FLAGS ${ORIGINAL_CMAKE_EXE_LINKER_FLAGS})

set(CLIB_FILES
  acpc/project_acpc_server/game.h
  acpc/project_acpc_server/game.cc
  acpc/project_acpc_server/rng.h
  acpc/project_acpc_server/rng.cc
)

add_library(universal_poker_clib OBJECT ${CLIB_FILES} )
set_target_properties(universal_poker_clib PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(HEADER_FILES
  acpc_cpp/acpc_game.h
  logic/card_set.h
  handIndex/index.h
)

set(SOURCE_FILES
  acpc_cpp/acpc_game.cc
  logic/card_set.cc
  handIndex/index.cpp
)

# The library contains header and source files.
add_library(universal_poker_lib OBJECT
  ${SOURCE_FILES}
  ${HEADER_FILES}
)

add_executable(universal_poker_acpc_cpp_test acpc_cpp/acpc_game_test.cc ${SOURCE_FILES}
        $<TARGET_OBJECTS:tests>)
target_link_libraries(universal_poker_acpc_cpp_test universal_poker_clib universal_poker_indexlib)

add_test(universal_poker_acpc_cpp_test universal_poker_acpc_cpp_test)

add_executable(universal_poker_card_set_test logic/card_set_test.cc ${SOURCE_FILES}
        $<TARGET_OBJECTS:tests>)
target_link_libraries(universal_poker_card_set_test universal_poker_clib universal_poker_indexlib)

add_test(universal_poker_card_set_test universal_poker_card_set_test)

add_executable(universal_poker_index_test handIndex/index_test.cpp ${SOURCE_FILES}
        $<TARGET_OBJECTS:tests>)
target_link_libraries(universal_poker_index_test universal_poker_clib universal_poker_indexlib)

add_test(universal_poker_index_test universal_poker_index_test)
