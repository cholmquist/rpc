#ifndef BOOST_RPC_TAGS_HPP
#define BOOST_RPC_TAGS_HPP

namespace boost{ namespace rpc{ namespace tags
{
	struct default_ {};
	struct parameter : default_ {};
	struct result : public default_ {};
	struct exception : public default_ {};
	struct placeholder : public default_ {};
}}}

#endif
