#ifndef BOOST_RPC_DETAIL_REALLOCATOR_HPP
#define BOOST_RPC_DETAIL_REALLOCATOR_HPP

#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <cstdlib>

namespace boost{ namespace rpc{ namespace detail
{

static std::size_t priv_max_size(std::size_t x, std::size_t y)
{
	return x < y ? y : x;
}

static std::size_t priv_min_size(std::size_t x, std::size_t y)
{
	return x < y ? x : y;
}

template<class Buffer, std::size_t MinIncrease = 64, std::size_t MaxIncrease = 65536>
struct reallocator
{
	BOOST_STATIC_ASSERT(MinIncrease < MaxIncrease);
	static void deallocate(const Buffer& p)
	{
		if(p.first)	std::free(p.first);
	}

	static void reallocate(Buffer& p, std::size_t s)
	{
		const std::size_t cur_size = p.second;
		if(!(cur_size < s)) return;
		const std::size_t new_size = priv_max_size(priv_min_size(cur_size, MaxIncrease),
			priv_max_size(s - cur_size, MinIncrease)) + cur_size;
		void* new_p = (std::realloc(p.first, new_size));
		if(new_p)
		{
			p.first = new_p;
			p.second = new_size;
		}
		else
		{
			boost::throw_exception(std::bad_alloc());
		}
	}
	
};


}}}

#endif
