/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_PROTOCOL_BINARY_SERIALIZATION_HPP
#define BOOST_RPC_PROTOCOL_BINARY_SERIALIZATION_HPP

#include <boost/rpc/protocol/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>

namespace boost
{

namespace rpc {namespace protocol {

class binary_serialization : public rpc::protocol::serialization<
	archive::binary_oarchive, archive::binary_iarchive,
	interprocess::basic_vectorbuf<std::vector<char> >,
	interprocess::basic_vectorbuf<std::vector<char> > >
{

public:
	typedef std::vector<char> input;
	typedef std::vector<char> output;
	class reader;
	class writer;

private:
	typedef interprocess::basic_vectorbuf<std::vector<char> > stream_buf;

	struct stream_buf_base // Used for base-from-member
	{
	protected:
		stream_buf_base() {}

		stream_buf_base(input& in)
		{
			m_stream_buf.swap_vector(in);
		}
		stream_buf m_stream_buf;
	};

public:

	class reader : stream_buf_base, public base_type::reader
	{
	public:
		reader(binary_serialization& s, input& in)
			: stream_buf_base(in)
			, base_type::reader(s, m_stream_buf)
		{
		}

	private:

	};

	class writer : stream_buf_base, public base_type::writer
	{
	public:
		writer(binary_serialization& s, output& out)
			: stream_buf_base()
			, base_type::writer(s, m_stream_buf)
			, m_output(out)
		{
		}

		~writer()
		{
			// This is bit of a hack.
			// Since can't pass a reference to interprocess::vectorbuf we'll swap the data before leaving scope
			m_stream_buf.swap_vector(m_output);

		}

	private:
		writer(const writer&);
		writer& operator=(const writer&);
		output& m_output;
	};

};

}}

}

#endif
