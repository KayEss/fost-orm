/*
    Copyright 1999-2010, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-schema.hpp"
#include <fost/detail/db-driver.hpp>
#include <fost/threading>

#include <fost/exception/transaction_fault.hpp>
#include <fost/exception/unexpected_eof.hpp>


using namespace fostlib;


namespace {


    const setting< string > c_defaultDriver( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"Default driver", L"json", true );
#ifdef WIN32
    const setting< string > c_json_driver( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database drivers", L"json", L"fost-jsondb.dll", true );
#else
#ifndef NeXTBSD 
#ifdef __APPLE__
    const setting< string > c_json_driver( L"/fost-base/Cpp/fost-schema/db.cpp",
        L"Database drivers", L"json",
            #ifdef _DEBUG
                L"libfost-jsondb-d.dylib",
            #else
                L"libfost-jsondb.dylib",
            #endif
         true
    );
#else
    // linux?
    const setting< string > c_json_driver( L"/fost-base/Cpp/fost-schema/db.cpp",
        L"Database drivers", L"json",
            #ifdef _DEBUG
                L"libfost-jsondb-d.so",
            #else
                L"libfost-jsondb.so",
            #endif
        true
    );
#endif
#endif
#endif

#ifdef _DEBUG
#define LOGGING true
#else
#define LOGGING false
#endif
    const setting< bool > c_logConnect( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"LogConnect", LOGGING, true );
    const setting< bool > c_logRead( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"LogRead", LOGGING, true );
    const setting< bool > c_logWrite( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"LogWrite", LOGGING, true );
    const setting< bool > c_logFailure( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"LogFailure", true, true );
#undef LOGGING

    const setting< int > c_countCommandTimeout( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"CountCommandTimeout", 120, true );
    const setting< int > c_readCommandTimeout( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"ReadCommandTimeout", 3600, true );
    const setting< int > c_writeCommandTimeout( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"WriteCommandTimeout", 15, true );


    typedef threadsafe_store< const dbinterface * > database_driver_store;
    database_driver_store &g_interfaces() {
        static database_driver_store interfaces;
        return interfaces;
    }

    fostlib::string driver_name( const fostlib::string &d ) {
        string p( partition( d, L";" ).first );
        string::size_type s( p.find( L'/' ) );
        if ( s == string::npos )
            return string();
        else
            return string( p, 0, s );
    }
    fostlib::string dsn( const fostlib::string &d ) {
        fostlib::string n( driver_name( d ) );
        if ( n.empty() )
            return d;
        else
            return d.substr( n.length() + 1 );
    }
    fostlib::string driver( const fostlib::string &read, const fostlib::nullable< fostlib::string > &write ) {
        fostlib::string rd( driver_name( read ) ), wd( driver_name( write.value( read ) ) );
        if ( rd != wd )
            throw fostlib::exceptions::data_driver( L"Read and write drivers not the same", rd, wd );
        if ( rd.empty() )
            return c_defaultDriver.value();
        else
            return rd;
    }


}


/*
    DBInterface
*/


fostlib::dbinterface::dbinterface( const string &name )
: name( name ) {
    g_interfaces().add( name, this );
}
fostlib::dbinterface::~dbinterface() {
    g_interfaces().remove( name(), this );
    //std::cout << L"Removed driver " << name() << std::endl;
}


std::auto_ptr< dbinterface::connection_data > fostlib::dbinterface::connect( dbconnection &d ) const {
    return std::auto_ptr< connection_data >();
}

fostlib::dbinterface::read::read( dbconnection &d )
: m_connection( d ) {
}


fostlib::dbinterface::read::~read() {
}


fostlib::dbinterface::write::write( dbinterface::read &r )
: m_connection( r.m_connection ), m_reader( r ) {
}


fostlib::dbinterface::write::~write() {
}


fostlib::dbinterface::recordset::recordset( const sql::statement &c )
: command( c ) {
}


fostlib::dbinterface::recordset::~recordset() {
}


/*
    DBConnection
*/


namespace {
    std::map< string, boost::weak_ptr< dynlib > > &g_dlls() {
        static std::map< string, boost::weak_ptr< dynlib > > dlls;
        return dlls;
    }
    std::pair< const dbinterface *, boost::shared_ptr< dynlib > > load_driver( const string &driver ) {
        // This code doesn't deal very gracefully with the case where there are more than one driver
        // in a driver DLL/so
        boost::shared_ptr< dynlib > driver_dll( g_dlls()[ driver ].lock() );
        if ( g_interfaces().find( driver ).empty() ) {
            nullable< string > dll = setting< string >::value( L"Database drivers", driver, null );
            if ( dll.isnull() )
                throw exceptions::data_driver( L"No driver found", driver );
            else
                try {
                    driver_dll.reset( new dynlib( dll.value() ) );
                    g_dlls()[ driver ] = driver_dll;
                    if ( g_interfaces().find( driver ).empty() )
                        throw exceptions::data_driver( L"No driver found even after loading driver file", driver );
                } catch ( exceptions::exception &e ) {
                    e.info() << L"Database driver file " << dll.value() << L"\nDrivers available: ";
                    database_driver_store::keys_t k(g_interfaces().keys());
                    if ( k.size() )
                        for ( database_driver_store::keys_t::const_iterator i(k.begin()); i != k.end(); ++i )
                            e.info() << *i << L" ";
                    else
                        e.info() << L"[no drivers are available]";
                    e.info() << std::endl;
                    throw;
                }
        }
        return std::make_pair( &**g_interfaces().find( driver ).begin(), driver_dll );
    }
    std::pair< const dbinterface *, boost::shared_ptr< dynlib > > connection( const json &cnx ) {
        string driver = cnx[ L"driver" ].get< string >().value( c_defaultDriver.value() );
        return load_driver( driver );
    }
    std::pair< const dbinterface *, boost::shared_ptr< dynlib > > connection( const string&read, const nullable< string > &write ) {
        string d = driver( read, write );
        return load_driver( d );
    }
}


const setting< bool > fostlib::dbconnection::c_commitCount( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"Commit count", true, true );
const setting< string > fostlib::dbconnection::c_commitCountDomain( L"/fost-base/Cpp/fost-schema/db.cpp", L"Database", L"Commit count domain", L"Commit count", true );


namespace {
    json cnx_conf( const string &r, const nullable< string > &w ) {
        json conf;
        jcursor( L"read" )( conf ) = dsn( r );
        if ( !w.isnull() )
            jcursor( L"write" )( conf ) = dsn( w.value() );
        return conf;
    }
    void establish(
        dbconnection &dbc,
        const std::pair< const dbinterface *, boost::shared_ptr< fostlib::dynlib > > &iface,
        boost::scoped_ptr< dbinterface::connection_data > &cnx_data,
        boost::shared_ptr< dbinterface::read > &connection
    ) {
        std::auto_ptr< dbinterface::connection_data > data = iface.first->connect( dbc );
        if ( data.get() )
            cnx_data.reset( data.release() );
        connection = iface.first->reader( dbc );
    }
}
fostlib::dbconnection::dbconnection( const json &j )
: configuration( j ), m_interface( ::connection( j ) ), m_transaction( NULL ) {
    establish( *this, m_interface, m_cnx_data, m_connection );
}
fostlib::dbconnection::dbconnection( const fostlib::string &r )
: configuration( cnx_conf( r, null ) ), m_interface( ::connection( r, null ) ), m_transaction( NULL ) {
    establish( *this, m_interface, m_cnx_data, m_connection );
}
fostlib::dbconnection::dbconnection( const fostlib::string &r, const fostlib::string &w )
: configuration( cnx_conf( r, w ) ), m_interface( ::connection( r, w ) ), m_transaction( NULL ) {
    establish( *this, m_interface, m_cnx_data, m_connection );
}


fostlib::dbconnection::~dbconnection()
try{
} catch ( ... ) {
    /*try {
        if ( setting< bool >::value( L"Database", L"LogFailure" ) ) {
            YAML::Record failure;
            failure.add( L"DB", L"Failure" );
            failure.add( L"Place", L"DBConnection::~DBConnection()" );
            failure.add( L"Exception", L"Unknown exception" );
            failure.add( L"Connection", L"Read" );
            failure.log();
        }
    } catch ( exceptions::exception & ) {
        absorbException();
    }*/
}

void fostlib::dbconnection::create_database( const fostlib::string &name ) {
    driver().create_database( *this, name );
}
void fostlib::dbconnection::drop_database( const fostlib::string &name ) {
    driver().drop_database( *this, name );
}


const dbinterface &fostlib::dbconnection::driver() const {
    return *m_interface.first;
}


recordset fostlib::dbconnection::query( const meta_instance &item, const json &key ) {
    if ( !m_connection )
        throw exceptions::transaction_fault( L"Database connection has not started a read transaction" );
    return m_connection->query( item, key );
}
recordset fostlib::dbconnection::query( const sql::statement &cmd ) {
    if ( !m_connection )
        throw exceptions::transaction_fault( L"Database connection has not started a read transaction" );
    return m_connection->query( cmd );
}


int64_t fostlib::dbconnection::next_id( const string &counter ) {
    return driver().next_id( *this, counter );
}
int64_t fostlib::dbconnection::current_id( const string &counter ) {
    return driver().current_id( *this, counter );
}
void fostlib::dbconnection::used_id( const string &counter, int64_t value ) {
    driver().used_id( *this, counter, value );
}


bool fostlib::dbconnection::in_transaction() const {
    return m_transaction != NULL;
}


dbtransaction &fostlib::dbconnection::transaction() {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"No transaction is active" );
    return *m_transaction;
}


dbinterface::connection_data &fostlib::dbconnection::connection_data() {
    if ( m_cnx_data.get() )
        return *m_cnx_data;
    else
        throw exceptions::null("This connection driver does not make use of connection data");
}


/*
    Transaction
*/


fostlib::dbtransaction::dbtransaction( dbconnection &dbc )
: m_connection( dbc ) {
    if ( m_connection.in_transaction() )
        throw exceptions::transaction_fault( L"Nested transaction not yet supported" );
    m_transaction = dbc.m_connection->writer();
    m_connection.m_transaction = this;
}


fostlib::dbtransaction::~dbtransaction() {
    if ( m_transaction )
        m_transaction->rollback();
    if ( m_connection.m_transaction != this )
        throw exceptions::transaction_fault( L"The current transaction has changed" );
    m_connection.m_transaction = NULL;
}


dbtransaction &fostlib::dbtransaction::create_table( const meta_instance &meta ) {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"This transaction has already been used" );
    m_transaction->create_table( meta );
    return *this;
}


dbtransaction &fostlib::dbtransaction::drop_table( const meta_instance &meta ) {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"This transaction has already been used" );
    m_transaction->drop_table( meta );
    return *this;
}
dbtransaction &fostlib::dbtransaction::drop_table( const fostlib::string &table ) {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"This transaction has already been used" );
    m_transaction->drop_table( table );
    return *this;
}


dbtransaction &fostlib::dbtransaction::insert( const instance &object, boost::function< void( void ) > oncommit ) {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"This transaction has already been used" );
    m_oncommit.push_back( oncommit );
    m_transaction->insert( object );
    return *this;
}
dbtransaction &fostlib::dbtransaction::execute( const sql::statement &command ) {
    if ( !m_transaction )
        throw exceptions::transaction_fault( L"This transaction has already been used" );
    m_transaction->execute( command );
    return *this;
}


namespace {
    void exec( boost::function< void( void ) > f ) {
        f();
    }
}
void fostlib::dbtransaction::commit() {
    m_transaction->commit();
    std::for_each( m_oncommit.begin(), m_oncommit.end(), exec );
    m_transaction = boost::shared_ptr< dbinterface::write >();
}


/*
    Recordset
*/


fostlib::recordset::recordset( boost::shared_ptr< dbinterface::recordset > rs )
: m_interface( rs ) {
}


fostlib::recordset::~recordset()
try {
} catch ( ... ) {
    absorbException();
}


const sql::statement &fostlib::recordset::command() const {
    return m_interface->command();
}


bool fostlib::recordset::eof() const {
    return m_interface->eof();
}


bool fostlib::recordset::isnull( const fostlib::string &i ) const {
    return field( i ).isnull();
}


bool fostlib::recordset::isnull( std::size_t i ) const {
    return field( i ).isnull();
}


void fostlib::recordset::moveNext() {
    m_interface->moveNext();
}


std::size_t fostlib::recordset::fields() const {
    return m_interface->fields();
}


const string &fostlib::recordset::name( std::size_t i ) const {
    return m_interface->name( i );
}


const json &fostlib::recordset::field( std::size_t i ) const {
    if ( m_interface.get() )
        return m_interface->field( i );
    else
        throw exceptions::unexpected_eof( L"The recordset came from a write SQL instruction" );
}


const json &fostlib::recordset::field( const string &name ) const {
    if ( m_interface.get() )
        return m_interface->field( name );
    else
        throw exceptions::unexpected_eof( L"The recordset came from a write SQL instruction" );
}


json fostlib::recordset::to_json() const {
    return m_interface->to_json();
}