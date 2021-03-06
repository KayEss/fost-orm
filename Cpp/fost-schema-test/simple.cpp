/*
    Copyright 2008-2015, Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-schema-test.hpp"
#include <fost/db>
#include <fost/exception/null.hpp>


using namespace fostlib;


FSL_TEST_SUITE( simple_schema );


FSL_TEST_FUNCTION( dynamic ) {
    meta_instance simple( L"simple" );
    FSL_CHECK_EQ( simple.name(), L"simple" );

    simple
        .primary_key( L"id", L"integer" )
        .field( L"name", L"varchar", true, 100 )
        .field( L"description", L"text", false );
    FSL_CHECK_EXCEPTION( simple.field( L"name", L"text", false ), exceptions::null& );

    FSL_CHECK_EQ( simple[ L"id" ].type().type_name(), L"integer" );
    FSL_CHECK_EQ( simple[ L"name" ].type().type_name(), L"varchar" );
    FSL_CHECK_EQ( simple[ L"description" ].type().type_name(), L"text" );

    FSL_CHECK_EXCEPTION( simple[ L"xx" ], exceptions::null& );
}


FSL_TEST_FUNCTION( dynamic_construct_blank ) {
    dbconnection dbc( L"master", L"master" );
    meta_instance simple( L"simple" );
    simple
        .primary_key( L"id", L"integer" )
        .field( L"name", L"varchar", true, 10 );
    auto i = simple.create();
    FSL_CHECK( !i->in_database() );
    FSL_CHECK_EQ( (*i)[ L"id" ]._meta().type().type_name(), L"integer" );
    FSL_CHECK_EQ( (*i)[ L"name" ]._meta().type().type_name(), L"varchar" );
    FSL_CHECK( (*i)[ L"id" ]._meta().required() );
    FSL_CHECK_EQ( (*i)[ L"id" ].to_json(), json( 0 ) );
    FSL_CHECK( (*i)[ L"name" ]._meta().required() );
    FSL_CHECK_EQ( (*i)[ L"name" ].to_json(), json( string() ) );
}


FSL_TEST_FUNCTION( dynamic_construct_json ) {
    dbconnection dbc( L"master", L"master" );
    meta_instance simple( L"simple" );
    simple
        .primary_key( L"id", L"integer" )
        .field( L"name", L"varchar", true, 10 );
    auto i = simple.create( json::parse(
        L"{ \"id\": 123, \"name\": \"A simple name\" }"
    ) );
    FSL_CHECK_EQ( (*i)[ L"id" ].to_json(), json( 123 ) );
    FSL_CHECK_EQ( (*i)[ L"name" ].to_json(), json( L"A simple name" ) );
}


FSL_TEST_FUNCTION( dynamic_enclosure ) {
    enclosure ns( L"ns" );
    meta_instance simple( ns, L"simple" );
    FSL_CHECK_EQ( simple.name(), L"simple" );
    FSL_CHECK_EQ( simple.parent().name(), L"ns" );
    FSL_CHECK_EQ( simple.fq_name(), L"ns.simple" );
    FSL_CHECK_EQ( simple.fq_name( L"_" ), L"ns_simple" );

    FSL_CHECK( !simple.in_global() );
    FSL_CHECK( simple.parent().in_global() );
}


FSL_TEST_FUNCTION( model_as_attribute ) {
    meta_instance simple( L"simple" ), ref1("ref1");
    /*
        Create a simple object definition
    */
    simple
        .primary_key(L"id", L"integer")
        .field(L"name", L"varchar", true, 10)
    ;
    // Add a non-nullable reference to a simple object
    ref1
        .primary_key(L"id", L"integer")
        .field(L"simple", simple, false)
    ;
}
