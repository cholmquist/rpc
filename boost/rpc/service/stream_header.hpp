#ifndef BOOST_RPC_STREAM_HEADER_HPP
#define BOOST_RPC_STREAM_HEADER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/array.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/asio/buffer.hpp>
#include <algorithm>
#include <iterator>

namespace boost{ namespace rpc{	namespace detail{

	template<class Iterator>
	inline Iterator write_size_t(Iterator first, std::size_t size)
	{
		BOOST_STATIC_ASSERT((boost::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value));
		const boost::uint32_t sizes[5] = {0, 0xff, 0xffff, 0xffffff, 0xffffffff};
		int length = 0;
		for(int i = 0; i < 5; ++i)
		{
			if(size > sizes[i])
			{
				++length;
			}
			else
			{
				break;
			}
		}
		for(int i = length; i; --i)
		{
			(*first++) = static_cast<char>(static_cast<unsigned char>((size)));
			size >>= 8;
		}
		return first;
	}

	template<class Iterator>
	inline std::size_t read_size_t(Iterator first, Iterator last)
	{
		BOOST_STATIC_ASSERT((boost::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value));
		std::size_t size = 0;
		for(int i = 0; first != last; ++first)
		{
			size |= static_cast<unsigned char>(*first) << i;
			i += 8;
		}
		return size;
	}

	struct control_data
	{
		typedef boost::array<char, 1 + 4 + 4> buffer_type;

		static asio::const_buffer encode(buffer_type& buffer, std::size_t header_size, std::size_t payload_size)
		{
			std::ptrdiff_t header_length = write_size_t(&buffer[1], header_size) - &buffer[1];
			std::ptrdiff_t payload_length = write_size_t(&buffer[1 + header_length], payload_size) - (&buffer[1 + header_length]);
			buffer[0] = (header_length << 0) | (payload_length << 4);
			return asio::const_buffer(&buffer[0], 1 + header_length + payload_length);
		}

		enum decoder_state
		{
			ds_init,
			ds_header_size,
			ds_payload_size,
			ds_done,
		};

		struct decoder
		{
			decoder()
			{
				reset();
			}

			boost::tribool process(char cc)
			{
				const unsigned char c = static_cast<unsigned char>(cc);
				switch(m_state)
				{
				case ds_init:
					m_lengths = c;
					if((m_lengths >> 0)& 0x0f) // Check existence of header length
					{
						m_state = ds_header_size;
					}
					else if((m_lengths >> 4)& 0x0f) // Check existence of payload length
					{
						m_state = ds_payload_size;
					}
					else
					{
						m_state = ds_done;
						return true; // Both header and payload lengths are zero
					}
					break;

				case ds_header_size:
					m_header_size |= (c << (m_counter * 8));
					if((++m_counter) == ((m_lengths >> 0)& 0x0f))
					{
						if((m_lengths >> 4)& 0x0f) // Check existence of payload length
						{
							m_counter = 0;
							m_state = ds_payload_size;
						}
						else
						{
							m_state = ds_done;
							return true;
						}
					}
					break;

				case ds_payload_size:
					m_payload_size |= (c << (m_counter * 8));
					if((++m_counter) == ((m_lengths >> 4)& 0x0f))
					{
						m_state = ds_done;
						return true;
					}
					break;

				case ds_done:
					return false;
				}
				return boost::indeterminate;
			}

			void reset()
			{
				m_state = ds_init;
				m_header_size = 0;
				m_payload_size = 0;
				m_counter = 0;
				m_lengths = 0;
			}

			std::size_t header_size() const
			{
				return static_cast<std::size_t>(m_header_size);
			}

			std::size_t payload_size() const
			{
				return static_cast<std::size_t>(m_payload_size);
			}

			template<class HeaderBuffer, class PayloadBuffer>
			void init_buffers(HeaderBuffer& header, PayloadBuffer& payload)
			{
				header.clear();
				header.reserve(this->header_size());
				payload.clear();
				payload.reserve(this->payload_size());
			}

		private:
			decoder_state m_state;
			boost::uint32_t m_header_size;
			boost::uint32_t m_payload_size;
			unsigned int m_counter;
			unsigned char m_lengths;
		};
	};

	template<std::size_t Size>
	class receive_buffer
	{
	public:
		receive_buffer()
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

		template<class Iterator>
		std::size_t flush(Iterator out, std::size_t max_size)
		{
			std::size_t n = (std::min)(max_size, static_cast<std::size_t>(m_end - m_begin));
			std::copy(m_begin, m_begin + n, out);
			m_begin += n;
			return n;
		}

		asio::mutable_buffer prepare() const
		{
			return asio::mutable_buffer(m_end, static_cast<std::size_t>(&m_buffer[Size] - m_end));
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
			std::size_t remain(static_cast<std::size_t>(m_end - m_begin));
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

}}}

#endif
