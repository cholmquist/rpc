#include "char_prototocol.hpp"
#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/async_local.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace rpc = boost::rpc;

int main()
{
	rpc::async_remote<vec_protocol> async_remote;
	async_remote(char_char, call())(1);
	async_remote(char_char, call(), empty_handler())(1);
	return boost::report_errors();
}
