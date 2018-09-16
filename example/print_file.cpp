//Copyright (c) 2018 Emil Dotchevski
//Copyright (c) 2018 Second Spectrum, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/leaf/expected.hpp>
#include <boost/leaf/common.hpp>
#include <boost/leaf/put.hpp>
#include <boost/leaf/current_exception_diagnostic_information.hpp>
#include <iostream>
#include <stdio.h>

namespace leaf = boost::leaf;

//We could define our own exception info types, but for this example the ones
//defined in <boost/leaf/common.hpp> are a perfect match.
using leaf::xi_file_name;
using leaf::xi_errno;

//Exception type hierarchy.
struct print_file_error : virtual std::exception { };
struct command_line_error : virtual print_file_error { };
struct bad_command_line : virtual command_line_error { };
struct io_error : virtual print_file_error { };
struct file_error : virtual io_error { };
struct fopen_error : virtual file_error { };
struct fread_error : virtual file_error { };
struct ftell_error : virtual file_error { };
struct fseek_error : virtual file_error { };

std::shared_ptr<FILE>
file_open( char const * file_name )
	{
	if( FILE * f = fopen(file_name,"rb") )
		return std::shared_ptr<FILE>(f,&fclose);
	else
		leaf::throw_with_info( fopen_error(), xi_file_name{file_name}, xi_errno{errno} );
	}

int
file_size( FILE & f )
	{
	auto put = leaf::preload(&leaf::get_errno);

	if( fseek(&f,0,SEEK_END) )
		throw fseek_error();
	int s = ftell(&f);
	if( s==-1L )
		throw ftell_error();
	if( fseek(&f,0,SEEK_SET) )
		throw fseek_error();
	return s;
	}

int
file_read( FILE & f, void * buf, int size )
	{
	int n = fread(buf,1,size,&f);
	if( ferror(&f) )
		leaf::throw_with_info( fread_error(), xi_errno{errno} );
	if( n!=size )
		throw fread_error();
	return n;
	}

void
print_file( char const * file_name )
	{
	auto put = leaf::preload( xi_file_name{file_name} );

	std::shared_ptr<FILE> f = file_open( file_name );
	std::string buffer( 1+file_size(*f), '\0' );
	file_read(*f,&buffer[0],buffer.size()-1);
	std::cout << buffer;
	}

char const *
parse_command_line( int argc, char const * argv[ ] )
	{
	if( argc!=2 )
		throw bad_command_line();
	return argv[1];
	}

int
main( int argc, char const * argv[ ] )
	{
	//We expect xi_file_name and xi_errno info to arrive with exceptions handled in this function.
	leaf::expected<xi_file_name,xi_errno> info;

	try
		{
		print_file(parse_command_line(argc,argv));
		}
	catch(
	bad_command_line & )
		{
		std::cout << "Bad command line argument" << std::endl;
		return 1;
		}
	catch(
	fopen_error const & )
		{
		//unwrap is given a list of match objects (in this case only one), which it attempts to match (in order) to
		//available exception info (if none can be matched, it throws leaf::mismatch_error).
		unwrap( info.match<xi_file_name,xi_errno>( [ ] ( std::string const & fn, int errn )
			{
			if( errn==ENOENT )
				std::cerr << "File not found: " << fn << std::endl;
			else
				std::cerr << "Failed to open " << fn << ", errno=" << errn << std::endl;
			} ) );
		return 2;
		}
	catch(
	io_error const & )
		{
		//unwrap is given a list of match objects, which it attempts to match (in order) to available exception info.
		//In this case it will first check if both xi_file_name and xi_errno are avialable; if not, it will next check
		//if just xi_errno is available; and if not, the last match will match even if no exception info is available,
		//to print a generic error message.
		unwrap(
			info.match<xi_file_name,xi_errno>( [ ] ( std::string const & fn, int errn )
				{
				std::cerr << "Failed to access " << fn << ", errno=" << errn << std::endl;
				} ),
			info.match<xi_errno>( [ ] ( int errn )
				{
				std::cerr << "I/O error, errno=" << errn << std::endl;
				} ),
			info.match<>( [ ]
				{
				std::cerr << "I/O error" << std::endl;
				} ) );
		return 3;
		}
	catch(...)
		{
		std::cerr <<
			"Unknown error, cryptic information follows." << std::endl <<
			leaf::current_exception_diagnostic_information();
		return 6;
		}
	return 0;
	}
