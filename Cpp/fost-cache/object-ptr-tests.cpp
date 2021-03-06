/*
    Copyright 2009-2010, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include <fost/test>
#include <fost/cache>
#include <fost/factory>


namespace {
    class Model : public fostlib::model< Model > {
    public:
        Model( const fostlib::initialiser &init )
        : model_type( init ),
            pk( this, init.data() ) {
        }

        typedef int key_type;
        FSL_ATTRIBUTE_PK( pk, int );
    };
    const fostlib::factory< Model > s_Model;
}


FSL_TEST_SUITE( object_ptr );


FSL_TEST_FUNCTION( basic_operations ) {
    fostlib::test::default_copy_constructable
        < fostlib::object_ptr< Model > >();
    fostlib::test::default_isnull< fostlib::object_ptr< Model > >();
}


FSL_TEST_FUNCTION( pointer_ops_when_null ) {
    fostlib::object_ptr< Model > ptr;
    const fostlib::object_ptr< Model > cptr;
    FSL_CHECK_EXCEPTION(ptr->pk(), fostlib::exceptions::null&);
    FSL_CHECK_EXCEPTION(cptr->pk(), fostlib::exceptions::null&);
    FSL_CHECK_EXCEPTION(ptr.operator -> (), fostlib::exceptions::null&);
    FSL_CHECK_EXCEPTION(cptr.operator -> (), fostlib::exceptions::null&);
    FSL_CHECK_EXCEPTION(ptr.operator * (), fostlib::exceptions::null&);
    FSL_CHECK_EXCEPTION(cptr.operator * (), fostlib::exceptions::null&);
}
