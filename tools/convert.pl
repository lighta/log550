#!/bin/perl
# author: lighta
#small script to convert an Atmel Studio6 Makefile to linux one
# NB you must have avr32 toolchain for the makefile to work afterward

use strict;
use warnings;
use Getopt::Long;
use File::Basename;
my $sFilein = "";
my $sFileout = "";
my $sHelp = 0;

Main();

sub GetArgs {
	GetOptions(
	'i=s' => \$sFilein, #Input file name.
	'o=s' => \$sFileout, #Output file name.
	'help!' => \$sHelp,
	) or $sHelp=1; #Display help if invalid options are supplied.

	if( $sHelp ) {
		print "Incorrect option specified. Available options:\n"
			."\t --o=filename => Output file name. \n"
			."\t --i=filename => Input file name. \n";
		exit;
	}
	unless($sFilein or $sFileout){
		print "ERROR: Filename_in and Filename_out are required to continue.\n";
		exit;
	}
}

sub escape { my ($str,$sregex,$sreplace) = @_;
	my @str_splitted = split($sregex, $str);
	my $result = "";
	for (my $i=0; $i<=$#str_splitted; $i++) {
		if ($i == 0) {
			$result = $str_splitted[0];
		} else {
			$result = $result.$sreplace.$str_splitted[$i];
		}
	}
	return $result
}

sub ConvertFile { my($sFilein,$sFileout)=@_;
	my $sFHout;
	print "Starting ConvertFile with: \n\t filein=$sFilein \n\t fileout=$sFileout \n";
	open FHIN,"$sFilein" or die "ERROR: Can't read or locate $sFilein.\n";
	open $sFHout,">$sFileout" or die "ERROR: Can't write $sFileout.\n";
	
	while(my $ligne=<FHIN>) {
		if ($ligne =~ /^\s*$/ ) { #empty line with space
				print $sFHout "\n";
				next;
		}
		$ligne =~ s/cmd\.exe/sh/g;
		if($ligne =~ /\$\(QUOTE\)C\:\\Program\ Files\ \(x86\)\\Atmel\\Atmel\ Toolchain\\AVR32\ GCC\\Native\\3\.4\.2\.1002\\avr32\-gnu\-toolchain\\bin\\avr32\-gcc\.exe\$\(QUOTE\)/ ){
			$ligne =~ s!\$\(QUOTE\)C\:\\Program\ Files\ \(x86\)\\Atmel\\Atmel\ Toolchain\\AVR32\ GCC\\Native\\3\.4\.2\.1002\\avr32\-gnu\-toolchain\\bin\\avr32\-gcc\.exe\$\(QUOTE\)!\@avr32\-gcc!g;
			$ligne = "\t".'@-mkdir -p $(shell dirname $@)'."\n".$ligne; #safecheck with folder creation for linux
		}
		$ligne =~ s!\"C\:\\Program\ Files\ \(x86\)\\Atmel\\Atmel\ Toolchain\\AVR32\ GCC\\Native\\3\.4\.2\.1002\\avr32\-gnu\-toolchain\\bin\\avr32\-objcopy\.exe\"!\@avr32\-objcopy!g;
		$ligne =~ s!\"C\:\\Program\ Files\ \(x86\)\\Atmel\\Atmel\ Toolchain\\AVR32\ GCC\\Native\\3\.4\.2\.1002\\avr32\-gnu\-toolchain\\bin\\avr32\-objdump\.exe\"!\@avr32\-objdump!g;
		$ligne =~ s!\"C\:\\Program\ Files\ \(x86\)\\Atmel\\Atmel\ Toolchain\\AVR32\ GCC\\Native\\3\.4\.2\.1002\\avr32\-gnu\-toolchain\\bin\\avr32\-size\.exe\"!\@avr32\-size!g;
		print $sFHout $ligne;
	}
	print $sFHout "\n";
}

sub Main {
	GetArgs();
	my($filename, $dir, $suffix) = fileparse($0);
	chdir $dir; #put ourself like was called in tool folder
	ConvertFile($sFilein,$sFileout);
	print "Conversion ended.\n";
}
