add_executable(surge-scoring-render ScoringRender.cpp)
target_link_libraries(surge-scoring-render PRIVATE surge::surge-common)

add_executable(surge-embedded-content-test EmbeddedContentTest.cpp)
target_link_libraries(surge-embedded-content-test PRIVATE surge::surge-common)
