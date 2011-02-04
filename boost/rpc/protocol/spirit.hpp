/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_PROTOCOL_SPIRIT_HPP
#define BOOST_RPC_PROTOCOL_SPIRIT_HPP

#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/qi/numeric.hpp>
#include <boost/spirit/home/karma/generate.hpp>
#include <boost/spirit/home/karma/numeric.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/throw_exception.hpp>

namespace boost{ namespace rpc{ namespace protocol
{

struct spirit_parse_error : std::exception
{
	virtual void const char* what() const
	{
		return "spirit parse error";
	}
};

struct spirit_generate_error : std::exception
{
	virtual void const char* what() const
	{
		return "spirit generate error"
	}
};

class spirit
{
public:
	typedef std::vector<char> input;
	typedef std::vector<char> output;

	spirit()
	{
	}

	struct reader
	{
		reader(spirit, input& in)
			: m_input(in)
			, m_input_itr(in.begin())
		{
		}

		template<class T>
		typename enable_if<boost::is_integral<T> >::type 
		operator()(T& t)
		{
			this->parse(::boost::spirit::qi::int_parser<T>(), t);
		}

		template<class Expr, class Attr>
		void parse(const Expr& expr, Attr& attr)
		{
			input::iterator end = m_input.end();
			if(m_input_itr != m_input.end())
			{
				if(!::boost::spirit::qi::parse(m_input_itr, m_input.end(), expr, attr))
				{
					throw_exception(std::exception());
				}
			}
			else
			{
				throw_exception(std::exception());
			}
		}

		input& m_input;
		input::iterator m_input_itr;


	};

	struct writer
	{
		writer(spirit, output& out)
			: m_output(out)
		{
		}
		template<class T>
		typename enable_if<boost::is_integral<T> >::type 
		operator()(const T& t)
		{
			this->generate(::boost::spirit::karma::int_, t);
		}

		template <typename Expr, typename Attr>
		void generate(Expr const& expr, Attr const& attr)
		{
			if(!::boost::spirit::karma::generate(std::back_inserter(m_output), expr, attr))
			{
			}
		}


		output& m_output;
	};

};

}}}

#endif
