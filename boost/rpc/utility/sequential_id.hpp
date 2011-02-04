#ifndef BOOST_RPC_UTILITY_SEQUENTIAL_HPP
#define BOOST_RPC_UTILITY_SEQUENTIAL_HPP

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/throw_exception.hpp>
#include <boost/checked_delete.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <limits>
#include <deque>

namespace boost{ namespace rpc{

namespace detail
{
	template<class T>
	const char* char_cast(const T* p)
	{
		return static_cast<const char*>(static_cast<const void*>(p));
	}
	template<class T>
	char* char_cast(T* p)
	{
		return static_cast<char*>(static_cast<void*>(p));
	}
}

template<class ID, class Generator>
class sequence_id
{
	typedef typename Generator::impl generator_impl;
	struct id_deallocator
	{
		id_deallocator()
		{
		}

		id_deallocator(boost::weak_ptr<generator_impl> gen)
			: generator(gen)
		{
		}
		void operator()(ID* pid)
		{
			ID id = *pid;
			boost::checked_delete(pid);
			if(boost::shared_ptr<generator_impl> gen = generator.lock())
			{
				gen->deallocate_id(id);
			}
		}

		boost::weak_ptr<generator_impl> generator;
	};
public:
	typedef ID value_type;
	typedef Generator generator_type;

	static const std::size_t static_size = sizeof(ID);

	sequence_id()
	{
	}

	explicit sequence_id(value_type value)
		: m_id(boost::shared_ptr<value_type>(new value_type(value), id_deallocator(boost::weak_ptr<generator_impl>())))
	{
	}

	sequence_id(value_type id, boost::weak_ptr<generator_impl> gen)
		: m_id(boost::shared_ptr<value_type>(new value_type(id), id_deallocator(gen)))
	{
	}

	value_type value() const
	{
		value_type* id_ptr = m_id.get();
		return id_ptr ? *id_ptr : value_type();
	}

	generator_type get_generator() const
	{
		id_deallocator* deleter = boost::get_deleter<id_deallocator>(m_id);
		return deleter ? generator_type(deleter->generator) : generator_type();
	}

/*	operator bool() const
	{
		return m_id.get() != 0;
	}*/

	char* data()
	{
		ID* p = m_id.get();
		if(p == 0)
		{
			m_id.reset(new ID(), id_deallocator());
			p = m_id.get();
		}
		return detail::char_cast(p);
	}

	const char* data() const
	{
		static const ID empty = ID();
		const ID* p = m_id.get();
		if(p == 0)
		{
			p = &empty;
		}
		return detail::char_cast(p);
	}

	bool operator<(const sequence_id& id) const
	{
		return value() < id.value();
	}

	bool operator==(const sequence_id& id) const
	{
		return value() == id.value();
	}

private:
	boost::shared_ptr<value_type> m_id;
};

template<class ID>
class id_generator
{
public:
	struct impl
	{
		impl()
			: m_id_sequence()
		{
		}
		ID allocate_id()
		{
			ID id;
			if(m_id_pool.empty())
			{
				if(m_id_sequence != std::numeric_limits<ID>::max())
				{
					id = ++m_id_sequence;
				}
				else
				{
					boost::throw_exception(std::out_of_range("id_generator<>"));
				}
			}
			else
			{
				id = m_id_pool.front();
				m_id_pool.pop_front();
			}
			return id;
		}

		void deallocate_id(ID id)
		{
			m_id_pool.push_back(id);
		}

	private:
		ID m_id_sequence;
		std::deque<ID> m_id_pool;
	};

	typedef sequence_id<ID, id_generator> id_type;
	id_generator()
		: m_impl(boost::make_shared<impl>())
	{
	}

	explicit id_generator(boost::weak_ptr<impl> pimpl)
		: m_impl(pimpl)
	{
	}

	id_type operator()()
	{
		ID id = m_impl->allocate_id();
		BOOST_TRY
		{
			return id_type(id, m_impl);
		}
		BOOST_CATCH(...)
		{
			m_impl->deallocate_id(id);
			BOOST_RETHROW;
		}
		BOOST_CATCH_END
	};

	void reset()
	{
		//m_impl = boost::make_shared<impl>();
	}

	bool operator==(id_generator gen) const
	{
		return m_impl == gen.m_impl;
	}

	bool operator!=(id_generator gen) const
	{
		return m_impl != gen.m_impl;
	}

private:
	boost::shared_ptr<impl> m_impl;
};

}}

#endif
