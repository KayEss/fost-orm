/*
    Copyright 2009, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-metamodel.hpp"
#include <fost/metamodel.hpp>


fostlib::type_descriptor::type_descriptor( const json &j )
: model_type( j ),
    name( this, j ) {
}
