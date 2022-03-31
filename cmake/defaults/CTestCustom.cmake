# set a timestamp id to identify this run of tests in the environment, if not
# already set
if (NOT DEFINED ENV{PXR_CTEST_RUN_ID})
    # otherwise, 
    string(TIMESTAMP _current_time)
    set(ENV{PXR_CTEST_RUN_ID} ${_current_time})
endif()
