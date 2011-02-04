#ifndef BOOST_RPC_DETAIL_FUTURE_HPP_INCLUDED
#define BOOST_RPC_DETAIL_FUTURE_HPP_INCLUDED

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace boost{ namespace rpc { namespace detail{

template <typename T>
class return_value
{
public:
  class exception_holder_base
  {
  public:
    virtual ~exception_holder_base()
    {
    }

    virtual void raise() const = 0;
  };

  template <typename E>
  class exception_holder :
    public exception_holder_base
  {
  public:
    exception_holder(E e)
      : e_(e)
    {
    }

    virtual void raise() const
    {
      throw e_;
    }

  private:
    E e_;
  };

  T get() const
  {
    mutex::scoped_lock lock(mutex_);
    while (!value_.get() && !exception_.get())
      condition_.wait(lock);
    if (exception_.get())
      exception_->raise();
    return *value_;
  }

  void set(T t)
  {
    mutex::scoped_lock lock(mutex_);
    if (value_.get() == 0 && exception_.get() == 0)
    {
      value_.reset(new T(t));
      condition_.notify_all();
    }
  }

  template <typename E>
  void fail(E e)
  {
    mutex::scoped_lock lock(mutex_);
    if (value_.get() == 0 && exception_.get() == 0)
    {
      exception_.reset(new exception_holder<E>(e));
      condition_.notify_all();
    }
  }

private:
  mutable mutex mutex_;
  mutable condition condition_;
  std::auto_ptr<T> value_;
  std::auto_ptr<exception_holder_base> exception_;
};

} // detail

template <typename T>
class future;

template <typename T>
class promise
{
public:
	promise()
		: return_value_(new detail::return_value<T>)
	{
	}

	void operator()(T t) // Fulfil
	{
		return_value_->set(t);
	}

	template <typename E>
	void fail(E e)
	{
		return_value_->fail(e);
	}

private:
	friend class future<T>;
	boost::shared_ptr<detail::return_value<T> > return_value_;
};

template <typename T>
class future
{
private:
public:
	future(promise<T>& p)
		: return_value_(p.return_value_)
	{
	}

	T operator()() const // Await
	{
		return return_value_->get();
	}

private:
	boost::shared_ptr<detail::return_value<T> > return_value_;
};

}}

#endif
