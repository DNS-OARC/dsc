use ExtUtils::MakeMaker;

WriteMakefile(
    'NAME'	=> 'DSC',
    'VERSION_FROM' => 'DSC/ploticus.pm',

    'LIBS'	=> [],
    'DEFINE'	=> '',
    'INC'	=> '',
    'PREREQ_PM'	=> {
		CGI => 0,
		CGI::Untaint => 0,
		Digest::MD5 => 0,
		File::Flock => 0,
		File::NFSLock => 0,
		File::Temp => 0,
		Hash::Merge => 0,
		IP::Country => 0,
		List::Util => 0,
		MIME::Base64 => 0,
		Math::Calc::Units => 0,
		Text::Template => 0,
		URI::Escape => 0,
		XML::Simple => 0,
    },
);
