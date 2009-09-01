/*
    Copyright 2008-2009, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-schema-test.hpp"
#include <fost/db>

#include <fost/exception/out_of_range.hpp>


using namespace fostlib;


FSL_TEST_SUITE( fields );


FSL_TEST_FUNCTION( define ) {
    FSL_CHECK_NOTHROW( const field_wrapper< int64_t > i( L"int64" ) );
}


FSL_TEST_FUNCTION( registry ) {
    FSL_CHECK_EQ( field_base::fetch( L"integer" ).type_name(), L"integer" );
    // zzzz is not a valid type. Should throw an exception
    FSL_CHECK_EXCEPTION( field_base::fetch( L"zzzz" ), exceptions::out_of_range< std::size_t >& );
    {
        // Registering a second instance of an extant type should make it no longer usable
        const field_wrapper< int64_t > i( L"integer" );
        FSL_CHECK_EXCEPTION( field_base::fetch( L"integer" ), exceptions::out_of_range< std::size_t >& );
    }
    // Once the new instance goes out of scope then the type becomes usable again
    FSL_CHECK_EQ( field_base::fetch( L"integer" ).type_name(), L"integer" );
}
