#!/usr/bin/perl -w
use strict;
use IO::Socket;
my ($hSock, $sLine);
$hSock = IO::Socket::INET->new( LocalPort => 514, Proto => 'udp', Reuse => 1) or die "Ups,
socket: $@";
print "Ok, Port 514-UDP ist aktiv\n";
while ($hSock->recv($sLine, 1023))
{
print $sLine, "\n";
}
die "Jetzt nicht!\n"; 
