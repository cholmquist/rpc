#            Copyright (c) Christian Holmquist 2011.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

include_directories(${BOOST-RPC_SOURCE_DIR})
find_package( Boost 1.40.0 )
add_executable(console console/console.cpp)
