#include <boost/array.hpp>
#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/local.hpp>
#include <boost/rpc/core/placeholder.hpp>
#include <boost/rpc/throws.hpp>
#include <boost/function/function2.hpp>
//#include <boost/bind/arg.hpp>
#include <iostream>
#include <string>
#include <map>
#include <cmath>

namespace rpc = boost::rpc;
//boost::arg
typedef std::map<std::string, boost::function<void (std::istream&, std::ostream&)> > function_map;


template<class Signature, class ExceptionHandler = rpc::no_exception_handler>
struct console_signature : public rpc::signature<std::string, Signature, ExceptionHandler>
{
	console_signature(const std::string& name)
		: rpc::signature<std::string, Signature, ExceptionHandler>(name)
	{}

};

struct my_protocol
{
	typedef std::istream input;
	typedef std::ostream output;

	struct reader
	{
		std::istream& is;
		reader(my_protocol, std::istream& is)
			: is(is)
		{}

		template<class T, class Tag>
		void operator()(T& t, Tag) const
		{
			if(is.good())
			{
				is >> t;
				if(!is.good())
				{
//					int c = 0;
				}
			}
		}

	};

	struct writer
	{
		std::ostream& os_;
		writer(my_protocol, std::ostream& os) : os_(os) {}

		template<class T>
		void operator()(const T& t, rpc::tags::parameter) const
		{
			os_ << t << " ";
		}

		template<class T>
		void operator()(const T& t, rpc::tags::result) const
		{
			os_ << t << " ";
		}

		template<class E>
		void operator()(const E& e, rpc::tags::exception) const
		{
			os_ << e.what() << " ";
		}

		std::ostream& operator()(rpc::tags::placeholder) const
		{
			return os_;
		}
	};

};

namespace api
{
	console_signature<void()> quit("quit");

	console_signature<void(int)> seed("seed");

	console_signature<int()> rand("rand");

	console_signature<std::string(std::string&)> hello("hello");

	console_signature<int(int, int)> sum_ints("sum_ints");

	console_signature<double(double, double)> sum("sum");

	console_signature<float(float), rpc::throws<std::exception> > sqrt("sqrt");

	// The help function does pretty printing directly to the std::ostream object, to avoid
	// unnecessary copies of the help string.
	console_signature<void()> help("help");

//	boost::rpc::function<void(), const char*> new_quit = {"quit"};

	// This version of sum_doubles will generate an int as a result,
	// regardless of the implementation generating a double.
	// The signature always decides the outcome of the operation
//	console_signature<int(double, double)> sum_doubles("sum_doubles");
}

namespace impl
{
	bool g_run = true;

	void quit()
	{
		g_run = false;
		//throw int(25);
		//throw std::exception("tjo");
	}
	int rand()
	{
		return std::rand();
	}

	void seed(int i)
	{
		return std::srand(i);
	}

	std::string hello(std::string& world)
	{
		world = "world";
		return "hello";
	}

	float sqrt(float f)
	{
		if(f < 0.0f)
		{
			throw std::runtime_error("sqrt exception");
		}
		return std::sqrt(f);
	}



	struct sum
	{
		template<class T>
		struct result;

		template<class T>
		struct result<sum(T, T)>
		{
			typedef typename boost::remove_reference<T>::type type;
		};

		template<class T>
		T operator()(const T& x, const T& y) const
		{
			return x + y;
		}

	};

	struct help
	{
		typedef void result_type;
		const function_map& functions;
		help(const function_map& functions) : functions(functions) {}
		void operator()(std::ostream& os)
		{
			function_map::const_iterator i = functions.begin();
			function_map::const_iterator e = functions.end();
			for(;i != e; ++i)
			{
				os << i->first <<std::endl;
			}
		}
	};
}

//rpc::placeholder<std::istream&> _istream = {};
const rpc::placeholder<std::ostream&> _ostream = {};

/*template<class T>
struct test
{
	typename boost::enable_if<boost::is_void<T> >::type
	operator()()
	{
		std::cout << "is void" <<std::endl;
	}
	typename boost::disable_if<boost::is_void<T> >::type
	operator()()
	{
		std::cout << "is not void" <<std::endl;
	}
};*/


int main()
{
	function_map fmap;
	rpc::local<my_protocol> local;
	fmap.insert(local(api::sum, impl::sum()));
	fmap.insert(local(api::quit, &impl::quit));
	fmap.insert(local(api::sqrt, &impl::sqrt));
	_ostream = _1;
	fmap.insert(local(api::help, impl::help(fmap), rpc::placeholders(_ostream = _1)));
//	fmap.insert(local(api::help, impl::help(fmap), rpc::placeholders(_ostream = _1, _ostream = _2)));

/*	add_function(fmap, api::sum_doubles, impl::sum());
	add_function(fmap, api::quit, &impl::quit);
	add_function(fmap, api::rand, &impl::rand);
	add_function(fmap, api::seed, &impl::seed);
	add_function(fmap, api::hello, &impl::hello);
	add_function(fmap, api::sum_ints, std::plus<int>());
*/

	while(impl::g_run)
	{
		std::string id;
		std::cin >> id;
		if(!id.empty())
		{
			function_map::iterator f = fmap.find(id);
			if(f != fmap.end())
			{
				f->second(std::cin, std::cout);
				std::cout << std::endl;
			}
			else
			{
				std::cout << "Unknown function " << id <<std::endl;
			}
		}
	}

	return 0;
}

