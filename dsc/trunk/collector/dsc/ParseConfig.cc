
#include <iostream>
#include <fstream>

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"

extern "C" int add_local_address(const char *);
extern "C" int open_interface(const char *);
extern "C" int set_run_dir(const char *);

extern "C" void ParseConfig(const char *);

using namespace Hapy;

enum {
	ctWhiteSpace = 1,
	ctBareToken,
	ctQuotedToken,
	ctToken,
	ctDecimalNumber,
	ctInterfaceName = 11,
	ctIPv4Address,
	ctInterface = 21,
	ctRunDir,
	ctLocalAddr,
	ctPacketFilterProg,
	ctConfig = 30,
	ctMax
} configToken;

string
remove_quotes(const string &s) 
{
	string::size_type f = s.find_first_of('"');
	string::size_type l = s.find_last_of('"');
	return s.substr(f+1, l-f-1);
}


/* my_parse
 *
 * I'm a recursive function.
 */
static int
my_parse(const PreeNode &tree, int level)
{
        switch (tree.rid()) {
        case ctInterface:
                if (open_interface(tree[1].image().c_str()) != 1)
			return 0;
                break;
        case ctRunDir:
                if (set_run_dir(remove_quotes(tree[1].image()).c_str()) != 1)
                    return 0;
                break;
        case ctLocalAddr:
		if (add_local_address(tree[1].image().c_str()) != 1)
			return 0;
                break;
        default:
                for (unsigned int i = 0; i < tree.count(); i++) {
                        if (my_parse(tree[i], level + 1) != 1)
				return 0;
                }
        }
	return 1;
}

void
ParseConfig(const char *fn)
{
	Rule rBareToken("bare-token", ctBareToken);
	Rule rQuotedToken("quoted-token", ctQuotedToken);
	Rule rToken("token", ctToken);
	Rule rDecimalNumber("decimal-number", ctDecimalNumber);
	Rule rWS("whitespace", ctWhiteSpace);

	Rule rInterfaceName("interfrace-name", ctInterfaceName);
	Rule rIPv4Address("ipv4-address", ctIPv4Address);

	Rule rInterface("Interface", ctInterface);
	Rule rRunDir("RunDir", ctRunDir);
	Rule rLocalAddr("LocalAddr", ctLocalAddr);
	Rule rPacketFilterProg("PacketFilter", ctPacketFilterProg);

	Rule rConfig("Config", ctConfig);


	// token level
	rWS = *space_r();
	rBareToken = +(anychar_r() - space_r() - "\"" - ";");
	rQuotedToken = quoted_r('"', anychar_r(), '"');
	rToken = rBareToken | rQuotedToken;
	rDecimalNumber = +(digit_r());

	// special token level
	rInterfaceName = rBareToken;
	rIPv4Address = rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber;


	// rule/line level
	rInterface = "interface" >>rWS >>rInterfaceName >>rWS >>";" >>rWS;
	rRunDir = "run_dir" >>rWS >>rQuotedToken >>rWS >>";" >>rWS;
	rLocalAddr = "local_address" >>rWS >>rIPv4Address >>rWS >>";" >>rWS;
	rPacketFilterProg = "bpf_program" >>rWS >>rQuotedToken >>rWS >>";" >>rWS;

	// the whole config
	rConfig = *(rInterface | rRunDir | rLocalAddr | rPacketFilterProg) >> end_r();

	// trimming
	rConfig.trim(*space_r());
	rQuotedToken.verbatim(true);
	rBareToken.verbatim(true);
	rIPv4Address.verbatim(true);

	// parse tree shaping
	rQuotedToken.leaf(true);
	rBareToken.leaf(true);
	rIPv4Address.leaf(true);

	string config;
	char c;
	ifstream in(fn);
	while (in.get(c))
		config += c;
	in.close();

	Parser parser;
	parser.grammar(rConfig);
	if (!parser.parse(config)) {
		parser.result().pree.print(cerr << "complete failure" << endl, "");
		string::size_type p = parser.result().pree.rawImageSize();
		cerr << "parse failure after " << p
			<< " bytes, near "
		       << (p >= config.size() ?
			       string(" the end of input") : config.substr(p).substr(0, 160))
		       << endl;
	}
	if (my_parse(parser.result().pree, 0) != 1)
		abort();
}
