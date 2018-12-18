#
# This file is part of the GUI library.
# Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

include(FetchContent)
find_package(OpenGL QUIET)

if(OPENGL_FOUND)
  if(WIN32)
    # fetch prebuild binaries
  	FetchContent_Declare(
      glew
      URL https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0-win32.zip
      URL_HASH MD5=32A72E6B43367DB8DBEA6010CD095355
  	)
  
  	FetchContent_GetProperties(glew)
  	if(NOT glew_POPULATED)
      execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "Fetching GLEW...")
      FetchContent_Populate(glew)
      execute_process(COMMAND ${CMAKE_COMMAND} -E echo "Done")
  	endif()
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(GLEW_ARCH "x64") # 64 bits
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(GLEW_ARCH "Win32") # 32 bits
    endif()
    
  	find_library(GLEW_LIBRARIES
  	  NAMES glew32
  	  HINTS ${glew_SOURCE_DIR}/bin/Release/${GLEW_ARCH}
  	)
  	find_path(GLEW_INCLUDE_DIRS
  	  NAMES glew.h
  	  HINTS ${glew_SOURCE_DIR}/include
  	  PATH_SUFFIXES GL
  	)
    set(GLEW_FOUND TRUE)
  else()
    # Dependency to GLEW
    find_package(GLEW)
  endif()
  
  if(GLEW_FOUND)
    set(GLIMPLEMENTATION_DEFINITIONS "LIB_GL" "LIB_GLEW")
    set(GLIMPLEMENTATION_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
    set(GLIMPLEMENTATION_LIBRARIES ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
  else()
  	message(FATAL_ERROR "Could not find GLEW.")
  endif()
else()
	message(FATAL_ERROR "Could not find OpenGL.")
endif()