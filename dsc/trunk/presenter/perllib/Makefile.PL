use ExtUtils::MakeMaker;

WriteMakefile(
    'NAME'	=> 'DSC',
    'VERSION_FROM' => 'DSC/ploticus.pm',

    'LIBS'	=> [],
    'DEFINE'	=> '',
    'INC'	=> '',
    'PREREQ_PM'	=> {
		CGI => undef,
		CGI::Untaint => undef,
		Digest::MD5 => undef,
		File::Flock => undef,
		File::Temp => undef,
		Hash::Merge => undef,
		IP::Country => undef,
		List::Util => undef,
		MIME::Base64 => undef,
		Math::Calc::Units => undef,
		Text::Template => undef,
		URI::Escape => undef,
		XML::Simple => undef,
    },
);
