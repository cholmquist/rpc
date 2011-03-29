/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_PROTOCOL_SERIALIZATION_HPP
#define BOOST_RPC_PROTOCOL_SERIALIZATION_HPP

#include <boost/rpc/core/tags.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>
#include <exception>

namespace boost
{

// Exception support
namespace serialization
{

template<class Archive>
void save(Archive& ar, const std::exception& e, const BOOST_PFTO unsigned int)
{
	std::string s(e.what());
	ar & s;
}

template<class Archive>
void load(Archive& ar, std::exception& e, const BOOST_PFTO unsigned int)
{
	std::string s;
	ar & s;
	e = std::exception(s.c_str());
}

}

namespace rpc {namespace protocol {

template<class OArchive, class IArchive, class IStream, class OStream>
class serialization
{
public:
	typedef serialization<OArchive, IArchive, IStream, OStream> base_type;

	unsigned int archive_flags;

	serialization(unsigned int archive_flags = 0)
		: archive_flags(archive_flags)
	{}

	class reader
	{
	public:
		reader(serialization& s, IStream& streambuf)
			: m_archive(streambuf, s.archive_flags)
		{}

		template<class T>
		void operator()(T& x, rpc::tags::parameter)
		{
			m_archive >> x;
		}

		template<class T>
		void operator()(T& x, rpc::tags::result)
		{
			m_archive >> x;
		}

		template<class T>
		void operator()(T& x, rpc::tags::exception)
		{
			m_archive >> x;
		}

	private:
		reader(const reader&);
		reader& operator=(const reader&);
		IArchive m_archive;
	};

	class writer
	{
	public:
		writer(serialization& s, OStream& streambuf)
			: m_archive(streambuf)
		{

		}

		template<class T>
		void operator()(const T& x, rpc::tags::parameter)
		{
			m_archive << x;
		}

		template<class T>
		void operator()(const T& x, rpc::tags::exception)
		{
			m_archive << x;
		}

		template<class T>
		void operator()(const T& x, rpc::tags::result)
		{
			m_archive << x;
		}

		OArchive& archive()
		{
			return m_archive;
		}

	private:
		writer(const writer&);
		writer& operator=(const writer&);

		OArchive m_archive;
	};

};

}}

}

BOOST_SERIALIZATION_SPLIT_FREE(std::exception)

#endif
