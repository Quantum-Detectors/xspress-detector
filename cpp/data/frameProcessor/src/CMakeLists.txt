set(CMAKE_INCLUDE_CURRENT_DIR on)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

# Add library for Xspress process plugin
add_library(XspressProcessPlugin SHARED XspressProcessPlugin.cpp XspressProcessPluginLib.cpp)
target_link_libraries(XspressProcessPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for Xspress list mode process plugin
add_library(XspressListModeProcessPlugin SHARED XspressListModeProcessPlugin.cpp XspressListModeProcessPluginLib.cpp)
target_link_libraries(XspressListModeProcessPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for X3X2 list mode process plugin
add_library(X3X2ListModeProcessPlugin SHARED X3X2ListModeProcessPlugin.cpp X3X2ListModeProcessPluginLib.cpp)
target_link_libraries(X3X2ListModeProcessPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

install(TARGETS XspressProcessPlugin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(TARGETS XspressListModeProcessPlugin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(TARGETS X3X2ListModeProcessPlugin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
