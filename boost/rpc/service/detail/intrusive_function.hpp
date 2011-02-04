#ifndef BOOST_RPC_DETAIL_INTRUSIVE_FUNCTION_HPP
#define BOOST_RPC_DETAIL_INTRUSIVE_FUNCTION_HPP

#include <boost/utility/typed_in_place_factory.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <memory>
#include <set>

namespace boost{ namespace rpc{ namespace detail{

template<class Key, class Allocator, class IntrusiveHook>
struct intrusive_function
{

	class factory_base;

	class base : public IntrusiveHook
	{
		friend class factory_base;
	public:
		typedef void (*destroy_func_type)(base*, Allocator&);

		Key first;
		base(const Key& key, destroy_func_type d) :
		first(key), destroy_(d) {}

		const Key& key() const { return first; }

	private:
		base( const base& );
		const base& operator=( const base& );

		destroy_func_type destroy_;
	};

	template<class Signature>
	class callable;

	template<class R>
	class callable<R()> : public base
	{
	protected:
		typedef R (*invoke_func_type)(const base*);
		invoke_func_type invoke_;
	public:
		callable(const Key& key, destroy_func_type d, invoke_func_type i) :
		  base(key, d), invoke_(i) {}

		  typedef R result_type;

		  R operator()() const { return invoke_(this); }

		  template<class Function>
		  class function : public callable<R()>
		  {
			  typedef function<Function> this_type;

			  function(const Key& key, const Function& h) :
			  callable<R()>(key, &this_type::destroy_explicit, &this_type::invoke), handler_(h) {}

			  Function handler_;

			  static R invoke(const base* h)
			  {
				  this_type* _this = const_cast<this_type*>(static_cast<const this_type*>(h));
				  return _this->handler_();
			  }

			  static void destroy(base *b, Allocator& alloc)
			  {
				  this_type* _this = static_cast<this_type*>(b);
				  _this->~this_type();
				  alloc.deallocate(static_cast<char *>(static_cast<void*>(_this)), sizeof(this_type));

			  }

		  };

	};

	template<class R, class A0>
	class callable<R(A0)> : public base
	{
	protected:
		typedef R (*invoke_func_type)(const base*, A0);
		invoke_func_type invoke_;
	public:
		callable(const Key& key, destroy_func_type d, invoke_func_type i) :
		  base(key, d), invoke_(i) {}

		  R operator()(A0 a0) const { invoke_(this, a0); }

		  template<class Function>
		  class function : public callable<R(A0)>
		  {
			  typedef function<Function> this_type;

		  public:
			  function(const Key& key, const Function& h) :
				callable<R(A0)>(key, &this_type::destroy, &this_type::invoke), handler_(h) {}

		  private:
			  Function handler_;
			  static R invoke(const base* h, A0 a0)
			  {
				  this_type* _this = const_cast<this_type*>(static_cast<const this_type*>(h));
				  return _this->handler_(a0);
			  }

			  static void destroy(base *b, Allocator& alloc)
			  {
				  this_type* _this = static_cast<this_type*>(b);
				  _this->~this_type();
				  alloc.deallocate(static_cast<char *>(static_cast<void*>(_this)), sizeof(this_type));

			  }

		  };

		  template<class Function>
		  class function_in_place : public callable<R(A0)>
		  {
			  typedef function_in_place<Function> this_type;

		  public:
			  template<class Factory>
			  function_in_place(const Key& key, const Factory& f) :
			  callable<R(A0)>(key, &this_type::destroy, &this_type::invoke),
				  handler_(static_cast<Function*>(f.apply(static_cast<void*>(storage_)))) { }

		  private:
			  Function* handler_;
			  char storage_[sizeof(Function)];

			  static R invoke(const base* h, A0 a0)
			  {
				  this_type* _this = const_cast<this_type*>(static_cast<const this_type*>(h));
				  return _this->handler_->operator()(a0);
			  }
			  static void destroy(base* h, Allocator& a)
			  {
				  this_type* _this = static_cast<this_type*>(h);
				  _this->handler_->~Function();
				  _this->~this_type();
				  a.deallocate(static_cast<char *>(static_cast<void*>(_this)), sizeof(this_type));
			  }
		  };
	};

	class factory_base
	{
	public:
		Allocator allocator;

		factory_base() { }
		factory_base(const Allocator& a) : allocator(a) { }

		void destroy(base *b)
		{
			b->destroy_(b, allocator);
		}
	};

	template<class Callable>
	struct factory : public factory_base
	{
		factory() { }
		factory(const Allocator& a) : factory_base(a) { }

		template<class FunctionOrFactory>
		Callable* construct(const Key& key, const FunctionOrFactory& f)
		{
			return _construct(key, f,
				boost::is_base_and_derived<boost::typed_in_place_factory_base, FunctionOrFactory>::type());
		}
	private:
		template<class Function>
		Callable* _construct(const Key& key, const Function& h, boost::false_type)
		{
			typedef typename Callable::function<Function> handler_type;
			return new(static_cast<void *>(allocator.allocate(sizeof(handler_type))))
				handler_type(key, h);
		}

		template<class TypedInFactory>
		Callable* _construct(const Key& key, const TypedInFactory& f, boost::true_type)
		{
			typedef typename Callable::function_in_place<typename TypedInFactory::value_type> handler_type;
			return new(static_cast<void *>( allocator.allocate(sizeof(handler_type)) )) handler_type(key, f);
		}
	};


};

}}}

#endif
