set(CMAKE_INCLUDE_CURRENT_DIR on)

include_directories(${FRAMERECEIVER_DIR}/include ${ODINDATA_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

# Add library for xspress ZMQ decoder
add_library(XspressFrameDecoder SHARED XspressFrameDecoder.cpp XspressFrameDecoderLib.cpp)
target_link_libraries(XspressFrameDecoder ${ODINDATA_LIBRARIES} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})

# Add library for xspress UDP decoder
add_library(XspressListModeFrameDecoder SHARED XspressListModeFrameDecoder.cpp XspressListModeFrameDecoderLib.cpp)
target_link_libraries(XspressListModeFrameDecoder ${ODINDATA_LIBRARIES} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})

# Add library for xspress X3X2 list mode decoder
add_library(X3X2ListModeFrameDecoder SHARED X3X2ListModeFrameDecoder.cpp X3X2ListModeFrameDecoderLib.cpp)
target_link_libraries(X3X2ListModeFrameDecoder ${ODINDATA_LIBRARIES} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})


install(TARGETS XspressFrameDecoder
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(TARGETS XspressListModeFrameDecoder
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(TARGETS X3X2ListModeFrameDecoder
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
