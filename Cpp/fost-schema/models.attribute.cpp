/*
    Copyright 1999-2010, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-schema.hpp"
#include <fost/schema.hpp>


using namespace fostlib;


/*
    fostlib::meta_attribute
*/


fostlib::meta_attribute::meta_attribute(
    const string &name, const field_base &type, bool key, bool not_null,
    const nullable< std::size_t > &size, const nullable< std::size_t > &precision
) : name( name ), key( key ), required( not_null ),
        size( size ), precision( precision ), m_type( type ) {
}

fostlib::meta_attribute::~meta_attribute() {}

const field_base &meta_attribute::type() const {
    return m_type;
}


/*
    fostlib::attribute_base
*/

fostlib::attribute_base::attribute_base() {
}

