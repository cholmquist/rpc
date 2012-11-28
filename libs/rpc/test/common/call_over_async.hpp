#ifndef CALL_OVER_ASYNC_HPP
#define CALL_OVER_ASYNC_HPP

#include <boost/system/error_code.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>


namespace boost{ namespace rpc{

namespace detail
{
  template<class Buffer>
  struct sync_data
  {
	sync_data() : done(false) {}

	Buffer buffer;
	::boost::condition_variable condition;
	::boost::mutex mutex;
	::boost::system::error_code error_code;
	bool done;
	
	void set(Buffer& data, const ::boost::system::error_code& ec)
	{
	    {
		::boost::lock_guard<boost::mutex> lock(this->mutex);
		this->buffer.swap(data);
		this->error_code = ec;
		done = true;
	    }
	    condition.notify_one();
	}

	void get(Buffer& data, ::boost::system::error_code& ec)
	{
	    ::boost::unique_lock<boost::mutex> lock(this->mutex);
	    while(!done)
	    {
		condition.wait(lock);
	    }
	    data.swap(this->buffer);
	    ec = this->error_code;
	}
  };
}
  
class call_over_async
{
public:
	typedef std::vector<char> buffer_type;

	call_over_async()
	: m_data(::boost::make_shared<sync_data>())
	{}

	void operator()(buffer_type& data, const boost::system::error_code& ec)
	{
	    m_data->set(data, ec);
	}
	
	void get(buffer_type& data, ::boost::system::error_code& ec)
	{
	    m_data->get(data, ec);
	}
	
private:
  typedef detail::sync_data<buffer_type> sync_data;
  
  ::boost::shared_ptr<sync_data> m_data;
};
  
}}

#endif
