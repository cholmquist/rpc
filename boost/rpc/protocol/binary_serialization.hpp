// (C) Copyright 2005-2007 Matthias Troyer
// (C) Copyright 2011 Christian Holmquist

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer

#ifndef BOOST_RPC_BINARY_SERIALIZATION
#define BOOST_RPC_BINARY_SERIALIZATION

#include <boost/config.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/shared_ptr_helper.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/always.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <vector>

namespace boost { namespace rpc {

template<class Buffer = std::vector<char> >
class binary_buffer_oprimitive
{
public:
	typedef Buffer buffer_type;

	binary_buffer_oprimitive(buffer_type & b)
		: buffer_(b)
	{
	}

	void const * address() const
	{
		return &buffer_.front();
	}

	const std::size_t& size() const
	{
		return size_ = buffer_.size();
	}

	void save_binary(void const *address, std::size_t count)
	{
		save_impl(address,count);
	}

	// fast saving of arrays
	template<class T>
	void save_array(serialization::array<T> const& x, unsigned int /* file_version */)
	{

		BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
		if (x.count())
			save_impl(x.address(), x.count()*sizeof(T));
	}

	template<class T>
	void save(serialization::array<T> const& x)
	{
		save_array(x,0u);
	}

	typedef serialization::is_bitwise_serializable<mpl::_1> use_array_optimization;

	// default saving of primitives.
	template<class T>
	void save(const T & t)
	{
		BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
		save_impl(&t, sizeof(T));
	}

	template<class CharType>
	void save(const std::basic_string<CharType> &s)
	{
		unsigned int l = static_cast<unsigned int>(s.size());
		save(l);
		save_impl(s.data(),s.size());
	}

private:

	void save_impl(void const * p, int l)
	{
		char const* ptr = reinterpret_cast<char const*>(p);
		buffer_.insert(buffer_.end(),ptr,ptr+l);
	}

	buffer_type& buffer_;
	mutable std::size_t size_;
};

template<class Buffer = std::vector<char> >
class packed_oarchive
  : public binary_buffer_oprimitive<Buffer>
  , public archive::detail::common_oarchive<packed_oarchive<Buffer> >
  , public archive::detail::shared_ptr_helper
{
public:
  /**
   *  Construct a @c packed_oarchive for transmission over the given
   *  MPI communicator and with an initial buffer.
   *
   *  @param comm The communicator over which this archive will be
   *  sent.
   *
   *  @param b A user-defined buffer that will be filled with the
   *  binary representation of serialized objects.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   *
   *  @param position Set the offset into buffer @p b at which
   *  deserialization will begin.
   */
   
  packed_oarchive(buffer_type & b, unsigned int flags = boost::archive::no_header)
         : oprimitive<Buffer>(b),
           archive::detail::common_oarchive<packed_oarchive>(flags)
        {}

  /**
   *  Construct a @c packed_oarchive for transmission over the given
   *  MPI communicator.
   *
   *  @param comm The communicator over which this archive will be
   *  sent.
   *
   *  @param s The size of the buffer to be received.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   */
   
  // Save everything else in the usual way, forwarding on to the Base class
  template<class T>
  void save_override(T const& x, int version, mpl::false_)
  {
    archive::detail::common_oarchive<packed_oarchive>::save_override(x,version);
  }

  // Save it directly using the primitives
  template<class T>
  void save_override(T const& x, int /*version*/, mpl::true_)
  {
    oprimitive::save(x);
  }

  // Save all supported datatypes directly
  template<class T>
  void save_override(T const& x, int version)
  {
    typedef typename mpl::apply1<use_array_optimization,T>::type use_optimized;
    save_override(x, version, use_optimized());
  }

  // input archives need to ignore  the optional information 
  void save_override(const archive::class_id_optional_type & /*t*/, int){}

  // explicitly convert to char * to avoid compile ambiguities
  void save_override(const archive::class_name_type & t, int){
      const std::string s(t);
      * this->This() << s;
  }

private:
  /// An internal buffer to be used when the user does not supply his
  /// own buffer.
  buffer_type internal_buffer_;
};

template<class Buffer = std::vector<char > >
class binary_buffer_iprimitive
{
public:
	/// the type of the buffer from which the data is unpacked upon deserialization
	typedef Buffer buffer_type;

	binary_buffer_iprimitive(buffer_type & b, int position = 0)
		: buffer_(b),
		position(position)
	{
	}

	void* address ()
	{
		return &buffer_.front();
	}

	void const* address () const
	{
		return &buffer_.front();
	}

	const std::size_t& size() const
	{
		return size_ = buffer_.size();
	}

	void resize(std::size_t s)
	{
		buffer_.resize(s);
	}

	void load_binary(void *address, std::size_t count)
	{
		load_impl(address,count);
	}

	// fast saving of arrays of fundamental types
	template<class T>
	void load_array(serialization::array<T> const& x, unsigned int /* file_version */)
	{
		BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
		if (x.count())
			load_impl(x.address(), sizeof(T)*x.count());
	}

	typedef serialization::is_bitwise_serializable<mpl::_1> use_array_optimization;

	template<class T>
	void load(serialization::array<T> const& x)
	{
		load_array(x,0u);
	}

	// default saving of primitives.
	template<class T>
	void load( T & t)
	{
		BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
		load_impl(&t, sizeof(T));
	}

	template<class CharType>
	void load(std::basic_string<CharType> & s)
	{
		unsigned int l;
		load(l);
		// borland de-allocator fixup
#if BOOST_WORKAROUND(_RWSTD_VER, BOOST_TESTED_AT(20101))
		if(NULL != s.data())
#endif
			s.resize(l);
		// note breaking a rule here - could be a problem on some platform
		load_impl(const_cast<char *>(s.data()),l);
	}

private:

	void load_impl(void * p, int l)
	{
		assert(position+l<=static_cast<int>(buffer_.size()));
		if (l)
			std::memcpy(p,&buffer_[position],l);
		position += l;
	}

	buffer_type & buffer_;
	mutable std::size_t size_;
	int position;
};



} }

#endif // BOOST_RPC_BINARY_SERIALIZATION
