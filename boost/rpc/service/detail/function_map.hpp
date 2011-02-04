#ifndef BOOST_RPC_FUNCTION_MAP_HPP
#define BOOST_RPC_FUNCTION_MAP_HPP

#include <boost/rpc/service/detail/intrusive_function.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/set_hook.hpp>

namespace boost{ namespace rpc{

template<class Key, class Signature, class Allocator = std::allocator<void> >
class function_map
{
	typedef typename Allocator::template rebind<char>::other allocator_type;

	typedef detail::intrusive_function<Key, allocator_type, intrusive::set_base_hook<> > function_base;
	typedef typename function_base::callable<Signature> abstract_handler;
	typedef typename function_base::factory<abstract_handler> factory;

	struct destroy
	{
		factory& factory_;
		destroy(factory& f) : factory_(f) { }
		void operator()(abstract_handler *h) const { factory_.destroy(h); }
	};

	struct compare
	{
		bool operator()(const Key& key, const abstract_handler& h) const
		{
			return key < h.first;
		}

		bool operator()(const abstract_handler& h, const Key& key) const
		{
			return h.first < key;
		}
	};

	typedef typename intrusive::make_set<abstract_handler>::type base_set;

	base_set set_;
	compare compare_;

	factory factory_;
public:
	function_map() { }
	~function_map() { clear(); }

	typedef abstract_handler function;

	typedef typename base_set::iterator iterator;
	typedef typename base_set::const_iterator const_iterator;

	iterator begin() { return set_.begin(); }
	iterator end() { return set_.end(); }

	const_iterator begin() const { return set_.begin(); }
	const_iterator end() const { return set_.end(); }

	void clear() { set_.clear_and_dispose(destroy(factory_)); }
	bool empty() const { return set_.empty(); }
	iterator find(const Key& key) { return set_.find(key, compare_); }
	iterator erase(iterator i) { return set_.erase_and_dispose(i, destroy(factory_)); }

	template<class Handler>
	std::pair<iterator, bool> insert(const Key& key, const Handler& h)
	{
		base_set::insert_commit_data commit;
		std::pair<iterator, bool> result = set_.insert_check(key, compare_, commit);
		if(result.second)
		{
			abstract_handler* base = factory_.construct(key, h);
			result.first = set_.insert_commit(*base, commit);
		}
		return result;
	}
};

}}

#endif
