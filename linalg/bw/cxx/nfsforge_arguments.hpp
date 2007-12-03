#ifndef NFSFORGE_ARGUMENTS_HPP_
#define NFSFORGE_ARGUMENTS_HPP_

#include "arguments.hpp"
#include "fmt.hpp"
#include <gmp.h>
#include <gmpxx.h>

struct nfsforge_arguments {
	long p;
	std::string in;
	std::string out;
	std::string clink;
	int nchar;
	nfsforge_arguments() : p(0), nchar(0) {}
	bool parse(argparser::situation& s) {
		if (s("--modulus", p)) return true;
		if (s("--in", in)) return true;
		if (s("--out", out)) return true;
		if (s("--clink", clink)) return true;
		if (s("--nchar", nchar)) return true;
		return false;
	}
	void doc(std::ostream& o) {
		o << "--modulus <p>\t modulus\n";
		o << "--in <filename>\t input row list\n";
		o << "--out <filename>\t output matrix\n";
		o << "--clink <filename>\t output clinks\n";
		o << "--nchar <integer>\t # of ideals to treat like characters\n";
	}
	bool check(std::ostream& o) {
		bool ok = true;
		if (in.empty() || out.empty() || clink.empty()) {
			o << "--in, --out, --clink are mandatory\n";
			ok = false;
		}
		if (!p) {
			o << "--modulus is mandatory\n";
			ok = false;
		}
		return ok;
	}
	void trigger() { }
};

#endif	/* NFSFORGE_ARGUMENTS_HPP_ */
