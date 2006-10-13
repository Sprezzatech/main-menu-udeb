#!/usr/bin/perl -w
# Script to filter out the output of websec-txt-all script
# Contributed by Jacobo Tarrio <jtarrio@debian.org>

use strict;

my @oldstat = ();
my @newstat = ();
my %oldfiles = ();
my %newfiles = ();
my $oldglobal = "";
my $newglobal = "";
my $oldmaster = "(unknown)";
my $newmaster = "(unknown)";

# Gather data
my @lines = <>;
foreach my $line (@lines) {
  chomp $line;
  if ($line =~ /^---\s+.*?level(\d)_(.*?)\.txt\.old\s+(.*?)\.0+\s+([-+]\d+)$/) {
    @oldstat = ($1, $2, "$3 $4");
  }
  elsif ($line =~ /^\+\+\+\s+.*?level(\d)_(.*?)\.txt\s+(.*?)\.0+\s+([-+]\d+)$/) {
    @newstat = ($1, $2, "$3 $4");
  }
  elsif ($line =~ /^([-+])Global statistics: (\d+t\d+f\d+u)/) {
    $oldglobal = $2 if ($1 eq '-');
    $newglobal = $2 if ($1 eq '+');
  }
  elsif ($line =~ /^([-+])(?:\s+\*?)(.*?\.po):?\s+((\d+t)?(\d+f)?(\d+u)?)/) {
    $oldfiles{$2} = $3 if ($1 eq '-');
    $newfiles{$2} = $3 if ($1 eq '+');
  }
  elsif ($line =~ /^([-+])Master:\s+(\d*t\d*f\d*u)/) {
    $oldmaster = $2 if ($1 eq '-');
    $newmaster = $2 if ($1 eq '+');
  }
}

# Prepare data
my $new = "";
my $changed = "";
my $removed = "";
foreach my $f (sort keys %newfiles) {
  if (!defined $oldfiles{$f}) {
    $new .= "  * $f : $newfiles{$f}\n";
  }
  else {
    $changed .= "  * $f : $oldfiles{$f} -> $newfiles{$f}\n";
    delete $oldfiles{$f};
  }
}
foreach my $f (sort keys %oldfiles) {
  $removed .= "  * $f : $oldfiles{$f}\n";
}

# Create report
my $report = "";
$report .= "Removed files:\n$removed\n" if ($removed ne "");
$report .= "Added files:\n$new\n" if ($new ne "");
$report .= "Changed files:\n$changed\n" if ($changed ne "");

$report .= "Master translation: $oldmaster -> $newmaster\n\n" if ($oldmaster ne $newmaster);

$report .= "Old totals: $oldglobal\n" if ($oldglobal ne "");
$report .= "New totals: $newglobal\n" if ($newglobal ne "");

# Display report if not empty
if ($report ne "") {
  print "Changes for $newstat[1] level $newstat[0]\n";
  print "   between $oldstat[2] and $newstat[2]\n";
  print "\n";
  print $report;
}
