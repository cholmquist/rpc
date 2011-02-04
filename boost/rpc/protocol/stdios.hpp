/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#pragma once

#include <boost/rpc/tags.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <iosfwd>
#include <stdexcept>

namespace boost{ namespace rpc { namespace protocol {

template<class IStream = std::istream, class OStream = std::ostream>
class stdios : boost::noncopyable
{

	IStream&	is_;
	OStream&	os_;

public:

	stdios(IStream& is, OStream& os) : is_(is), os_(os) {}

	template<class T>
	void write(const T& w, tags::default_)
	{
		os_ << w << " ";
	}

	template<class T>
	void read(T& r, tags::default_)
	{
		is_ >> r;
	}

	IStream& in() const
	{
		return is_;
	}

	OStream& out() const
	{
		return os_;
	}

};

}}}
