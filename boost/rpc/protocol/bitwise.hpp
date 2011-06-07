/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_PROTOCOL_BITWISE_HPP
#define BOOST_RPC_PROTOCOL_BITWISE_HPP

#include <boost/rpc/core/tags.hpp>
#include <boost/rpc/detail/init_variant.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/array.hpp>
#include <boost/numeric/conversion/cast.hpp> // Temporary solution for std::size_t

// container_tag_of includes
#include <boost/fusion/support/tag_of.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/variant/variant.hpp>

// array support
#include <boost/array.hpp>
#include <boost/mpl/if.hpp>

// exception support
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>

#include <vector>
#include <cstring>

namespace boost{ namespace rpc { namespace traits {

	struct bitwise_container_tag {};
	struct std_container_tag {};
	struct fusion_container_tag {};
	struct variant_container_tag {};
	struct exception_tag {};
	struct array_container_tag : std_container_tag {};
	struct unknown_tag {};

	namespace detail
	{
		BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)
		BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
		BOOST_MPL_HAS_XXX_TRAIT_DEF(size_type)
		BOOST_MPL_HAS_XXX_TRAIT_DEF(reference)
	}

	template<class T, typename Active = void>
	struct container_tag_of
	{
		typedef unknown_tag type;
	};

	template<class T>
	struct container_tag_of<T, typename boost::enable_if<fusion::detail::has_fusion_tag<T> >::type>
	{
		typedef fusion_container_tag type;
	};

	template<class T>
	struct container_tag_of<T, typename boost::enable_if<serialization::is_bitwise_serializable<T> >::type>
	{
		typedef bitwise_container_tag type;
	};

    template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct container_tag_of<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    {
		typedef variant_container_tag type;
	};

	template<class T>
	struct container_tag_of<T, typename boost::enable_if<is_array<T> > >
	{
		typedef array_container_tag type;
	};

	template<class T>
	struct container_tag_of<T, typename boost::enable_if<boost::is_base_of<std::exception, T> >::type >
	{
		typedef exception_tag type;
	};

	template<class T>
	struct is_array : boost::false_type
	{
	};

	template<class T, std::size_t N>
	struct is_array<boost::array<T, N> > : boost::true_type
	{
	};

	template <typename T>
	struct container_tag_of<T, typename boost::enable_if_c<
        detail::has_value_type<T>::value &&
        detail::has_iterator<T>::value &&
        detail::has_size_type<T>::value &&
		detail::has_reference<T>::value>::type >
	{
		typedef typename boost::mpl::if_<is_array<T>, array_container_tag, std_container_tag>::type type;
//		typedef std_container_tag type;
	};

}}}

namespace boost{ namespace rpc { namespace protocol {

namespace detail
{
	template<class Function, class A1>
	struct refbinder1 : A1
	{
		refbinder1(Function& f, A1 a1) : m_function(f), A1(a1) {}

		typedef void result_type;

		template<class T>
		void operator()(T& t) const
		{
			m_function(t, static_cast<A1>(*this));
		}

		template<class T>
		void operator()(T const& t) const
		{
			m_function(t, static_cast<A1>(*this));
		}

		Function& m_function;

	};

	template<class Function, class A1>
	refbinder1<Function, A1> refbind1(Function& f, A1 a1)
	{
		return refbinder1<Function, A1>(f, a1);
	}

	template<class Exception>
	void assign_exception_what_impl(Exception& e, const std::string& what, boost::true_type)
	{
		e = Exception(what);
	}

	template<class Exception>
	void assign_exception_what_impl(Exception& e, const std::string& what, boost::false_type)
	{
		e = Exception(what.c_str());
	}

	template<class Exception>
	void assign_exception_what(Exception& e, const std::string& what)
	{
		assign_exception_what_impl(e, what, boost::is_convertible<std::string, Exception>());
	}

};

struct bitwise_reader_error : std::exception
{
	virtual const char* what() const
	{
		return "bitwise reader error";
	}
};

/*
class bitwise
{
public:

	typedef std::vector<char> input_type;
	typedef std::vector<char> output_type;

	std::size_t reserve_size;

	bitwise(std::size_t reserve_size = 0)
		: reserve_size(reserve_size)
	{}
*/
	template<class Buffer = std::vector<char> >
	struct bitwise_reader
	{
		typedef Buffer input_type;

		bitwise_reader(input_type& in)
			: m_input(in)
			, m_cursor(0)
		{
//			m_input.reserve(p.reserve_size);
		}

		template<class T, class Tag>
		void operator()(T& t, Tag tag)
		{
			this->read(t, tag, rpc::traits::container_tag_of<T>::type());
		}

		template<class T, class Tag>
		void read(T& t, Tag, rpc::traits::bitwise_container_tag)
		{
			const std::size_t length = sizeof(T);
			if(length <= m_input.size() + m_cursor)
			{
				std::memcpy(boost::addressof(t), &m_input[m_cursor], length);
				m_cursor += length;
			}
			else
			{
				throw_exception(bitwise_reader_error());
			}
		}

		template<class T, class Tag>
		void read(T& t, Tag tag, rpc::traits::fusion_container_tag)
		{
			fusion::for_each(t, detail::refbind1(*this, tag));
		}

		template<class T, class Tag>
		void read(T& t, Tag tag, rpc::traits::variant_container_tag)
		{
			char which = 0;
			read(which, tag, rpc::traits::bitwise_container_tag());
			if(rpc::detail::init_variant(which, t, detail::refbind1(*this, tag)) == false)
			{
				throw_exception(bitwise_reader_error());
			}
		}

		template<class Container, class Tag>
		void read(Container& c, Tag tag, rpc::traits::std_container_tag)
		{
			// TODO: Implement better read/write of std::size_t
			boost::uint32_t size = 0;
			read(size, tag, rpc::traits::bitwise_container_tag());
			for(boost::uint32_t i = 0; i < size; ++i)
			{
				typename Container::value_type val;
				this->read(val, tag, rpc::traits::container_tag_of<typename Container::value_type>::type());
				c.insert(c.end(), val);
			}
		}

		template<class Container, class Tag>
		void read(Container& c, Tag tag, rpc::traits::array_container_tag)
		{
			boost::uint32_t size = 0;
			read(size, tag, rpc::traits::bitwise_container_tag());
			for(boost::uint32_t i = 0; i < size; ++i)
			{
				typename Container::value_type val; // TODO: Remove recreation of val, by implementing detail::clear(T&, container_tag);
				this->read(val, tag, rpc::traits::container_tag_of<typename Container::value_type>::type());
				c[i] = val;
			}
		}

		template<class T, class Tag>
		void read(T& t, Tag tag, rpc::traits::exception_tag)
		{
			std::string what;
			read(what, tag, rpc::traits::std_container_tag());
			detail::assign_exception_what(t, what);
		}

		template<class T, class Tag>
		void read(T& t, Tag tag, rpc::traits::unknown_tag)
		{
			t.serialize(*this, 0);
		}

	private:
		input_type&	m_input;
		std::size_t m_cursor;
	};

	template<class Buffer = std::vector<char> >
	struct bitwise_writer
	{
	public:
		typedef Buffer output_type;

		bitwise_writer(output_type& out)
			: m_output(out)
		{
		}

		template<class T, class Tag>
		void operator()(const T& t, Tag tag)
		{
			this->write(t, tag, typename rpc::traits::container_tag_of<T>::type());
		}

		template<class T, class Tag>
		void write(const T& t, Tag, rpc::traits::bitwise_container_tag)
		{
			const std::size_t length = sizeof(T);
			const std::size_t cursor = m_output.size();
			m_output.resize(cursor + length);
			std::memcpy(&m_output[cursor], boost::addressof(t), length);
		}

		template<class T, class Tag>
		void write(const T& t, Tag tag, rpc::traits::fusion_container_tag)
		{
			fusion::for_each(t, detail::refbind1(*this, tag));
		}

		template<class T, class Tag>
		void write(const T& t, Tag tag, rpc::traits::variant_container_tag)
		{
			const char which = static_cast<char>(t.which());
			write(which, tag, rpc::traits::bitwise_container_tag());
			boost::apply_visitor(detail::refbind1(*this, tag), t);
		}

		template<class T, class Tag>
		void write(const T& t, Tag tag, rpc::traits::std_container_tag)
		{
			// TODO: Implement better read/write of std::size_t
			const boost::uint32_t size = boost::numeric_cast<boost::uint32_t>(t.size());
			write(size, tag, rpc::traits::bitwise_container_tag());
			for(typename T::const_iterator i = t.begin(), e = t.end(); i != e; ++i)
			{
				this->write(*i, tag, typename rpc::traits::container_tag_of<typename T::value_type>::type());
			}
		}

		template<class T, class Tag>
		void write(const T& t, Tag tag, rpc::traits::exception_tag)
		{
			std::string what(t.what());
			write(what, tag, rpc::traits::std_container_tag());
		}

		template<class T, class Tag>
		void write(const T& t, Tag tag, rpc::traits::unknown_tag)
		{
			const_cast<T&>(t).serialize(*this, 0);
			//serialize(*this, t);
		}

	private:
		output_type& m_output;

	};
/*
};
*/
template<class Buffer, class T>
bitwise_reader<Buffer>& operator&(bitwise_reader<Buffer>& r, T& t)
{
	r(t, rpc::tags::parameter());
	return r;
}

template<class Buffer, class T>
bitwise_writer<Buffer>& operator&(bitwise_writer<Buffer>& w, const T& t)
{
	w(t, rpc::tags::parameter());
	return w;
}


}}}

#endif
