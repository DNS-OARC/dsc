use ExtUtils::MakeMaker;

WriteMakefile(
    'NAME'	=> 'OARC',
    'VERSION_FROM' => 'lib/OARC/ploticus.pm',

    # You MUST alter this to match your LIBS line from ploticus
    'LIBS'	=> [],
    'DEFINE'	=> '',
    'INC'	=> '',
);
