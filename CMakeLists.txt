#            Copyright (c) Christian Holmquist 2011.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 2.8)

project(BOOST-RPC)

find_package(Boost COMPONENTS system thread REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTI_THREADED ON)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories( ${Boost_INCLUDE_DIRS} )

add_subdirectory(libs/rpc/example)
add_subdirectory(libs/rpc/test)
