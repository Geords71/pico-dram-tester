set(VENV_DIR "${CMAKE_BINARY_DIR}/venv")

set(VENV_DIR "${CMAKE_BINARY_DIR}/venv")
set(VENV_PYTHON "${VENV_DIR}/bin/python")
if(CMAKE_HOST_WIN32)
    set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
endif()

if(NOT EXISTS "${VENV_DIR}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m virtualenv "${VENV_DIR}"
        RESULT_VARIABLE VENV_RESULT
        OUTPUT_VARIABLE VENV_OUT
        ERROR_VARIABLE VENV_ERR
    )

    if(NOT VENV_RESULT EQUAL "0")
        message(FATAL_ERROR "Failed to create virtualenv")
    endif()
endif()

set(VE_CREATE ${VENV_PYTHON} -m pip install -r ${CMAKE_CURRENT_LIST_DIR}/requirements.txt)
execute_process(
    COMMAND ${VE_CREATE}
    RESULT_VARIABLE VENV_RESULT
    OUTPUT_VARIABLE VENV_OUT
    ERROR_VARIABLE VENV_ERR
)
if(NOT VENV_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to install requirements into virtualenv:\n${VENV_ERR}")
endif()