add_library(driver_arduinohmd MODULE driver_arduinohmd.cpp driverlog.cpp)
target_include_directories(driver_arduinohmd PRIVATE ../../headers)
target_compile_features(driver_arduinohmd PRIVATE cxx_std_11)
set_target_properties(driver_arduinohmd PROPERTIES PREFIX "")
target_link_options(driver_arduinohmd PRIVATE -Wl,--no-undefined)

install(TARGETS driver_arduinohmd LIBRARY DESTINATION bin/linux64)
install(FILES linux/driver.vrdrivermanifest DESTINATION .)
install(FILES linux/default.vrsettings DESTINATION resources/settings)
