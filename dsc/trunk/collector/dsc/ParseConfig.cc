
#include <iostream>
#include <fstream>

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

extern "C" int add_local_address(const char *);
extern "C" int open_interface(const char *);
extern "C" int set_run_dir(const char *);
extern "C" int add_dataset(const char *name, const char *layer,
	const char *firstname, const char *firstindexer,
	const char *secondname, const char *secondindexer,
	const char *filtername);
extern "C" int set_bpf_vlan_tag_byte_order(const char *);
extern "C" int set_match_vlan(const char *);

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
	ctMatchVlan,
	ctConfig = 30,
	ctMax
} configToken;

Rule rBareToken;
Rule rQuotedToken;
Rule rToken;
Rule rDecimalNumber;
Rule rComment;

Rule rIPv4Address;
Rule rHostOrNet;

Rule rInterface;
Rule rRunDir;
Rule rLocalAddr;
Rule rPacketFilterProg;
Rule rDataset;
Rule rBVTBO;
Rule rMatchVlan;

Rule rConfig;

string
remove_quotes(const string &s) 
{
	string::size_type f = s.find_first_of('"');
	string::size_type l = s.find_last_of('"');
	return s.substr(f+1, l-f-1);
}


/* interpret()
 *
 * I'm a recursive function.
 */
static int
interpret(const Pree &tree, int level)
{
	int x;
        if (tree.rid() == rInterface.id()) {
		assert(tree.count() > 1);
                if (open_interface(tree[1].image().c_str()) != 1)
			return 0;
	} else
        if (tree.rid() == rRunDir.id()) {
		assert(tree.count() > 1);
                if (set_run_dir(remove_quotes(tree[1].image()).c_str()) != 1)
                    return 0;
	} else
        if (tree.rid() == rLocalAddr.id()) {
		assert(tree.count() > 1);
		if (add_local_address(tree[1].image().c_str()) != 1)
			return 0;
        } else
	if (tree.rid() == rDataset.id()) {
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
        } else
	if (tree.rid() == rBVTBO.id()) {
		assert(tree.count() > 1);
		if (set_bpf_vlan_tag_byte_order(tree[1].image().c_str()) != 1)
			return 0;
	} else
	if (tree.rid() == rMatchVlan.id()) {
		for(unsigned int i = 0; i<tree[1].count(); i++) {
			if (set_match_vlan(tree[1][i].image().c_str()) != 1)
				return 0;
		}
	} else
        {
                for (unsigned int i = 0; i < tree.count(); i++) {
                        if (interpret(tree[i], level + 1) != 1)
				return 0;
                }
        }
	return 1;
}

void
ParseConfig(const char *fn)
{
//	Rule rBareToken("bare-token", ctBareToken);
//	Rule rQuotedToken("quoted-token", ctQuotedToken);
//	Rule rToken("token", ctToken);
//	Rule rDecimalNumber("decimal-number", ctDecimalNumber);
//	Rule rComment("comment", ctComment);
//
//	Rule rIPv4Address("ipv4-address", ctIPv4Address);
//	Rule rHostOrNet("host-or-net", ctHostOrNet);
//
//	Rule rInterface("interface", ctInterface);
//	Rule rRunDir("run_dir", ctRunDir);
//	Rule rLocalAddr("local_address", ctLocalAddr);
//	Rule rPacketFilterProg("bpf_program", ctPacketFilterProg);
//	Rule rDataset("dataset", ctDataset);
//	Rule rBVTBO("bpf_vlan_tag_byte_order", ctBVTBO);
//	Rule rMatchVlan("match_vlan", ctMatchVlan);
//
//	Rule rConfig("Config", ctConfig);

	// primitive token level
	rBareToken = +(anychar_r - space_r - "\"" - ";");
	rQuotedToken = quoted_r('"', anychar_r, '"');
	rToken = rBareToken | rQuotedToken;
	rDecimalNumber = +digit_r;
	rComment = quoted_r("#", anychar_r, eol_r);

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
	rMatchVlan = "match_vlan" >> +rDecimalNumber >>";" ;

	// the whole config
	rConfig = *(
		rInterface |
		rRunDir |
		rLocalAddr |
		rPacketFilterProg |
		rDataset |
		rBVTBO |
		rMatchVlan
	) >> end_r;

	// trimming - do not allow whitespace INSIDE these objects
	rConfig.trim(*(space_r | rComment));
	rQuotedToken.verbatim(true);
	rBareToken.verbatim(true);
	rDecimalNumber.verbatim(true);
	rIPv4Address.verbatim(true);

	// parse tree shaping
	rQuotedToken.leaf(true);
	rBareToken.leaf(true);
	rDecimalNumber.leaf(true);
	rIPv4Address.leaf(true);
	rHostOrNet.leaf(true);

	// commit points
        rInterface.committed(true);
        rRunDir.committed(true);
        rLocalAddr.committed(true);
        rPacketFilterProg.committed(true);
        rDataset.committed(true);
	rBVTBO.committed(true);
	rMatchVlan.committed(true);

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
		string::size_type p = parser.result().maxPos;
		cerr << "parse failure after " << p
			<< " bytes, near "
		       << (p >= config.size() ?
			       string(" the end of input") : config.substr(p).substr(0, 160))
		       << endl;
		exit(1);
	}
	if (interpret(parser.result().pree, 0) != 1)
		abort();
}
