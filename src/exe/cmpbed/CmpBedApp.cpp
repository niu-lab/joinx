#include "CmpBedApp.hpp"

#include "bedutil/Bed.hpp"
#include "bedutil/BedStream.hpp"
#include "bedutil/IntersectAnnotation.hpp"
#include "bedutil/TranscriptStructure.hpp"
#include "bedutil/intconfig.hpp"

#include <boost/program_options.hpp>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace std;
namespace po = boost::program_options;

CmpBedApp::CmpBedApp(int argc, char** argv)
    : _firstOnly(false)
    , _outputBoth(false)
{
    parseArguments(argc, argv);
}

void CmpBedApp::parseArguments(int argc, char** argv) {
    po::options_description opts("Available Options");
    opts.add_options()
        ("help", "this message")
        ("file-a,a", po::value<string>(&_fileA), "input file a (required)")
        ("file-b,b", po::value<string>(&_fileB), "input file b (required)")
        ("first-only,f", "notice only the first thing to hit records in b, not the full intersection")
        ("output-both,", "concatenate intersecting lines in output (vs writing out only lines from 'a')");

    po::positional_options_description posOpts;
    posOpts.add("file-a", 1);
    posOpts.add("file-b", 1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .options(opts)
            .positional(posOpts).run(),
        vm
    );
    po::notify(vm);

    if (vm.count("help")) {
        stringstream ss;
        ss << opts;
        throw runtime_error(ss.str());
    }

    if (!vm.count("file-a")) {
        stringstream ss;
        ss << "no file-a specified!" << endl << endl << opts;
        throw runtime_error(ss.str());
    }

    if (!vm.count("file-b")) {
        stringstream ss;
        ss << "no file-b specified!" << endl << endl << opts;
        throw runtime_error(ss.str());
    }

    if (vm.count("first-only"))
        _firstOnly = true;
    
    if (vm.count("output-both"))
        _outputBoth = true;
}

namespace {
    void onHit(const Bed& a, const TranscriptStructure& b) {
        // mimic what bedtools does
        //unsigned start = std::max(a.start, b.start);
        //unsigned end = std::min(a.end, b.end);
        //cout << a.chrom << "\t" << start << "\t" << end << "\t" << a.refCall << "\t" << a.qual << "\n";
        cout << a << "\n";
    }

    void onHitBoth(const Bed& a, const TranscriptStructure& b) {
        cout << a << "\t" << b.line() << "\n";
    }
}

void CmpBedApp::exec() {
    ifstream inA(_fileA.c_str());
    if (!inA)
        throw runtime_error("Failed to open input file '" + _fileA + "'");
    ifstream inB(_fileB.c_str());
    if (!inB)
        throw runtime_error("Failed to open input file '" + _fileB + "'");

    BedStream fa(_fileA, inA);

    boost::function<void(const Bed&, const TranscriptStructure&)> action = onHit;
    if (_outputBoth)
        action = onHitBoth;
    IntersectAnnotation intersector(inB, action, _firstOnly);
    
    Bed bed;
    uint64_t lineNo = 0;
    while (fa.next(bed) && intersector.intersect(bed)) {
        ++lineNo;
        if (lineNo % 10000 == 0)
            cerr << "Processed " << lineNo << " lines" << endl;
    }
}