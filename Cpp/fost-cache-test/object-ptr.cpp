/*
    Copyright 2009, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-cache-test.hpp"
#include <fost/cache>
#include <fost/factory>


namespace {
    class Model : public fostlib::model< Model > {
    public:
        Model( const fostlib::json &j )
        : model_type( j ) {
        }

        typedef int key_type;
        FSL_ATTRIBUTE_PK( pk, int );
    };
    const fostlib::factory< Model > s_Model;
}


FSL_TEST_SUITE( object_ptr );


FSL_TEST_FUNCTION( basic_operations ) {
    fostlib::test::default_copy_constructable< fostlib::object_ptr< Model > >();
    fostlib::test::default_isnull< fostlib::object_ptr< Model > >();
}
