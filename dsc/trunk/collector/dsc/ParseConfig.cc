
#include <iostream>
#include <fstream>

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"

extern "C" int add_local_address(const char *);
extern "C" int open_interface(const char *);
extern "C" int set_run_dir(const char *);
extern "C" int add_dataset(const char *name, const char *layer,
	const char *firstname, const char *firstindexer,
	const char *secondname, const char *secondindexer,
	const char *filtername);

extern "C" void ParseConfig(const char *);

using namespace Hapy;

enum {
	ctBareToken = 1,
	ctQuotedToken,
	ctToken,
	ctDecimalNumber,
	ctComment,
	ctIPv4Address = 11,
	ctHostOrNet,
	ctInterface = 21,
	ctRunDir,
	ctLocalAddr,
	ctPacketFilterProg,
	ctDataset,
	ctBVTBO,		// bpf_vlan_tag_byte_order
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
	int x;
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
	case ctDataset:
		assert(tree.count() > 9);
		x = add_dataset(tree[1].image().c_str(),	// name
			tree[2].image().c_str(),		// layer
			tree[3].image().c_str(),		// 1st dim name
			tree[5].image().c_str(),		// 1st dim indexer
			tree[6].image().c_str(),		// 2nd dim name
			tree[8].image().c_str(),		// 2nd dim indexer
			tree[9].image().c_str());		// filter name
		if (x != 1)
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
	Rule rComment("comment", ctComment);

	Rule rIPv4Address("ipv4-address", ctIPv4Address);
	Rule rHostOrNet("host-or-net", ctHostOrNet);

	Rule rInterface("Interface", ctInterface);
	Rule rRunDir("RunDir", ctRunDir);
	Rule rLocalAddr("LocalAddr", ctLocalAddr);
	Rule rPacketFilterProg("PacketFilter", ctPacketFilterProg);
	Rule rDataset("Dataset", ctDataset);
	Rule rBVTBO("bpf_vlan_tag_byte_order", ctBVTBO);

	Rule rConfig("Config", ctConfig);

	// primitive token level
	rBareToken = +(anychar_r() - space_r() - "\"" - ";");
	rQuotedToken = quoted_r('"', anychar_r(), '"');
	rToken = rBareToken | rQuotedToken;
	rDecimalNumber = +digit_r();
	rComment = quoted_r("#", anychar_r(), eol_r());

	// fancy token level
	rIPv4Address = rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber;
	rHostOrNet = string_r("host") | string_r("net");


	// rule/line level
	rInterface = "interface" >>rBareToken >>";" ;
	rRunDir = "run_dir" >>rQuotedToken >>";" ;
	rLocalAddr = "local_address" >>rIPv4Address >>";" ;
	rPacketFilterProg = "bpf_program" >>rQuotedToken >>";" ;
	rDataset = "dataset" >>rBareToken >>rBareToken
		>>rBareToken >>":" >>rBareToken
		>>rBareToken >>":" >>rBareToken
		>>rBareToken >>";" ;
	rBVTBO = "bpf_vlan_tag_byte_order" >>rHostOrNet >>";" ;

	// the whole config
	rConfig = *(
		rInterface |
		rRunDir |
		rLocalAddr |
		rPacketFilterProg |
		rDataset |
		rBVTBO
	) >> end_r();

	// trimming
	rConfig.trim(*(space_r() | rComment));
	rQuotedToken.verbatim(true);
	rBareToken.verbatim(true);
	rIPv4Address.verbatim(true);

	// parse tree shaping
	rQuotedToken.leaf(true);
	rBareToken.leaf(true);
	rIPv4Address.leaf(true);
	rHostOrNet.leaf(true);

	// commit points
        rInterface.committed(true);
        rRunDir.committed(true);
        rLocalAddr.committed(true);
        rPacketFilterProg.committed(true);
        rDataset.committed(true);
	rBVTBO.committed(true);

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
