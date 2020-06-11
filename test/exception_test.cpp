// Copyright (c) 2018-2020 Emil Dotchevski and Reverge Studios, Inc.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/leaf/config.hpp>
#ifdef LEAF_NO_EXCEPTIONS

#include <iostream>

int main()
{
	std::cout << "Unit test not applicable." << std::endl;
	return 0;
}

#else

#include <boost/leaf/handle_exception.hpp>
#include <boost/leaf/exception.hpp>
#include <boost/leaf/on_error.hpp>
#include "lightweight_test.hpp"

namespace leaf = boost::leaf;

struct info { int value; };
struct extra_info { int value; };

struct my_exception: std::exception { };

struct extra_info_exception:
	std::exception
{
	extra_info_exception() noexcept
	{
		leaf::current_error().load(extra_info{42});
	}
};

template <class F>
int test( F && f )
{
	return leaf::try_catch(
		[&]() -> int
		{
			f();
			return 0;
		},

		[]( leaf::catch_<my_exception>, leaf::match<info,42>, leaf::e_source_location )
		{
			return 20;
		},
		[]( leaf::catch_<my_exception>, leaf::match<info,42>, info x )
		{
			return 21;
		},
		[]( leaf::catch_<my_exception>, leaf::e_source_location )
		{
			return 22;
		},
		[]( leaf::catch_<my_exception> )
		{
			return 23;
		},
		[]( leaf::match<info,42>, leaf::e_source_location )
		{
			return 40;
		},
		[]( leaf::match<info,42>, info x )
		{
			return 41;
		},
		[]( leaf::e_source_location )
		{
			return 42;
		},
		[]
		{
			return 43;
		} );
}

int main()
{
	BOOST_TEST_EQ(20, test([]{ LEAF_THROW(my_exception{}, info{42}); }));
	BOOST_TEST_EQ(20, test([]{ throw LEAF_EXCEPTION(my_exception{}, info{42}); }));
	BOOST_TEST_EQ(21, test([]{ throw leaf::exception(my_exception{}, info{42}); }));
	BOOST_TEST_EQ(22, test([]{ LEAF_THROW(my_exception{}); }));
	BOOST_TEST_EQ(22, test([]{ throw LEAF_EXCEPTION(my_exception{}); }));
	BOOST_TEST_EQ(23, test([]{ throw leaf::exception(my_exception{}); }));

	BOOST_TEST_EQ(40, test([]{ LEAF_THROW(info{42}); }));
	BOOST_TEST_EQ(40, test([]{ throw LEAF_EXCEPTION(info{42}); }));
	BOOST_TEST_EQ(41, test([]{ throw leaf::exception(info{42}); }));
	BOOST_TEST_EQ(42, test([]{ LEAF_THROW(); }));
	BOOST_TEST_EQ(42, test([]{ throw LEAF_EXCEPTION(); }));
	BOOST_TEST_EQ(43, test([]{ throw leaf::exception(); }));

	{
		char const * wh = 0;
		leaf::try_catch(
			[]
			{
				throw std::runtime_error("Test");
			},
			[&]( leaf::catch_<std::exception> ex )
			{
				wh = ex.value().what();
			} );
		BOOST_TEST(wh!=0 || !strcmp(wh,"Test"));
	}

	{
		int const id = leaf::leaf_detail::current_id();
		BOOST_TEST_EQ( 21, test( []
		{
			auto load = leaf::on_error(info{42});
			throw my_exception();
		} ) );
		BOOST_TEST_NE(id, leaf::leaf_detail::current_id());
	}

	{
		BOOST_TEST_EQ( 23, test( []
		{
			int const id = leaf::leaf_detail::current_id();
			try
			{
				leaf::try_catch(
					[]
					{
						throw my_exception();
					} );
			}
			catch(...)
			{
				BOOST_TEST_EQ(id, leaf::leaf_detail::current_id());
				throw;
			}
		} ) );
	}

	return boost::report_errors();
}

#endif
