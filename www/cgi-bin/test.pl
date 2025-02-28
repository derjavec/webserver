#!/usr/bin/perl
use strict;
use warnings;

print "Content-Type: text/html\n\n";

my $method = $ENV{'REQUEST_METHOD'} || 'GET';

if ($method eq 'POST') {
    my $content_length = $ENV{'CONTENT_LENGTH'} || 0;
    my $input;
    read(STDIN, $input, $content_length);

    my $name = 'Guest';
    if ($input =~ /name=(.*)/) {
        $name = $1;
    }

    print "<h1>Welcome, $name!</h1>";
} else {
    print "<h1>Hello! This is a static message from PERL.</h1>";
}
