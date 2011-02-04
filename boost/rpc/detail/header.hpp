#if 0
#ifndef BOOST_RPC_DETAIL_HEADER_HPP
#define BOOST_RPC_DETAIL_HEADER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/rpc/detail/switch_.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <cstring> // std::memcpy

namespace boost{ namespace rpc{ namespace detail{

template<class HeaderVariant>
struct max_header_length
{
	struct cmp
	{
		template<class Left, class Right>
		struct apply : mpl::if_c< ((Left::static_length) < (Right::static_length)), mpl::true_, mpl::false_>
		{
		};
	};

	static const int value = mpl::deref<
		typename mpl::max_element<typename HeaderVariant::types, cmp>::type
	>::type::static_length;
};

template<class HeaderVariant>
struct construct_header2
{
	HeaderVariant& header;
	uint8_t& length;
	construct_header2(HeaderVariant& h, uint8_t& l)
		: header(h)
		, length(l)
	{

	}

	template<class N>
	void operator()(N) const
	{
		typedef typename mpl::at<typename HeaderVariant::types, N>::type header_type;
		header = header_type();
		length = header_type::static_length;
	}
};

struct copy_header_buffer : boost::static_visitor<uint8_t>
{
	uint8_t* m_dst;
	copy_header_buffer(uint8_t *dst) : m_dst(dst) {}
	template<class Header>
	uint8_t operator()(const Header& h) const
	{
		BOOST_STATIC_ASSERT(sizeof(h.data) < 255);
		std::memcpy(m_dst, &h.data[0], sizeof(h.data));
		return sizeof(h.data);
	}
};

struct set_header_char : boost::static_visitor<bool>
{
	int m_offset;
	char m_value;
	set_header_char(int offset, char value)
		: m_offset(offset)
		, m_value(value) {}
	template<class Header>
	bool operator()(Header& h) const
	{
		if(m_offset < sizeof(h.data))
		{
			h.data[m_offset] = m_value;
			return true;
		}
		return false;
	}
};

enum decoder_state
{
	ds_init,
	ds_payload_size,
	ds_header,
	ds_done,
	ds_fail,
};

template<int Size>
struct storage
{
	storage()
		: m_begin(m_buffer)
		, m_end(m_buffer)
	{
	}

	uint8_t pop()
	{
		BOOST_ASSERT(m_begin != m_end);
		return *(m_begin++);
	}

	void debug_push(uint8_t c)
	{
		*(asio::buffer_cast<uint8_t*>(prepare())) = c;
		commit(1);
	}

	bool empty() const
	{
		return m_begin == m_end;
	}

	void commit(std::size_t n)
	{
		m_end += n;
		BOOST_ASSERT(m_end <= &m_buffer[Size]);
	}

	std::size_t flush(char *out, std::size_t size)
	{
		std::size_t n = (std::min)(size, static_cast<std::size_t>(m_end - m_begin));
		std::memcpy(out, m_begin, n);
		m_begin += n;
		return n;
	}

	asio::mutable_buffer prepare() const
	{
		return asio::mutable_buffer(m_end, &m_buffer[Size] - m_end);
	}

	asio::const_buffer reset()
	{
		asio::const_buffer remain(m_begin, m_end - m_begin);
		m_begin = m_buffer;
		m_end = m_buffer;
		return remain;
	}

	void reset2()
	{
		std::size_t remain(m_end - m_begin);
		if(remain)
		{
			std::memmove(m_buffer, m_begin, remain);
		}
		m_begin = m_buffer;
		m_end = m_buffer + remain;
	}

private:
	uint8_t m_buffer[Size];
	uint8_t* m_begin;
	uint8_t* m_end;
};

template<class Data>
struct header
{
	//static const int max_header_length = max_header_length<Data>::value;
	static const int max_header_length = 4;

	struct decoder
	{
		decoder()
		{
			reset();
		}

		decoder_state process(uint8_t c)
		{
			switch(m_state)
			{
			case ds_init:
				m_control_bits = c;
				if(rpc::detail::switch_<mpl::size<typename Data::types>::value>(static_cast<int>(variant_index()),
					detail::construct_header2<Data>(m_header, m_header_length)))
				{
					if(payload_size_length())
						m_state = ds_payload_size;
					else if(m_header_length)
						m_state = ds_header;
					else
						m_state = ds_done;
				}
				else
				{
					m_state = ds_fail;
				}
				break;
			case ds_payload_size:
				m_payload_size |= (c << (m_payload_size_itr * 8));
				if(++m_payload_size_itr == payload_size_length())
				{
					if(m_header_length)
						m_state = ds_header;
					else
						m_state = ds_done;
				}
				break;
			case ds_header:
				if(boost::apply_visitor(set_header_char(m_header_itr++, c), m_header))
				{
					if(m_header_itr == m_header_length)
						m_state = ds_done;
				}
				else
				{
					m_state = ds_fail;
				}
				break;

			case ds_done:
				BOOST_ASSERT(false && "already done");
				break;
			}
			return static_cast<decoder_state>(m_state);
		}

		std::size_t payload_size() const
		{
			return static_cast<std::size_t>(m_payload_size);
		}

		uint8_t variant_index() const
		{
			return m_control_bits & ((1 << 6) - 1);
		}

		uint8_t payload_size_length() const
		{
			return m_control_bits >> 6;
		}

		void reset()
		{
			m_state = ds_init;
			m_payload_size = 0;
			m_header_length = 0;

			m_control_bits = 0;
			m_header_itr = 0;
			m_payload_size_itr = 0;
		}

		const Data& header() const { return m_header;}

		Data m_header;

		uint32_t m_payload_size;
		uint8_t m_state;
		uint8_t m_header_length;
		uint8_t m_control_bits;
		uint8_t m_header_itr;
		uint8_t m_payload_size_itr;
	};

	struct encoder
	{
		encoder(const Data& header, std::size_t payload_size)
			: m_buffer_size(0)
		{
			uint8_t control_bits = header.which();
			uint8_t payload_size_length = 0;
			if(payload_size > 0)
			{
				if(payload_size > 0xff)
					if(payload_size > 0xffff)
						payload_size_length = 3;
					else
						payload_size_length = 2;
				else
					payload_size_length = 1;
			}
			control_bits |= payload_size_length << 6;
			m_buffer[m_buffer_size++] = control_bits;
			for(; payload_size_length; --payload_size_length)
			{
				m_buffer[m_buffer_size++] = static_cast<uint8_t>(payload_size);
				payload_size >>= 8;
			}
			m_buffer_size += static_cast<uint32_t>(boost::apply_visitor(copy_header_buffer(m_buffer + m_buffer_size), header));
		}

		asio::const_buffer buffer() const
		{
			return asio::const_buffer(m_buffer, static_cast<std::size_t>(m_buffer_size));
		}
		uint32_t	m_buffer_size;
		uint8_t		m_buffer[1 + 3 + max_header_length];
	};

	typedef storage<1 + 3 + max_header_length> storage_type;

};

}}}

#endif
#endif
