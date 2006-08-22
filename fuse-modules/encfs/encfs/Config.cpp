/*****************************************************************************
 * Author:   Valient Gough <vgough@pobox.com>
 *
 *****************************************************************************
 * Copyright (c) 2004, Valient Gough
 * 
 * This program is free software; you can distribute it and/or modify it under 
 * the terms of the GNU General Public License (GPL), as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include "Config.h"

#include <rlog/rlog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


using namespace std;
using namespace rlog;


Config::Config()
{
}

Config::~Config()
{
}

// read the entire file into a ConfigVar instance and then use that to decode
// into mapped variables.
bool 
Config::load(const char *fileName)
{
    struct stat stbuf;
    memset( &stbuf, 0, sizeof(struct stat));
    if( lstat( fileName, &stbuf ) != 0)
	return false;

    int size = stbuf.st_size;

    int fd = open( fileName, O_RDONLY );
    if(fd < 0)
	return false;

    char *buf = new char[size];

    int res = ::read( fd, buf, size );
    close( fd );

    if( res != size )
    {
	rWarning("Partial read of config file, expecting %i bytes, got %i",
		size, res);
	delete[] buf;
	return false;
    }

    ConfigVar in;
    in.write( (unsigned char *)buf, size );
    delete[] buf;

    return loadFromVar( in );
}

bool 
Config::loadFromVar(ConfigVar &in)
{
    in.resetOffset();

    // parse.
    int numEntries = in.readBER();

    for(int i=0; i<numEntries; ++i)
    {
	string key, value;
	in >> key >> value;

	if(key.length() == 0)
	{
	    rError("Invalid key encoding in buffer");
	    return false;
	}
	ConfigVar newVar( value );
	vars.insert( make_pair( key, newVar ) );
    }

    return true;
}

bool
Config::save(const char *fileName) const
{
    // write everything to a ConfigVar, then output to disk
    ConfigVar out = toVar();

    int fd = ::open( fileName, O_RDWR | O_CREAT, 0640 );
    if(fd >= 0)
    {
	int retVal = ::write( fd, out.buffer(), out.size() );
	close( fd );
	if(retVal != out.size())
	{
	    rError("Error writing to config file %s", fileName);
	    return false;
	}
    } else
    {
	rError("Unable to open or create file %s", fileName);
	return false;
    }

    return true;
}

ConfigVar
Config::toVar() const
{
    // write everything to a ConfigVar, then output to disk
    ConfigVar out;
    out.writeBER( vars.size() );
    map<string, ConfigVar>::const_iterator it;
    for(it = vars.begin(); it != vars.end(); ++it)
    {
	out.writeBER( it->first.size() );
	out.write( (unsigned char*)it->first.data(), it->first.size() );
	out.writeBER( it->second.size() );
	out.write( (unsigned char*)it->second.buffer(), it->second.size() );
    }

    return out;
}

ConfigVar Config::operator[] ( const std::string &varName ) const
{
    // read only
    map<string, ConfigVar>::const_iterator it = vars.find( varName );
    if( it == vars.end() )
	return ConfigVar();
    else
	return it->second;
}

ConfigVar &Config::operator[] ( const std::string &varName )
{
    return vars[ varName ];
}
