/*
    Copyright 1999-2016, Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-schema.hpp"
#include <fost/factory.hpp>
#include <fost/threading>

#include <fost/exception/out_of_range.hpp>


using namespace fostlib;


namespace {
    typedef threadsafe_store< detail::factory_base * > factory_registry_type;
    factory_registry_type &g_registry() {
        static factory_registry_type registry;
        return registry;
    }
    void add_register( detail::factory_base *factory, const std::type_info &t, const nullable< string > &name ) {
        g_registry().add( string( t.name() ), factory );
        if ( name ) g_registry().add( name.value(), factory );
    }
}


fostlib::detail::factory_base::factory_base( const enclosure &enc, const std::type_info &t, const nullable< string > &name )
: m_type( t ), m_container( &enc ), m_name( name ) {
    add_register( this, t, name );
}
fostlib::detail::factory_base::factory_base( const factory_base &enc, const std::type_info &t, const nullable< string > &name )
: m_type( t ), m_container( &enc ) {
    add_register( this, t, name );
}

fostlib::detail::factory_base::~factory_base() {
    g_registry().remove( string( m_type.name() ), this );
    if ( m_name ) g_registry().remove( m_name.value(), this );
}

string fostlib::detail::factory_base::name() const {
    return m_name.value_or(string(m_type.name()));
}

namespace {
    const struct container_content
#if BOOST_VERSION < 105800
            : public boost::static_visitor< const enclosure & >
#endif
    {
        container_content() {}
        const enclosure &operator () ( const enclosure * const enc ) const {
            return *enc;
        }
        const enclosure &operator () ( const detail::factory_base * const enc ) const {
            return *enc->meta();
        }
    } c_container_dereferencer;
}
const enclosure &fostlib::detail::factory_base::ns() const {
    return boost::apply_visitor( c_container_dereferencer, m_container );
}

boost::shared_ptr< meta_instance > fostlib::detail::factory_base::meta() const {
    if ( !m_meta )
        m_meta = boost::shared_ptr< meta_instance >( new meta_instance( ns(), name() ) );
    return m_meta;
}

const detail::factory_base &fostlib::detail::find_factory( const std::type_info &type ) {
    return find_factory( string( type.name() ) );
}
const detail::factory_base &fostlib::detail::find_factory( const string &name ) {
    try {
        factory_registry_type::found_t factories( g_registry().find( name ) );
        if ( factories.size() != 1 )
            throw exceptions::out_of_range< std::size_t >( L"Factory could not be found for name", 1, 1, factories.size() );
        return **factories.begin();
    } catch ( exceptions::exception &e ) {
        fostlib::insert(e.data(), "factory", "name", name);
        throw;
    }
}

